/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "satellite-net-device.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-static-routing-helper.h"

namespace
{

ns3::Ipv4Address
ResolveGatewayForPacket(ns3::Ptr<ns3::Node> node, ns3::Ptr<const ns3::Packet> packet)
{
    if (!node || !packet)
    {
        return ns3::Ipv4Address::GetZero();
    }

    ns3::Ipv4Header ipHeader;
    if (!packet->PeekHeader(ipHeader))
    {
        return ns3::Ipv4Address::GetZero();
    }

    ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
    if (!ipv4)
    {
        return ns3::Ipv4Address::GetZero();
    }

    ns3::Ipv4StaticRoutingHelper helper;
    ns3::Ptr<ns3::Ipv4StaticRouting> staticRouting = helper.GetStaticRouting(ipv4);
    if (!staticRouting)
    {
        return ns3::Ipv4Address::GetZero();
    }

    const ns3::Ipv4Address destination = ipHeader.GetDestination();
    for (uint32_t i = 0; i < staticRouting->GetNRoutes(); ++i)
    {
        ns3::Ipv4RoutingTableEntry route = staticRouting->GetRoute(i);
        if (route.GetDest() == destination)
        {
            return route.GetGateway();
        }
    }

    return ns3::Ipv4Address::GetZero();
}

bool
PeerHasIpv4Address(ns3::Ptr<ns3::SatelliteNetDevice> peer, const ns3::Ipv4Address& address)
{
    if (!peer || address == ns3::Ipv4Address::GetZero())
    {
        return false;
    }

    ns3::Ptr<ns3::Node> node = peer->GetNode();
    ns3::Ptr<ns3::Ipv4> ipv4 = node ? node->GetObject<ns3::Ipv4>() : nullptr;
    if (!ipv4)
    {
        return false;
    }

    for (uint32_t iface = 1; iface < ipv4->GetNInterfaces(); ++iface)
    {
        for (uint32_t addrIdx = 0; addrIdx < ipv4->GetNAddresses(iface); ++addrIdx)
        {
            if (ipv4->GetAddress(iface, addrIdx).GetLocal() == address)
            {
                return true;
            }
        }
    }

    return false;
}

} // namespace

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
    // IPv4 interface send paths may query broadcast even for point-to-point devices.
    // Return the standard MAC broadcast address for compatibility.
    return Mac48Address::GetBroadcast();
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

    const bool isBroadcast = (dest == Mac48Address::GetBroadcast());
    const Ipv4Address gateway = isBroadcast ? ResolveGatewayForPacket(m_node, packet)
                                            : Ipv4Address::GetZero();
    for (const auto& link : m_links)
    {
        if (!link)
        {
            continue;
        }

        Ptr<SatelliteNetDevice> peer = nullptr;
        Ptr<SatelliteNetDevice> dev0 = DynamicCast<SatelliteNetDevice>(link->GetDevice(0));
        Ptr<SatelliteNetDevice> dev1 = DynamicCast<SatelliteNetDevice>(link->GetDevice(1));
        if (dev0 == this)
        {
            peer = dev1;
        }
        else if (dev1 == this)
        {
            peer = dev0;
        }
        if (!peer)
        {
            continue;
        }

        if (isBroadcast && gateway != Ipv4Address::GetZero() && !PeerHasIpv4Address(peer, gateway))
        {
            continue;
        }

        const Address resolvedDest = isBroadcast ? peer->GetAddress() : dest;
        if (link->Send(this, packet, resolvedDest, protocolNumber, m_address))
        {
            return true;
        }
    }
    return false;
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

    const bool isBroadcast = (dest == Mac48Address::GetBroadcast());
    const Ipv4Address gateway = isBroadcast ? ResolveGatewayForPacket(m_node, packet)
                                            : Ipv4Address::GetZero();
    for (const auto& link : m_links)
    {
        if (!link)
        {
            continue;
        }

        Ptr<SatelliteNetDevice> peer = nullptr;
        Ptr<SatelliteNetDevice> dev0 = DynamicCast<SatelliteNetDevice>(link->GetDevice(0));
        Ptr<SatelliteNetDevice> dev1 = DynamicCast<SatelliteNetDevice>(link->GetDevice(1));
        if (dev0 == this)
        {
            peer = dev1;
        }
        else if (dev1 == this)
        {
            peer = dev0;
        }
        if (!peer)
        {
            continue;
        }

        if (isBroadcast && gateway != Ipv4Address::GetZero() && !PeerHasIpv4Address(peer, gateway))
        {
            continue;
        }

        const Address resolvedDest = isBroadcast ? peer->GetAddress() : dest;
        if (link->Send(this, packet, resolvedDest, protocolNumber, source))
        {
            return true;
        }
    }
    return false;
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
