/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "orbitshield-routing-helper.h"
#include "constellation.h"
#include "satellite.h"
#include "ground-station.h"
#include "satellite-net-device.h"
#include "satellite-link.h"

#include "ns3/ipv4-interface.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/attribute.h"
#include "ns3/log.h"

#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OrbitShieldRoutingHelper");

namespace
{

struct LinkInterfaceInfo
{
    Ptr<Node> a;
    Ptr<Node> b;
    uint32_t aIf{0};
    uint32_t bIf{0};
    Ipv4Address aAddr;
    Ipv4Address bAddr;
};

bool
IsLoopbackAddress(const Ipv4Address& address)
{
    return address == Ipv4Address::GetLoopback();
}

std::vector<Ipv4Address>
CollectNodeIpv4Addresses(Ptr<Ipv4> ipv4)
{
    std::vector<Ipv4Address> addresses;
    if (!ipv4)
    {
        return addresses;
    }

    for (uint32_t i = 1; i < ipv4->GetNInterfaces(); ++i)
    {
        for (uint32_t j = 0; j < ipv4->GetNAddresses(i); ++j)
        {
            const Ipv4Address local = ipv4->GetAddress(i, j).GetLocal();
            if (!IsLoopbackAddress(local))
            {
                addresses.push_back(local);
            }
        }
    }

    return addresses;
}

bool
FindLinkInterfaceInfo(Ptr<SatelliteLink> link, LinkInterfaceInfo& info)
{
    if (!link)
    {
        return false;
    }

    Ptr<NetDevice> devA = link->GetDevice(0);
    Ptr<NetDevice> devB = link->GetDevice(1);
    if (!devA || !devB)
    {
        return false;
    }

    Ptr<Node> nodeA = devA->GetNode();
    Ptr<Node> nodeB = devB->GetNode();
    if (!nodeA || !nodeB)
    {
        return false;
    }

    Ptr<Ipv4> ipv4A = nodeA->GetObject<Ipv4>();
    Ptr<Ipv4> ipv4B = nodeB->GetObject<Ipv4>();
    if (!ipv4A || !ipv4B)
    {
        return false;
    }

    static const Ipv4Mask kLinkMask("255.255.255.252");
    for (uint32_t ifA = 1; ifA < ipv4A->GetNInterfaces(); ++ifA)
    {
        for (uint32_t addrA = 0; addrA < ipv4A->GetNAddresses(ifA); ++addrA)
        {
            const Ipv4InterfaceAddress aIfAddr = ipv4A->GetAddress(ifA, addrA);
            if (aIfAddr.GetMask() != kLinkMask || IsLoopbackAddress(aIfAddr.GetLocal()))
            {
                continue;
            }

            const Ipv4Address aNetwork = aIfAddr.GetLocal().CombineMask(kLinkMask);

            for (uint32_t ifB = 1; ifB < ipv4B->GetNInterfaces(); ++ifB)
            {
                for (uint32_t addrB = 0; addrB < ipv4B->GetNAddresses(ifB); ++addrB)
                {
                    const Ipv4InterfaceAddress bIfAddr = ipv4B->GetAddress(ifB, addrB);
                    if (bIfAddr.GetMask() != kLinkMask || IsLoopbackAddress(bIfAddr.GetLocal()))
                    {
                        continue;
                    }

                    if (aNetwork == bIfAddr.GetLocal().CombineMask(kLinkMask) &&
                        aIfAddr.GetLocal() != bIfAddr.GetLocal())
                    {
                        info.a = nodeA;
                        info.b = nodeB;
                        info.aIf = ifA;
                        info.bIf = ifB;
                        info.aAddr = aIfAddr.GetLocal();
                        info.bAddr = bIfAddr.GetLocal();
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

} // namespace

OrbitShieldRoutingHelper::OrbitShieldRoutingHelper()
{
    NS_LOG_FUNCTION(this);
}

OrbitShieldRoutingHelper::~OrbitShieldRoutingHelper()
{
    NS_LOG_FUNCTION(this);
}

void
OrbitShieldRoutingHelper::Install(Ptr<Constellation> constellation)
{
    NS_LOG_FUNCTION(this << constellation);
    if (!constellation)
    {
        return;
    }

    // Install Internet stack on all satellites
    InternetStackHelper inetHelper;
    NodeContainer satNodes;
    for (const auto& sat : constellation->GetSatellites())
    {
        satNodes.Add(sat);
    }
    inetHelper.Install(satNodes);

    // Install Internet stack on all ground stations
    NodeContainer gsNodes;
    for (const auto& gs : constellation->GetGroundStations())
    {
        gsNodes.Add(gs);
    }
    inetHelper.Install(gsNodes);

    // Enable IPv4 forwarding on satellites
    for (const auto& sat : constellation->GetSatellites())
    {
        Ptr<Ipv4> ipv4 = sat->GetObject<Ipv4>();
        if (ipv4)
        {
            ipv4->SetAttribute("IpForward", BooleanValue(true));
        }
    }

    // Assign sequential /30 subnets to all links: ISLs first, then GSLs in creation order.
    // ns-3's Ipv4AddressHelper only allocates addresses within the single subnet established by
    // SetBase(); it does NOT auto-advance to the next subnet. We therefore call SetBase() with
    // the correct block address before each Assign() to obtain strict sequential /30 allocation:
    //   ISL link 0  → 10.0.0.0/30  → endpoints get 10.0.0.1 / 10.0.0.2
    //   ISL link 1  → 10.0.0.4/30  → endpoints get 10.0.0.5 / 10.0.0.6
    //   GSL link 0  → 10.0.0.(4*K)/30 (continues from last ISL block)
    //   ...
    static const uint32_t kBaseNet = 0x0A000000; // 10.0.0.0
    Ipv4AddressHelper addressHelper;
    uint32_t blockIndex = 0;

    auto assignLinkAddresses = [&](const std::vector<Ptr<SatelliteLink>>& links) {
        for (const auto& link : links)
        {
            Ptr<NetDevice> dev0 = link->GetDevice(0);
            Ptr<NetDevice> dev1 = link->GetDevice(1);
            if (dev0 && dev1)
            {
                // Set the base to the correct /30 block for this link index so
                // that NewAddress() always starts from offset 1 within the block.
                addressHelper.SetBase(Ipv4Address(kBaseNet + blockIndex * 4),
                                      Ipv4Mask("255.255.255.252"));
                NetDeviceContainer devices;
                devices.Add(dev0);
                devices.Add(dev1);
                addressHelper.Assign(devices);
                ++blockIndex;
            }
        }
    };

    const auto& isls = constellation->GetCurrentIsls();
    const auto& groundLinks = constellation->GetCurrentGroundLinks();
    assignLinkAddresses(isls);
    assignLinkAddresses(groundLinks);

    constellation->SetRouteUpdateCallback(
        MakeCallback(&OrbitShieldRoutingHelper::RecomputeRoutes, this));
    RecomputeRoutes(constellation);

    NS_LOG_INFO("Installed Internet stack and assigned IPv4 addresses to " << isls.size()
                                                                           << " ISLs and " << groundLinks.size() << " ground links");
}

void
OrbitShieldRoutingHelper::RecomputeRoutes(Ptr<Constellation> constellation)
{
    NS_LOG_FUNCTION(this << constellation);
    if (!constellation)
    {
        return;
    }

    std::vector<Ptr<Node>> nodes;
    for (const auto& sat : constellation->GetSatellites())
    {
        nodes.push_back(sat);
    }
    for (const auto& gs : constellation->GetGroundStations())
    {
        nodes.push_back(gs);
    }

    if (nodes.empty())
    {
        return;
    }

    std::unordered_map<uint32_t, std::size_t> nodeIndex;
    nodeIndex.reserve(nodes.size());
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        nodeIndex[nodes[i]->GetId()] = i;
    }

    struct NeighborEdge
    {
        std::size_t nextNode;
        uint32_t outIf;
        Ipv4Address nextHop;
    };

    std::vector<std::vector<NeighborEdge>> adjacency(nodes.size());
    auto addEdgesFromLinks = [&](const std::vector<Ptr<SatelliteLink>>& links) {
        for (const auto& link : links)
        {
            LinkInterfaceInfo info;
            if (!FindLinkInterfaceInfo(link, info))
            {
                continue;
            }

            auto aIt = nodeIndex.find(info.a->GetId());
            auto bIt = nodeIndex.find(info.b->GetId());
            if (aIt == nodeIndex.end() || bIt == nodeIndex.end())
            {
                continue;
            }

            adjacency[aIt->second].push_back({bIt->second, info.aIf, info.bAddr});
            adjacency[bIt->second].push_back({aIt->second, info.bIf, info.aAddr});
        }
    };

    addEdgesFromLinks(constellation->GetCurrentIsls());
    addEdgesFromLinks(constellation->GetCurrentGroundLinks());

    Ipv4StaticRoutingHelper staticRoutingHelper;
    std::vector<Ptr<Ipv4StaticRouting>> perNodeRouting(nodes.size());
    std::vector<std::vector<Ipv4Address>> perNodeAddresses(nodes.size());

    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        Ptr<Ipv4> ipv4 = nodes[i]->GetObject<Ipv4>();
        if (!ipv4)
        {
            continue;
        }

        Ptr<Ipv4StaticRouting> staticRouting = staticRoutingHelper.GetStaticRouting(ipv4);
        while (staticRouting->GetNRoutes() > 0)
        {
            staticRouting->RemoveRoute(0);
        }

        perNodeRouting[i] = staticRouting;
        perNodeAddresses[i] = CollectNodeIpv4Addresses(ipv4);
    }

    for (std::size_t source = 0; source < nodes.size(); ++source)
    {
        Ptr<Ipv4StaticRouting> sourceRouting = perNodeRouting[source];
        if (!sourceRouting)
        {
            continue;
        }

        std::vector<int32_t> parent(nodes.size(), -1);
        std::queue<std::size_t> pending;
        parent[source] = static_cast<int32_t>(source);
        pending.push(source);

        while (!pending.empty())
        {
            const std::size_t current = pending.front();
            pending.pop();

            for (const auto& edge : adjacency[current])
            {
                if (parent[edge.nextNode] == -1)
                {
                    parent[edge.nextNode] = static_cast<int32_t>(current);
                    pending.push(edge.nextNode);
                }
            }
        }

        std::unordered_set<uint32_t> installedDestinations;
        for (std::size_t destination = 0; destination < nodes.size(); ++destination)
        {
            if (destination == source || parent[destination] == -1)
            {
                continue;
            }

            std::size_t firstHop = destination;
            while (parent[firstHop] != static_cast<int32_t>(source))
            {
                firstHop = static_cast<std::size_t>(parent[firstHop]);
            }

            const auto nextHopIt = std::find_if(adjacency[source].begin(), adjacency[source].end(),
                                                [firstHop](const NeighborEdge& edge) {
                                                    return edge.nextNode == firstHop;
                                                });
            if (nextHopIt == adjacency[source].end())
            {
                continue;
            }

            for (const auto& destinationAddress : perNodeAddresses[destination])
            {
                if (installedDestinations.insert(destinationAddress.Get()).second)
                {
                    sourceRouting->AddHostRouteTo(destinationAddress,
                                                  nextHopIt->nextHop,
                                                  nextHopIt->outIf);
                }
            }
        }
    }
}

}  // namespace ns3
