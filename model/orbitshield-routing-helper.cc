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

    // Assign IPv4 addresses to all links using /30 subnets
    uint32_t addressCounter = 0;
    Ipv4AddressHelper addressHelper;
    
    // Process ISL links
    const auto& isls = constellation->GetCurrentIsls();
    for (const auto& link : isls)
    {
        // Assign /30 subnet starting from 10.0.0.0
        std::ostringstream oss;
        oss << "10." << (addressCounter / 256) << "." << (addressCounter % 256) << ".0";
        addressHelper.SetBase(Ipv4Address(oss.str().c_str()), Ipv4Mask("255.255.255.252"));
        
        // Get the two devices on this link
        Ptr<NetDevice> dev0 = link->GetDevice(0);
        Ptr<NetDevice> dev1 = link->GetDevice(1);
        
        if (dev0 && dev1)
        {
            NetDeviceContainer devices;
            devices.Add(dev0);
            devices.Add(dev1);
            addressHelper.Assign(devices);
            addressCounter += 4;  // Each /30 subnet uses 4 addresses
        }
    }

    // Process ground links
    const auto& groundLinks = constellation->GetCurrentGroundLinks();
    for (const auto& link : groundLinks)
    {
        // Assign /30 subnet starting from a higher range
        uint32_t adjustedCounter = addressCounter + (1u << 16);  // Offset for ground links
        std::ostringstream oss;
        oss << "10." << (adjustedCounter / 256) << "." << (adjustedCounter % 256) << ".0";
        addressHelper.SetBase(Ipv4Address(oss.str().c_str()), Ipv4Mask("255.255.255.252"));

        // Get the two devices on this link
        Ptr<NetDevice> dev0 = link->GetDevice(0);
        Ptr<NetDevice> dev1 = link->GetDevice(1);
        
        if (dev0 && dev1)
        {
            NetDeviceContainer devices;
            devices.Add(dev0);
            devices.Add(dev1);
            addressHelper.Assign(devices);
            addressCounter += 4;
        }
    }

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
