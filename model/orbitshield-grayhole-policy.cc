/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "orbitshield-grayhole-policy.h"

#include "satellite.h"

#include "ns3/ipv4-header.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OrbitShieldGrayholePolicy");
NS_OBJECT_ENSURE_REGISTERED(OrbitShieldGrayholePolicy);

namespace
{

std::string
DefaultTargetPairId(Ipv4Address source, Ipv4Address destination)
{
    std::ostringstream out;
    out << source << "->" << destination;
    return out.str();
}

std::unordered_set<std::string>
MakeSatelliteSet(const std::vector<std::string>& satelliteNames)
{
    return std::unordered_set<std::string>(satelliteNames.begin(), satelliteNames.end());
}

} // namespace

TypeId
OrbitShieldGrayholePolicy::GetTypeId()
{
    static TypeId tid = TypeId("ns3::OrbitShieldGrayholePolicy")
                            .SetParent<Object>()
                            .SetGroupName("OrbitShield")
                            .AddConstructor<OrbitShieldGrayholePolicy>();
    return tid;
}

OrbitShieldGrayholePolicy::OrbitShieldGrayholePolicy()
    : m_random(CreateObject<UniformRandomVariable>())
{
    NS_LOG_FUNCTION(this);
}

OrbitShieldGrayholePolicy::~OrbitShieldGrayholePolicy()
{
    NS_LOG_FUNCTION(this);
}

void
OrbitShieldGrayholePolicy::SetCompromisedSatellites(const std::vector<std::string>& satelliteNames)
{
    m_compromisedSatellites = MakeSatelliteSet(satelliteNames);
}

void
OrbitShieldGrayholePolicy::SetAttackWindow(Time start, Time stop)
{
    m_attackStart = start;
    m_attackStop = stop;
}

void
OrbitShieldGrayholePolicy::SetDropProbability(double probability)
{
    m_dropProbability = std::max(0.0, std::min(1.0, probability));
}

void
OrbitShieldGrayholePolicy::SetDirection(OrbitShieldScenario3Direction direction)
{
    m_direction = direction;
}

int64_t
OrbitShieldGrayholePolicy::AssignStreams(int64_t stream)
{
    m_random->SetStream(stream);
    return 1;
}

void
OrbitShieldGrayholePolicy::AddTargetPair(Ipv4Address source,
                                         Ipv4Address destination,
                                         const std::vector<std::string>& routeSatellites,
                                         const std::string& targetPairId)
{
    TargetPair targetPair;
    targetPair.source = source;
    targetPair.destination = destination;
    targetPair.targetPairId = targetPairId.empty() ? DefaultTargetPairId(source, destination) : targetPairId;
    targetPair.routeSatellites = MakeSatelliteSet(routeSatellites);
    m_targetPairs.push_back(std::move(targetPair));
}

void
OrbitShieldGrayholePolicy::SetTargetRouteSatellites(
    Ipv4Address source,
    Ipv4Address destination,
    const std::vector<std::string>& routeSatellites)
{
    TargetPair* targetPair = FindTargetPair(source, destination);
    if (!targetPair)
    {
        AddTargetPair(source, destination, routeSatellites);
        return;
    }
    targetPair->routeSatellites = MakeSatelliteSet(routeSatellites);
}

void
OrbitShieldGrayholePolicy::ClearTargetPairs()
{
    m_targetPairs.clear();
}

void
OrbitShieldGrayholePolicy::SetDecisionCallback(DecisionCallback callback)
{
    m_decisionCallback = callback;
}

bool
OrbitShieldGrayholePolicy::ShouldDrop(Ptr<Node> node,
                                      Ptr<const Packet> packet,
                                      uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << node << packet << protocolNumber);
    (void)protocolNumber;

    if (!node || !packet)
    {
        return false;
    }

    Ptr<Packet> packetCopy = packet->Copy();
    Ipv4Header ipv4Header;
    if (!packetCopy->PeekHeader(ipv4Header))
    {
        return false;
    }

    const Ipv4Address source = ipv4Header.GetSource();
    const Ipv4Address destination = ipv4Header.GetDestination();
    const TargetPair* targetPair = MatchTargetPair(source, destination);
    if (!targetPair)
    {
        return false;
    }

    Ptr<Satellite> satellite = DynamicCast<Satellite>(node);
    const std::string nodeName = satellite ? satellite->GetName() : std::string();
    if (!satellite)
    {
        EmitDecision(node, nodeName, source, destination, targetPair->targetPairId, "not-satellite", false);
        return false;
    }

    if (m_compromisedSatellites.count(nodeName) == 0)
    {
        EmitDecision(node, nodeName, source, destination, targetPair->targetPairId, "not-compromised", false);
        return false;
    }

    if (targetPair->routeSatellites.count(nodeName) == 0)
    {
        EmitDecision(node, nodeName, source, destination, targetPair->targetPairId, "not-route-active", false);
        return false;
    }

    const Time now = Simulator::Now();
    if (now < m_attackStart || now >= m_attackStop)
    {
        EmitDecision(node, nodeName, source, destination, targetPair->targetPairId, "outside-window", false);
        return false;
    }

    const bool dropped = m_dropProbability >= 1.0 ||
                         (m_dropProbability > 0.0 && m_random->GetValue() < m_dropProbability);
    EmitDecision(node,
                 nodeName,
                 source,
                 destination,
                 targetPair->targetPairId,
                 dropped ? "grayhole-drop" : "probability-forward",
                 dropped);
    return dropped;
}

OrbitShieldGrayholePolicy::TargetPair*
OrbitShieldGrayholePolicy::FindTargetPair(Ipv4Address source, Ipv4Address destination)
{
    for (auto& targetPair : m_targetPairs)
    {
        if (targetPair.source == source && targetPair.destination == destination)
        {
            return &targetPair;
        }
    }
    return nullptr;
}

const OrbitShieldGrayholePolicy::TargetPair*
OrbitShieldGrayholePolicy::MatchTargetPair(Ipv4Address source, Ipv4Address destination) const
{
    for (const auto& targetPair : m_targetPairs)
    {
        if (IsDirectionMatch(targetPair, source, destination))
        {
            return &targetPair;
        }
    }
    return nullptr;
}

bool
OrbitShieldGrayholePolicy::IsDirectionMatch(const TargetPair& targetPair,
                                            Ipv4Address source,
                                            Ipv4Address destination) const
{
    const bool forward = targetPair.source == source && targetPair.destination == destination;
    const bool reverse = targetPair.source == destination && targetPair.destination == source;

    switch (m_direction)
    {
    case OrbitShieldScenario3Direction::FORWARD:
        return forward;
    case OrbitShieldScenario3Direction::REVERSE:
        return reverse;
    case OrbitShieldScenario3Direction::BIDIRECTIONAL:
        return forward || reverse;
    }
    return forward || reverse;
}

void
OrbitShieldGrayholePolicy::EmitDecision(Ptr<Node> node,
                                        const std::string& nodeName,
                                        Ipv4Address source,
                                        Ipv4Address destination,
                                        const std::string& targetPairId,
                                        const std::string& reason,
                                        bool dropped) const
{
    if (!m_decisionCallback.IsNull())
    {
        m_decisionCallback(Simulator::Now(),
                           node ? node->GetId() : 0,
                           nodeName,
                           source,
                           destination,
                           targetPairId,
                           reason,
                           dropped);
    }
}

} // namespace ns3