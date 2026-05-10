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
#include "ns3/attribute.h"
#include "ns3/log.h"

#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OrbitShieldRoutingHelper");

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

    NS_LOG_INFO("Installed Internet stack and assigned IPv4 addresses to " << isls.size()
                                                                           << " ISLs and " << groundLinks.size() << " ground links");
}

void
OrbitShieldRoutingHelper::RecomputeRoutes(Ptr<Constellation> constellation)
{
    NS_LOG_FUNCTION(this << constellation);
    // Stub: no-op in Milestone 1.2
    // Real implementation deferred to Milestone 4
}

}  // namespace ns3
