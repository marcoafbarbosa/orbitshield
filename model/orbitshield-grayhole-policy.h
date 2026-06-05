/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_GRAYHOLE_POLICY_H
#define ORBITSHIELD_GRAYHOLE_POLICY_H

#include "orbitshield-scenario3-config.h"

#include "ns3/callback.h"
#include "ns3/ipv4-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace ns3
{

class OrbitShieldGrayholePolicy : public Object
{
  public:
    using DecisionCallback = Callback<void,
                                      Time,
                                      uint32_t,
                                      std::string,
                                      Ipv4Address,
                                      Ipv4Address,
                                      std::string,
                                      std::string,
                                      bool>;

    static TypeId GetTypeId();

    OrbitShieldGrayholePolicy();
    ~OrbitShieldGrayholePolicy() override;

    void SetCompromisedSatellites(const std::vector<std::string>& satelliteNames);
    void SetAttackWindow(Time start, Time stop);
    void SetDropProbability(double probability);
    void SetDirection(OrbitShieldScenario3Direction direction);
    int64_t AssignStreams(int64_t stream);

    void AddTargetPair(Ipv4Address source,
                       Ipv4Address destination,
                       const std::vector<std::string>& routeSatellites,
                       const std::string& targetPairId = "");
    void SetTargetRouteSatellites(Ipv4Address source,
                                  Ipv4Address destination,
                                  const std::vector<std::string>& routeSatellites);
    void ClearTargetPairs();

    void SetDecisionCallback(DecisionCallback callback);
    bool ShouldDrop(Ptr<Node> node, Ptr<const Packet> packet, uint16_t protocolNumber);

  private:
    struct TargetPair
    {
        Ipv4Address source;
        Ipv4Address destination;
        std::string targetPairId;
        std::unordered_set<std::string> routeSatellites;
    };

    TargetPair* FindTargetPair(Ipv4Address source, Ipv4Address destination);
    const TargetPair* MatchTargetPair(Ipv4Address source, Ipv4Address destination) const;
    bool IsDirectionMatch(const TargetPair& targetPair,
                          Ipv4Address source,
                          Ipv4Address destination) const;
    void EmitDecision(Ptr<Node> node,
                      const std::string& nodeName,
                      Ipv4Address source,
                      Ipv4Address destination,
                      const std::string& targetPairId,
                      const std::string& reason,
                      bool dropped) const;

    std::unordered_set<std::string> m_compromisedSatellites;
    std::vector<TargetPair> m_targetPairs;
    OrbitShieldScenario3Direction m_direction{OrbitShieldScenario3Direction::BIDIRECTIONAL};
    Time m_attackStart{Seconds(0)};
    Time m_attackStop{Seconds(0)};
    double m_dropProbability{0.0};
    Ptr<UniformRandomVariable> m_random;
    DecisionCallback m_decisionCallback;
};

} // namespace ns3

#endif /* ORBITSHIELD_GRAYHOLE_POLICY_H */