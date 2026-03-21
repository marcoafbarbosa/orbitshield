/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "satellite-link.h"
#include "isl-propagation-delay-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "satellite-net-device.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SatelliteLink");

NS_OBJECT_ENSURE_REGISTERED(SatelliteLink);

TypeId
SatelliteLink::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatelliteLink")
            .SetParent<Channel>()
            .SetGroupName("OrbitShield")
            .AddConstructor<SatelliteLink>();
    return tid;
}

SatelliteLink::SatelliteLink()
    : m_deviceA(nullptr), m_deviceB(nullptr), m_maxRange(1000000.0), m_delayModel(CreateObject<IslPropagationDelayModel>())
{
    NS_LOG_FUNCTION(this);
}

SatelliteLink::SatelliteLink(Ptr<SatelliteNetDevice> a, Ptr<SatelliteNetDevice> b)
    : m_deviceA(a), m_deviceB(b), m_maxRange(1000000.0), m_delayModel(CreateObject<IslPropagationDelayModel>())
{
    NS_LOG_FUNCTION(this << a << b);
    if (a) a->SetLink(this);
    if (b) b->SetLink(this);
}

SatelliteLink::~SatelliteLink()
{
    NS_LOG_FUNCTION(this);
}

std::size_t
SatelliteLink::GetNDevices() const
{
    NS_LOG_FUNCTION(this);
    return 2;
}

Ptr<NetDevice>
SatelliteLink::GetDevice(std::size_t i) const
{
    NS_LOG_FUNCTION(this << i);
    if (i == 0)
    {
        return m_deviceA;
    }
    if (i == 1)
    {
        return m_deviceB;
    }
    return nullptr;
}

void
SatelliteLink::SetMaxRange(double range)
{
    NS_LOG_FUNCTION(this << range);
    m_maxRange = range;
}

double
SatelliteLink::GetMaxRange() const
{
    NS_LOG_FUNCTION(this);
    return m_maxRange;
}

void
SatelliteLink::SetPropagationDelayModel(Ptr<PropagationDelayModel> delayModel)
{
    NS_LOG_FUNCTION(this << delayModel);
    m_delayModel = delayModel;
}

Ptr<PropagationDelayModel>
SatelliteLink::GetPropagationDelayModel() const
{
    NS_LOG_FUNCTION(this);
    return m_delayModel;
}

bool
SatelliteLink::Send(Ptr<NetDevice> source,
                     Ptr<Packet> packet,
                     const Address& dest,
                     uint16_t protocolNumber,
                     const Address& sourceAddress)
{
    NS_LOG_FUNCTION(this << source << packet << dest << protocolNumber << sourceAddress);

    Ptr<SatelliteNetDevice> srcDev = DynamicCast<SatelliteNetDevice>(source);
    if (!srcDev)
    {
        return false;
    }

    Ptr<SatelliteNetDevice> dstDev = nullptr;
    if (srcDev == m_deviceA)
    {
        dstDev = m_deviceB;
    }
    else if (srcDev == m_deviceB)
    {
        dstDev = m_deviceA;
    }
    else
    {
        return false;
    }

    if (!dstDev)
    {
        return false;
    }

    if (dest != dstDev->GetAddress() && dest != Mac48Address::GetBroadcast())
    {
        return false;
    }

    Ptr<Node> srcNode = srcDev->GetNode();
    Ptr<Node> dstNode = dstDev->GetNode();
    if (!srcNode || !dstNode)
    {
        return false;
    }

    Ptr<MobilityModel> srcMob = srcNode->GetObject<MobilityModel>();
    Ptr<MobilityModel> dstMob = dstNode->GetObject<MobilityModel>();
    if (!srcMob || !dstMob)
    {
        return false;
    }

    double distance = srcMob->GetDistanceFrom(dstMob);
    if (distance > m_maxRange)
    {
        return false;
    }

    Time delay = Seconds(0.0);
    if (m_delayModel)
    {
        delay = m_delayModel->GetDelay(srcMob, dstMob);
    }

    Ptr<Packet> packetCopy = packet->Copy();

    Simulator::ScheduleWithContext(
        dstNode->GetId(),
        delay,
        &SatelliteNetDevice::ReceiveFromChannel,
        dstDev,
        packetCopy,
        sourceAddress,
        dstDev->GetAddress(),
        protocolNumber);

    return true;
}

} // namespace ns3
