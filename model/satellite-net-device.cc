/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "satellite-net-device.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SatelliteNetDevice");

NS_OBJECT_ENSURE_REGISTERED(SatelliteNetDevice);

TypeId
SatelliteNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatelliteNetDevice")
            .SetParent<NetDevice>()
            .SetGroupName("OrbitShield")
            .AddConstructor<SatelliteNetDevice>();
    return tid;
}

SatelliteNetDevice::SatelliteNetDevice()
    : m_ifIndex(0), m_linkUp(true), m_mtu(1500)
{
    NS_LOG_FUNCTION(this);
    m_address = Mac48Address::Allocate();
}

SatelliteNetDevice::~SatelliteNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
SatelliteNetDevice::AddLink(Ptr<SatelliteLink> link)
{
    NS_LOG_FUNCTION(this << link);
    m_links.push_back(link);
}

void
SatelliteNetDevice::SetLink(Ptr<SatelliteLink> link)
{
    NS_LOG_FUNCTION(this << link);
    m_links.clear();
    if (link)
    {
        m_links.push_back(link);
    }
}

Ptr<SatelliteLink>
SatelliteNetDevice::GetSatelliteLink() const
{
    if (m_links.empty())
    {
        return nullptr;
    }
    return m_links[0];
}

const std::vector<Ptr<SatelliteLink>>&
SatelliteNetDevice::GetLinks() const
{
    return m_links;
}

bool
SatelliteNetDevice::ReceiveFromChannel(Ptr<Packet> packet,
                                       const Address& source,
                                       const Address& dest,
                                       uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << source << dest << protocolNumber);
    if (!m_receiveCallback.IsNull())
    {
        return m_receiveCallback(this, packet, protocolNumber, source);
    }
    return false;
}

void
SatelliteNetDevice::SetIfIndex(const uint32_t index)
{
    m_ifIndex = index;
}

uint32_t
SatelliteNetDevice::GetIfIndex() const
{
    return m_ifIndex;
}

Ptr<Channel>
SatelliteNetDevice::GetChannel() const
{
    if (m_links.empty())
    {
        return nullptr;
    }
    return m_links[0];
}

void
SatelliteNetDevice::SetAddress(Address address)
{
    m_address = address;
}

Address
SatelliteNetDevice::GetAddress() const
{
    return m_address;
}

bool
SatelliteNetDevice::SetMtu(const uint16_t mtu)
{
    m_mtu = mtu;
    return true;
}

uint16_t
SatelliteNetDevice::GetMtu() const
{
    return m_mtu;
}

bool
SatelliteNetDevice::IsLinkUp() const
{
    return m_linkUp;
}

void
SatelliteNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    m_linkChangeCallback = callback;
}

bool
SatelliteNetDevice::IsBroadcast() const
{
    return false;
}

Address
SatelliteNetDevice::GetBroadcast() const
{
    NS_ASSERT_MSG(false, "Point-to-point devices do not have broadcast addresses");
    return Address();
}

bool
SatelliteNetDevice::IsMulticast() const
{
    return false;
}

Address
SatelliteNetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    NS_ASSERT_MSG(false, "Point-to-point devices do not support multicast");
    return Address();
}

Address
SatelliteNetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_ASSERT_MSG(false, "Point-to-point devices do not support multicast");
    return Address();
}

bool
SatelliteNetDevice::IsBridge() const
{
    return false;
}

bool
SatelliteNetDevice::IsPointToPoint() const
{
    return true;
}

bool
SatelliteNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);
    if (m_links.empty())
    {
        return false;
    }

    // Send packet on all active links
    bool anySent = false;
    for (const auto& link : m_links)
    {
        if (link && link->Send(this, packet, dest, protocolNumber, m_address))
        {
            anySent = true;
        }
    }
    return anySent;
}

bool
SatelliteNetDevice::SendFrom(Ptr<Packet> packet,
                              const Address& source,
                              const Address& dest,
                              uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << source << dest << protocolNumber);
    if (m_links.empty())
    {
        return false;
    }

    // Send packet on all active links
    bool anySent = false;
    for (const auto& link : m_links)
    {
        if (link && link->Send(this, packet, dest, protocolNumber, source))
        {
            anySent = true;
        }
    }
    return anySent;
}

Ptr<Node>
SatelliteNetDevice::GetNode() const
{
    return m_node;
}

void
SatelliteNetDevice::SetNode(Ptr<Node> node)
{
    m_node = node;
}

bool
SatelliteNetDevice::NeedsArp() const
{
    return false;
}

void
SatelliteNetDevice::SetReceiveCallback(ReceiveCallback cb)
{
    m_receiveCallback = cb;
}

void
SatelliteNetDevice::SetPromiscReceiveCallback(PromiscReceiveCallback cb)
{
    m_promiscCallback = cb;
}

bool
SatelliteNetDevice::SupportsSendFrom() const
{
    return true;
}

} // namespace ns3
