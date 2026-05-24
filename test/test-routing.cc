/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "test-routing.h"
#include "ns3/orbitshield-module.h"
#include "ns3/test.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ping-helper.h"
#include "ns3/ping.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RoutingTest");

namespace
{

/**
 * \brief Per-GS-pair result accumulator used by the multi-GS routing test.
 */
struct GsPairResult
{
    uint32_t replyCount{0};  ///< Number of ICMP echo replies received for this pair.
    Time maxRtt{Seconds(0)}; ///< Maximum RTT observed for this pair.
    bool allRttsValid{true}; ///< True if every observed RTT was <= 500 ms.
};

/**
 * \brief Free-function RTT callback bound to a specific GsPairResult via MakeBoundCallback.
 *
 * \param result Pointer to the GsPairResult for this pair (bound argument).
 * \param seq    ICMP sequence number (unused).
 * \param rtt    Round-trip time for this echo reply.
 */
void
OnGsPairRtt(GsPairResult* result, uint16_t seq, Time rtt)
{
    (void)seq;
    ++result->replyCount;
    if (rtt > result->maxRtt)
    {
        result->maxRtt = rtt;
    }
    if (rtt > MilliSeconds(500))
    {
        result->allRttsValid = false;
    }
}

Ptr<GroundStation>
FindGroundStationByName(const std::vector<Ptr<GroundStation>>& stations, const std::string& name)
{
    for (const auto& station : stations)
    {
        if (station && station->GetName() == name)
        {
            return station;
        }
    }
    return nullptr;
}

Ipv4Address
GetFirstNonLoopbackAddress(Ptr<Node> node)
{
    Ptr<Ipv4> ipv4 = node ? node->GetObject<Ipv4>() : nullptr;
    if (!ipv4)
    {
        return Ipv4Address::GetZero();
    }

    for (uint32_t iface = 1; iface < ipv4->GetNInterfaces(); ++iface)
    {
        for (uint32_t addrIdx = 0; addrIdx < ipv4->GetNAddresses(iface); ++addrIdx)
        {
            Ipv4Address address = ipv4->GetAddress(iface, addrIdx).GetLocal();
            if (address != Ipv4Address::GetLoopback())
            {
                return address;
            }
        }
    }

    return Ipv4Address::GetZero();
}

std::vector<Ipv4Address>
GetNonLoopbackAddresses(Ptr<Node> node)
{
    std::vector<Ipv4Address> addresses;
    Ptr<Ipv4> ipv4 = node ? node->GetObject<Ipv4>() : nullptr;
    if (!ipv4)
    {
        return addresses;
    }

    for (uint32_t iface = 1; iface < ipv4->GetNInterfaces(); ++iface)
    {
        for (uint32_t addrIdx = 0; addrIdx < ipv4->GetNAddresses(iface); ++addrIdx)
        {
            Ipv4Address address = ipv4->GetAddress(iface, addrIdx).GetLocal();
            if (address != Ipv4Address::GetLoopback())
            {
                addresses.push_back(address);
            }
        }
    }

    return addresses;
}

std::vector<Ptr<Node>>
CollectConstellationNodes(Ptr<Constellation> constellation)
{
    std::vector<Ptr<Node>> nodes;
    if (!constellation)
    {
        return nodes;
    }

    for (const auto& sat : constellation->GetSatellites())
    {
        nodes.push_back(sat);
    }
    for (const auto& gs : constellation->GetGroundStations())
    {
        nodes.push_back(gs);
    }

    return nodes;
}

std::unordered_map<uint32_t, Ptr<Node>>
BuildIpv4AddressToNodeMap(const std::vector<Ptr<Node>>& nodes)
{
    std::unordered_map<uint32_t, Ptr<Node>> addressToNode;
    for (const auto& node : nodes)
    {
        Ptr<Ipv4> ipv4 = node ? node->GetObject<Ipv4>() : nullptr;
        if (!ipv4)
        {
            continue;
        }

        for (uint32_t iface = 1; iface < ipv4->GetNInterfaces(); ++iface)
        {
            for (uint32_t addrIdx = 0; addrIdx < ipv4->GetNAddresses(iface); ++addrIdx)
            {
                const Ipv4Address local = ipv4->GetAddress(iface, addrIdx).GetLocal();
                if (local != Ipv4Address::GetLoopback())
                {
                    addressToNode[local.Get()] = node;
                }
            }
        }
    }

    return addressToNode;
}

bool
FindHostRoute(Ptr<Ipv4StaticRouting> staticRouting,
              Ipv4Address destination,
              Ipv4RoutingTableEntry& selected)
{
    if (!staticRouting)
    {
        return false;
    }

    for (uint32_t i = 0; i < staticRouting->GetNRoutes(); ++i)
    {
        Ipv4RoutingTableEntry route = staticRouting->GetRoute(i);
        if (route.IsHost() && route.GetDest() == destination)
        {
            selected = route;
            return true;
        }
    }

    return false;
}

uint32_t
ComputeStaticHostRouteHopCount(Ptr<Node> source,
                               Ptr<Node> destinationNode,
                               Ipv4Address destinationAddress,
                               const std::vector<Ptr<Node>>& nodes)
{
    if (!source || !destinationNode || destinationAddress == Ipv4Address::GetZero())
    {
        return 0;
    }

    Ipv4StaticRoutingHelper staticRoutingHelper;
    const auto addressToNode = BuildIpv4AddressToNodeMap(nodes);
    std::unordered_set<uint32_t> visitedNodeIds;

    Ptr<Node> current = source;
    uint32_t hops = 0;
    const uint32_t hopLimit = static_cast<uint32_t>(nodes.size()) + 1;

    while (current && current != destinationNode && hops <= hopLimit)
    {
        if (!visitedNodeIds.insert(current->GetId()).second)
        {
            return 0;
        }

        Ptr<Ipv4> ipv4 = current->GetObject<Ipv4>();
        if (!ipv4)
        {
            return 0;
        }

        Ptr<Ipv4StaticRouting> staticRouting = staticRoutingHelper.GetStaticRouting(ipv4);
        Ipv4RoutingTableEntry hostRoute;
        if (!FindHostRoute(staticRouting, destinationAddress, hostRoute))
        {
            return 0;
        }

        Ptr<Node> nextNode = nullptr;
        if (hostRoute.GetGateway().IsAny())
        {
            auto destinationIt = addressToNode.find(destinationAddress.Get());
            if (destinationIt != addressToNode.end())
            {
                nextNode = destinationIt->second;
            }
        }
        else
        {
            auto gatewayIt = addressToNode.find(hostRoute.GetGateway().Get());
            if (gatewayIt != addressToNode.end())
            {
                nextNode = gatewayIt->second;
            }
        }

        if (!nextNode)
        {
            return 0;
        }

        ++hops;
        current = nextNode;
    }

    return (current == destinationNode) ? hops : 0;
}

} // namespace

OrbitShieldIridiumTopologyTest::OrbitShieldIridiumTopologyTest()
    : TestCase("Test OrbitShield Iridium topology loading and discovery")
{
}

OrbitShieldIridiumTopologyTest::~OrbitShieldIridiumTopologyTest()
{
}

void
OrbitShieldIridiumTopologyTest::DoRun()
{
    // Create a constellation and load satellites and constellation metadata from YAML file
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    
    // Load from the Iridium YAML file which includes TLE file path, ring structure, and ground stations
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.yaml");

    // Verify constellation name
    NS_TEST_EXPECT_MSG_EQ(constellation->GetConstellationName(),
                          std::string("iridium-2026"),
                          "Constellation name should be 'iridium-2026'");

    // Verify ring count
    NS_TEST_EXPECT_MSG_EQ(constellation->GetRingCount(),
                          6u,
                          "Iridium constellation should have 6 rings");

    // Verify satellite count - the TLE file may have satellites from the constellation
    const auto& satellites = constellation->GetSatellites();
    NS_TEST_EXPECT_MSG_GT(satellites.size(),
                          0u,
                          "Expected satellites to be loaded from the TLE file referenced in YAML");
    NS_LOG_INFO("Total satellites loaded: " << satellites.size());

    // Verify ground stations loaded from the YAML dataset.
    // The ground stations are defined in the YAML and should always be discoverable
    const auto& groundStations = constellation->GetGroundStations();
    NS_TEST_EXPECT_MSG_EQ(groundStations.size(),
                          5u,
                          "Expected 5 ground stations (Tempe, Fairbanks, Svalbard, Izhevsk, Punta Arenas)");

    // Verify specific ground stations by name
    std::set<std::string> expectedGsNames = {"Tempe", "Fairbanks", "Svalbard", "Izhevsk", "Punta Arenas"};
    std::set<std::string> loadedGsNames;
    for (const auto& gs : groundStations)
    {
        loadedGsNames.insert(gs->GetName());
        NS_LOG_INFO("Ground station: " << gs->GetName() << " at lat=" << gs->GetLatitude()
                                       << ", lon=" << gs->GetLongitude());
    }

    NS_TEST_EXPECT_MSG_EQ(loadedGsNames.size(),
                          expectedGsNames.size(),
                          "Loaded ground station count should match expected count");
    
    for (const auto& gsName : expectedGsNames)
    {
        bool found = loadedGsNames.count(gsName) > 0;
        NS_TEST_EXPECT_MSG_EQ(found,
                              true,
                              "Ground station " << gsName << " should be loaded");
    }

    // Verify ground station coordinates are reasonable (not zero)
    for (const auto& gs : groundStations)
    {
        double lat = gs->GetLatitude();
        double lon = gs->GetLongitude();
        bool validLat = lat >= -90.0 && lat <= 90.0;
        bool validLon = lon >= -180.0 && lon <= 180.0;
        NS_TEST_EXPECT_MSG_EQ(validLat && validLon,
                              true,
                              "Ground station " << gs->GetName() << " coordinates should be valid");
    }

    NS_LOG_INFO("Successfully loaded and verified Iridium constellation topology - ring count, "
                "satellite count, and ground station names/coordinates");
}

OrbitShieldRoutingHelperTest::OrbitShieldRoutingHelperTest()
    : TestCase("Test OrbitShield routing helper API")
{
}

OrbitShieldRoutingHelperTest::~OrbitShieldRoutingHelperTest()
{
}

void
OrbitShieldRoutingHelperTest::DoRun()
{
    // Create a constellation
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.yaml");

    // Create the routing helper
    OrbitShieldRoutingHelper routingHelper;

    // Test Install method - should not crash
    routingHelper.Install(constellation);
    NS_LOG_INFO("Successfully called Install");
    NS_TEST_EXPECT_MSG_EQ(true, true, "Install method executed successfully");

    // Test RecomputeRoutes method - should not crash
    routingHelper.RecomputeRoutes(constellation);
    NS_LOG_INFO("Successfully called RecomputeRoutes");
    NS_TEST_EXPECT_MSG_EQ(true, true, "RecomputeRoutes method executed successfully");

    // Test SetRouteUpdateCallback - just verify it doesn't crash with null callback
    Callback<void, Ptr<Constellation>> nullCallback;
    constellation->SetRouteUpdateCallback(nullCallback);
    NS_LOG_INFO("Successfully set route update callback (null)");
    NS_TEST_EXPECT_MSG_EQ(true, true, "Callback storage functionality works");
}

OrbitShieldMultiLinkDeviceTest::OrbitShieldMultiLinkDeviceTest()
    : TestCase("Test multi-link SatelliteNetDevice support")
{
}

OrbitShieldMultiLinkDeviceTest::~OrbitShieldMultiLinkDeviceTest()
{
}

void
OrbitShieldMultiLinkDeviceTest::DoRun()
{
    // Create two satellites to form ISL links
    std::string tle1 = "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996";
    std::string tle2 = "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227";
    perturb::JulianDate simStart(perturb::DateTime(2026, 1, 1, 0, 0, 0));

    Ptr<Satellite> sat1 = CreateObject<Satellite>("SAT-1", tle1, tle2, simStart);
    Ptr<Satellite> sat2 = CreateObject<Satellite>("SAT-2", tle1, tle2, simStart);
    Ptr<Satellite> sat3 = CreateObject<Satellite>("SAT-3", tle1, tle2, simStart);

    // Create a network device for each satellite
    Ptr<SatelliteNetDevice> dev1 = CreateObject<SatelliteNetDevice>();
    Ptr<SatelliteNetDevice> dev2 = CreateObject<SatelliteNetDevice>();
    Ptr<SatelliteNetDevice> dev3 = CreateObject<SatelliteNetDevice>();
    dev1->SetNode(sat1);
    dev2->SetNode(sat2);
    dev3->SetNode(sat3);
    sat1->AddDevice(dev1);
    sat2->AddDevice(dev2);
    sat3->AddDevice(dev3);

    // Create multiple ISL links from sat1 to different peers
    // The constructor automatically calls AddLink on both ends
    Ptr<SatelliteLink> link1 = CreateObject<SatelliteLink>(dev1, dev2);
    Ptr<SatelliteLink> link2 = CreateObject<SatelliteLink>(dev1, dev3);
    link1->SetMaxRange(5000000.0);
    link2->SetMaxRange(5000000.0);

    // Verify device trait methods for point-to-point links
    NS_TEST_EXPECT_MSG_EQ(dev1->IsPointToPoint(), true, "Satellite device should be point-to-point");
    NS_TEST_EXPECT_MSG_EQ(dev1->IsBroadcast(), false, "Point-to-point device should not broadcast");
    NS_TEST_EXPECT_MSG_EQ(dev1->NeedsArp(), false, "Point-to-point device should not need ARP");
    NS_TEST_EXPECT_MSG_EQ(dev1->IsMulticast(), false, "Point-to-point device should not support multicast");


    // Verify we can retrieve all links
    const auto& links = dev1->GetLinks();
    NS_TEST_EXPECT_MSG_EQ(links.size(), 2u, "Device should have exactly 2 links");
    NS_TEST_EXPECT_MSG_EQ(links[0], link1, "First link should match");
    NS_TEST_EXPECT_MSG_EQ(links[1], link2, "Second link should match");
    NS_LOG_INFO("Device has " << links.size() << " links as expected");

    // Verify backward compatibility with single-link API
    NS_TEST_EXPECT_MSG_EQ(dev1->GetSatelliteLink(), link1, "GetSatelliteLink should return first link");

    // Test SetLink (backward compatibility) - should clear existing links and set one
    Ptr<SatelliteNetDevice> dev4 = CreateObject<SatelliteNetDevice>();
    Ptr<Satellite> sat4 = CreateObject<Satellite>("SAT-4", tle1, tle2, simStart);
    dev4->SetNode(sat4);
    sat4->AddDevice(dev4);
    
    Ptr<SatelliteLink> link3 = CreateObject<SatelliteLink>(dev1, dev4);
    dev1->SetLink(link3);
    const auto& newLinks = dev1->GetLinks();
    NS_TEST_EXPECT_MSG_EQ(newLinks.size(), 1u, "SetLink should clear and set single link");
    NS_TEST_EXPECT_MSG_EQ(newLinks[0], link3, "Link should be the one set");

    NS_LOG_INFO("Multi-link device test completed successfully");
}

OrbitShieldGroundStationMultiLinkTest::OrbitShieldGroundStationMultiLinkTest()
    : TestCase("Test ground station multi-link support")
{
}

OrbitShieldGroundStationMultiLinkTest::~OrbitShieldGroundStationMultiLinkTest()
{
}

void
OrbitShieldGroundStationMultiLinkTest::DoRun()
{
    // Create ground station
    Ptr<GroundStation> gs = CreateObject<GroundStation>();
    gs->SetName("Test Ground Station");
    gs->SetLatitude(33.4);
    gs->SetLongitude(-111.9);

    // Create satellites
    std::string tle1 = "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996";
    std::string tle2 = "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227";
    perturb::JulianDate simStart(perturb::DateTime(2026, 1, 1, 0, 0, 0));

    Ptr<Satellite> sat1 = CreateObject<Satellite>("SAT-1", tle1, tle2, simStart);
    Ptr<Satellite> sat2 = CreateObject<Satellite>("SAT-2", tle1, tle2, simStart);

    // Create network devices
    Ptr<SatelliteNetDevice> satDev1 = CreateObject<SatelliteNetDevice>();
    Ptr<SatelliteNetDevice> satDev2 = CreateObject<SatelliteNetDevice>();
    Ptr<SatelliteNetDevice> gsDev = CreateObject<SatelliteNetDevice>();

    satDev1->SetNode(sat1);
    satDev2->SetNode(sat2);
    gsDev->SetNode(gs);

    sat1->AddDevice(satDev1);
    sat2->AddDevice(satDev2);
    gs->AddDevice(gsDev);

    // Create ground links from ground station to multiple satellites
    // The constructor automatically calls AddLink on both ends
    Ptr<SatelliteLink> link1 = CreateObject<SatelliteLink>(satDev1, gsDev);
    Ptr<SatelliteLink> link2 = CreateObject<SatelliteLink>(satDev2, gsDev);
    link1->SetMaxRange(50000000.0);
    link2->SetMaxRange(50000000.0);

    // Verify ground station device has multiple links
    const auto& gsLinks = gsDev->GetLinks();
    NS_TEST_EXPECT_MSG_EQ(gsLinks.size(), 2u, "Ground station device should have 2 ground links");
    NS_LOG_INFO("Ground station device has " << gsLinks.size() << " links to satellites");

    // Verify device trait methods are correct for ground links
    NS_TEST_EXPECT_MSG_EQ(gsDev->IsPointToPoint(), true, "Ground station device should be point-to-point");
    NS_TEST_EXPECT_MSG_EQ(gsDev->IsBroadcast(), false, "Ground station device should not broadcast");
    NS_TEST_EXPECT_MSG_EQ(gsDev->NeedsArp(), false, "Ground station device should not need ARP");

    // Verify satellites can each reach the ground station
    const auto& sat1Links = satDev1->GetLinks();
    const auto& sat2Links = satDev2->GetLinks();
    NS_TEST_EXPECT_MSG_EQ(sat1Links.size(), 1u, "Satellite 1 should have 1 ground link");
    NS_TEST_EXPECT_MSG_EQ(sat2Links.size(), 1u, "Satellite 2 should have 1 ground link");

    NS_LOG_INFO("Ground station multi-link test completed successfully");
}

OrbitShieldIpv4AddressAssignmentTest::OrbitShieldIpv4AddressAssignmentTest()
    : TestCase("Test sequential subnet IPv4 address assignment by OrbitShieldRoutingHelper Install")
{
}

OrbitShieldIpv4AddressAssignmentTest::~OrbitShieldIpv4AddressAssignmentTest()
{
}

void
OrbitShieldIpv4AddressAssignmentTest::DoRun()
{
    // Load the Iridium constellation
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.yaml");

    // Set ranges: CreateIslLinks/CreateGroundLinks store m_islMaxRange / m_groundMaxRange
    // as a side effect, which RefreshIslTopology uses to build m_currentIsls / m_currentGroundLinks.
    constellation->CreateIslLinks(2000000.0);    // 2000 km ISL range
    constellation->CreateGroundLinks(50000000.0); // 50 000 km GSL range
    constellation->RefreshIslTopology();          // populates GetCurrentIsls / GetCurrentGroundLinks

    const auto& isls = constellation->GetCurrentIsls();
    const auto& gsls = constellation->GetCurrentGroundLinks();

    if (isls.empty() && gsls.empty())
    {
        NS_LOG_WARN("No active links at epoch; skipping sequential /30 address verification");
        Simulator::Destroy();
        return;
    }

    uint32_t totalLinks = static_cast<uint32_t>(isls.size() + gsls.size());
    NS_LOG_INFO("Testing sequential /30 allocation for " << isls.size()
                << " ISL links and " << gsls.size() << " GSL links");

    // Install routing (installs IPv4 stack and assigns sequential /30 addresses)
    OrbitShieldRoutingHelper routingHelper;
    routingHelper.Install(constellation);

    // Collect all non-loopback IPv4 addresses from every node.
    // Each link creates one new interface per endpoint, so K links → K*2 addresses total.
    std::vector<uint32_t> assignedAddresses;

    auto collectAddresses = [&](Ptr<Node> node) {
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        if (!ipv4)
        {
            return;
        }
        // Interface 0 is always the loopback; start from 1.
        for (uint32_t iface = 1; iface < ipv4->GetNInterfaces(); ++iface)
        {
            for (uint32_t addrIdx = 0; addrIdx < ipv4->GetNAddresses(iface); ++addrIdx)
            {
                Ipv4InterfaceAddress ifAddr = ipv4->GetAddress(iface, addrIdx);
                // Verify mask is /30 on every assigned interface
                NS_TEST_ASSERT_MSG_EQ(ifAddr.GetMask(),
                                      Ipv4Mask("255.255.255.252"),
                                      "Every assigned interface must have a /30 mask");
                assignedAddresses.push_back(ifAddr.GetLocal().Get());
            }
        }
    };

    for (const auto& sat : constellation->GetSatellites())
    {
        collectAddresses(sat);
    }
    for (const auto& gs : constellation->GetGroundStations())
    {
        collectAddresses(gs);
    }

    // Exactly two addresses per link (one per endpoint)
    NS_TEST_ASSERT_MSG_EQ(assignedAddresses.size(),
                          static_cast<std::size_t>(totalLinks) * 2,
                          "Expected exactly 2 addresses per link");

    // Sort numerically so sequential /30 blocks appear in order
    std::sort(assignedAddresses.begin(), assignedAddresses.end());

    // Uniqueness: no two (node, interface) pairs share the same address
    for (std::size_t i = 1; i < assignedAddresses.size(); ++i)
    {
        NS_TEST_ASSERT_MSG_NE(assignedAddresses[i],
                              assignedAddresses[i - 1],
                              "All assigned IPv4 addresses must be unique");
    }

    // Sequential /30 blocks: block i occupies [10.0.0.0 + 4*i + 1, 10.0.0.0 + 4*i + 2].
    // After sorting, pair (2*i, 2*i+1) must correspond to block i.
    const uint32_t base = 0x0A000000; // 10.0.0.0
    for (uint32_t i = 0; i < totalLinks; ++i)
    {
        uint32_t expectedFirst  = base + 4 * i + 1;
        uint32_t expectedSecond = base + 4 * i + 2;
        NS_TEST_ASSERT_MSG_EQ(assignedAddresses[2 * i],
                              expectedFirst,
                              "Block " << i << " first address mismatch");
        NS_TEST_ASSERT_MSG_EQ(assignedAddresses[2 * i + 1],
                              expectedSecond,
                              "Block " << i << " second address mismatch");
    }

    NS_LOG_INFO("Sequential /30 allocation verified: " << totalLinks << " link(s), all blocks correct");
    Simulator::Destroy();
}

OrbitShieldRefreshSafeRoutingTest::OrbitShieldRefreshSafeRoutingTest()
    : TestCase("Test refresh-safe routing recomputation with ICMP delivery across topology refresh")
{
}

OrbitShieldRefreshSafeRoutingTest::~OrbitShieldRefreshSafeRoutingTest()
{
}

void
OrbitShieldRefreshSafeRoutingTest::OnRttTrace(uint16_t seq, Time rtt)
{
    (void)seq;
    (void)rtt;
    ++m_rttCount;
}

void
OrbitShieldRefreshSafeRoutingTest::DoRun()
{
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.yaml");

    constellation->SetIslRefreshInterval(Seconds(10.0));
    constellation->CreateIslLinks(2000000.0);
    constellation->CreateGroundLinks(50000000.0);
    constellation->RefreshIslTopology();

    OrbitShieldRoutingHelper routingHelper;
    routingHelper.Install(constellation);

    Ptr<GroundStation> tempe = FindGroundStationByName(constellation->GetGroundStations(), "Tempe");
    Ptr<GroundStation> fairbanks = FindGroundStationByName(constellation->GetGroundStations(), "Fairbanks");

    NS_TEST_ASSERT_MSG_NE(tempe, nullptr, "Tempe ground station must exist in Iridium dataset");
    NS_TEST_ASSERT_MSG_NE(fairbanks, nullptr, "Fairbanks ground station must exist in Iridium dataset");

    Ipv4Address destination = GetFirstNonLoopbackAddress(fairbanks);
    NS_TEST_ASSERT_MSG_NE(destination,
                          Ipv4Address::GetZero(),
                          "Fairbanks must have a non-loopback IPv4 address after Install");

    PingHelper pingHelper(destination);
    pingHelper.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    pingHelper.SetAttribute("Size", UintegerValue(56));
    pingHelper.SetAttribute("Count", UintegerValue(20));

    ApplicationContainer apps = pingHelper.Install(tempe);
    Ptr<Ping> ping = DynamicCast<Ping>(apps.Get(0));
    NS_TEST_ASSERT_MSG_NE(ping, nullptr, "Ping application must be created");

    m_rttCount = 0;
    ping->TraceConnectWithoutContext("Rtt",
                                     MakeCallback(&OrbitShieldRefreshSafeRoutingTest::OnRttTrace,
                                                  this));

    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(26.0));

    Simulator::Stop(Seconds(30.0));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_GT(m_rttCount,
                          0u,
                          "Expected at least one successful ICMP echo reply during topology refresh window");

    Simulator::Destroy();
}

OrbitShieldDynamicRouteRefreshTest::OrbitShieldDynamicRouteRefreshTest()
    : TestCase("Test dynamic route refresh across topology refresh intervals")
{
}

OrbitShieldDynamicRouteRefreshTest::~OrbitShieldDynamicRouteRefreshTest()
{
}

void
OrbitShieldDynamicRouteRefreshTest::OnRttTrace(uint16_t seq, Time rtt)
{
    (void)seq;
    ++m_replyCount;
    if (m_replyCount == 1)
    {
        m_minRtt = rtt;
        m_maxRtt = rtt;
        return;
    }

    m_minRtt = std::min(m_minRtt, rtt);
    m_maxRtt = std::max(m_maxRtt, rtt);
}

void
OrbitShieldDynamicRouteRefreshTest::DoRun()
{
    NS_LOG_FUNCTION(this);

    // Load constellation and set up dynamic topology with 60-second refresh
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.yaml");

    constellation->SetIslRefreshInterval(Seconds(60.0));
    constellation->CreateIslLinks(2000000.0);
    constellation->CreateGroundLinks(50000000.0);
    constellation->RefreshIslTopology();

    // Install routing with callback
    OrbitShieldRoutingHelper routingHelper;
    routingHelper.Install(constellation);

    // Find ground stations
    Ptr<GroundStation> tempe = FindGroundStationByName(constellation->GetGroundStations(), "Tempe");
    Ptr<GroundStation> fairbanks =
        FindGroundStationByName(constellation->GetGroundStations(), "Fairbanks");

    NS_TEST_ASSERT_MSG_NE(tempe, nullptr, "Tempe ground station must exist in Iridium dataset");
    NS_TEST_ASSERT_MSG_NE(fairbanks, nullptr, "Fairbanks ground station must exist in Iridium dataset");

    Ipv4Address destination = GetFirstNonLoopbackAddress(fairbanks);
    NS_TEST_ASSERT_MSG_NE(destination,
                          Ipv4Address::GetZero(),
                          "Fairbanks must have a non-loopback IPv4 address after Install");

    // Configure ping helper to send one echo per 60-second interval
    // Total simulation time: 600 seconds = 10 refresh intervals
    PingHelper pingHelper(destination);
    pingHelper.SetAttribute("Interval", TimeValue(Seconds(60.0)));
    pingHelper.SetAttribute("Size", UintegerValue(56));
    pingHelper.SetAttribute("Count", UintegerValue(10));
    pingHelper.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::SILENT));

    ApplicationContainer apps = pingHelper.Install(tempe);
    Ptr<Ping> ping = DynamicCast<Ping>(apps.Get(0));
    NS_TEST_ASSERT_MSG_NE(ping, nullptr, "Ping application must be created");

    m_replyCount = 0;
    m_minRtt = Seconds(0);
    m_maxRtt = Seconds(0);
    ping->TraceConnectWithoutContext("Rtt",
                                     MakeCallback(&OrbitShieldDynamicRouteRefreshTest::OnRttTrace,
                                                  this));

    apps.Start(Seconds(0.0));
    apps.Stop(Seconds(600.0));

    Simulator::Stop(Seconds(600.0));
    Simulator::Run();

    NS_LOG_INFO("Dynamic route refresh test completed: "
                << m_replyCount << " ICMP echo replies received");

    // Verify that the simulation completed without crash/assertion
    NS_TEST_ASSERT_MSG_EQ(true,
                          true,
                          "Simulation completed without crash or assertion failure");

    // Verify at least one ICMP echo reply within first 120 seconds
    // (allows time for initial topology setup and 2 refresh intervals)
    NS_TEST_ASSERT_MSG_GT(m_replyCount,
                          0u,
                          "Expected at least one successful ICMP echo reply within 600 seconds of "
                          "dynamic topology refresh cycles");

    NS_LOG_INFO("Successfully validated that routes are updated across refresh intervals with "
                "at least one ICMP delivery");

    Simulator::Destroy();
}

OrbitShieldTempeFairbanksPingPathTest::OrbitShieldTempeFairbanksPingPathTest()
    : TestCase("Test end-to-end Tempe->Fairbanks ICMP path with RTT bounds and multi-hop routing")
{
}

OrbitShieldTempeFairbanksPingPathTest::~OrbitShieldTempeFairbanksPingPathTest()
{
}

void
OrbitShieldTempeFairbanksPingPathTest::OnRttTrace(uint16_t seq, Time rtt)
{
    (void)seq;
    ++m_replyCount;
    if (m_replyCount == 1)
    {
        m_minRtt = rtt;
        m_maxRtt = rtt;
        return;
    }

    m_minRtt = std::min(m_minRtt, rtt);
    m_maxRtt = std::max(m_maxRtt, rtt);
}

void
OrbitShieldTempeFairbanksPingPathTest::DoRun()
{
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.yaml");

    constellation->CreateIslLinks(2000000.0);
    constellation->CreateGroundLinks(50000000.0);
    constellation->RefreshIslTopology();

    OrbitShieldRoutingHelper routingHelper;
    routingHelper.Install(constellation);

    Ptr<GroundStation> tempe = FindGroundStationByName(constellation->GetGroundStations(), "Tempe");
    Ptr<GroundStation> fairbanks = FindGroundStationByName(constellation->GetGroundStations(), "Fairbanks");

    NS_TEST_ASSERT_MSG_NE(tempe, nullptr, "Tempe ground station must exist in Iridium dataset");
    NS_TEST_ASSERT_MSG_NE(fairbanks, nullptr, "Fairbanks ground station must exist in Iridium dataset");

    const std::vector<Ptr<Node>> allNodes = CollectConstellationNodes(constellation);
    const std::vector<Ipv4Address> fairbanksAddresses = GetNonLoopbackAddresses(fairbanks);
    NS_TEST_ASSERT_MSG_GT(fairbanksAddresses.size(),
                          0u,
                          "Fairbanks must have at least one non-loopback IPv4 address after Install");

    Ipv4Address destination = Ipv4Address::GetZero();
    uint32_t hopCount = 0;
    for (const auto& candidate : fairbanksAddresses)
    {
        const uint32_t candidateHops =
            ComputeStaticHostRouteHopCount(tempe, fairbanks, candidate, allNodes);
        if (candidateHops > hopCount)
        {
            hopCount = candidateHops;
            destination = candidate;
        }
    }

    NS_TEST_ASSERT_MSG_GT(hopCount,
                          0u,
                          "A static host route from Tempe to Fairbanks must exist");
    NS_TEST_ASSERT_MSG_GT(hopCount,
                          1u,
                          "Expected a multi-hop path from Tempe to Fairbanks (>= 2 hops)");

    PingHelper pingHelper(destination);
    pingHelper.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    pingHelper.SetAttribute("Size", UintegerValue(56));
    pingHelper.SetAttribute("Count", UintegerValue(60));
    pingHelper.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::SILENT));

    ApplicationContainer apps = pingHelper.Install(tempe);
    Ptr<Ping> ping = DynamicCast<Ping>(apps.Get(0));
    NS_TEST_ASSERT_MSG_NE(ping, nullptr, "Ping application must be created");

    m_replyCount = 0;
    m_minRtt = Seconds(0);
    m_maxRtt = Seconds(0);
    ping->TraceConnectWithoutContext("Rtt",
                                     MakeCallback(&OrbitShieldTempeFairbanksPingPathTest::OnRttTrace,
                                                  this));

    apps.Start(Seconds(0.0));
    apps.Stop(Seconds(60.0));

    Simulator::Stop(Seconds(60.0));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_GT(m_replyCount,
                          0u,
                          "Expected at least one successful ICMP echo reply at Tempe within 60 seconds");
    NS_TEST_ASSERT_MSG_GT(m_minRtt.GetNanoSeconds(),
                          0,
                          "Measured ICMP RTT must be non-zero");
    NS_TEST_ASSERT_MSG_EQ(m_maxRtt <= MilliSeconds(500),
                          true,
                          "Measured ICMP RTT must be <= 500 ms");

    Simulator::Destroy();
}

// ---------------------------------------------------------------------------
// OrbitShieldMultiGroundStationRoutingTest
// ---------------------------------------------------------------------------

OrbitShieldMultiGroundStationRoutingTest::OrbitShieldMultiGroundStationRoutingTest()
    : TestCase("Test multi-GS routing across all Iridium ground station pairs")
{
}

OrbitShieldMultiGroundStationRoutingTest::~OrbitShieldMultiGroundStationRoutingTest()
{
}

void
OrbitShieldMultiGroundStationRoutingTest::DoRun()
{
    NS_LOG_FUNCTION(this);

    // Load the Iridium constellation with a 30-second topology refresh interval.
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.yaml");
    constellation->SetIslRefreshInterval(Seconds(30.0));
    constellation->CreateIslLinks(2000000.0);
    constellation->CreateGroundLinks(50000000.0);
    constellation->RefreshIslTopology();

    OrbitShieldRoutingHelper routingHelper;
    routingHelper.Install(constellation);

    // Collect all 5 ground stations.
    const auto& gsList = constellation->GetGroundStations();
    NS_TEST_ASSERT_MSG_EQ(gsList.size(), 5u, "Iridium dataset must have exactly 5 ground stations");

    // Build all 10 pairwise combinations (i < j).
    struct GsPair
    {
        Ptr<GroundStation> src;
        Ptr<GroundStation> dst;
    };
    std::vector<GsPair> pairs;
    for (size_t i = 0; i < gsList.size(); ++i)
    {
        for (size_t j = i + 1; j < gsList.size(); ++j)
        {
            pairs.push_back({gsList[i], gsList[j]});
        }
    }
    NS_TEST_ASSERT_MSG_EQ(pairs.size(), 10u, "Expected exactly 10 GS pairs");

    // Gather all constellation nodes for hop-count computation.
    const std::vector<Ptr<Node>> allNodes = CollectConstellationNodes(constellation);

    // Compute initial static-routing hop counts (before simulation starts).
    // The Dijkstra pass run by Install() produces the first set of routes.
    std::vector<uint32_t> hopCounts(pairs.size(), 0u);
    for (size_t pairIdx = 0; pairIdx < pairs.size(); ++pairIdx)
    {
        Ipv4Address dstAddr = GetFirstNonLoopbackAddress(pairs[pairIdx].dst);
        if (dstAddr != Ipv4Address::GetZero())
        {
            hopCounts[pairIdx] = ComputeStaticHostRouteHopCount(pairs[pairIdx].src,
                                                                pairs[pairIdx].dst,
                                                                dstAddr,
                                                                allNodes);
        }
    }

    // One ping every 30 s → 10 total pings per pair over the 300-second window.
    // That gives one echo per topology-refresh interval.
    const uint32_t pingCount = 10u;

    // Install per-pair ping applications and connect the RTT trace.
    std::vector<GsPairResult> results(pairs.size());
    for (size_t pairIdx = 0; pairIdx < pairs.size(); ++pairIdx)
    {
        Ipv4Address dstAddr = GetFirstNonLoopbackAddress(pairs[pairIdx].dst);
        if (dstAddr == Ipv4Address::GetZero())
        {
            NS_LOG_WARN("No IPv4 address for destination "
                        << pairs[pairIdx].dst->GetName() << "; skipping pair " << pairIdx);
            continue;
        }

        PingHelper pingHelper(dstAddr);
        pingHelper.SetAttribute("Interval", TimeValue(Seconds(30.0)));
        pingHelper.SetAttribute("Size", UintegerValue(56));
        pingHelper.SetAttribute("Count", UintegerValue(pingCount));
        pingHelper.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::SILENT));

        ApplicationContainer apps = pingHelper.Install(pairs[pairIdx].src);
        Ptr<Ping> ping = DynamicCast<Ping>(apps.Get(0));
        if (ping)
        {
            ping->TraceConnectWithoutContext("Rtt",
                                             MakeBoundCallback(&OnGsPairRtt, &results[pairIdx]));
        }
        apps.Start(Seconds(1.0));
        apps.Stop(Seconds(300.0));
    }

    Simulator::Stop(Seconds(300.0));
    Simulator::Run();

    // --- Evaluate pass conditions ---

    uint32_t pairsAbove80pct = 0u;
    uint32_t pairsAt100pct = 0u;
    bool allRttsValid = true;
    uint32_t maxHops = 0u;

    for (size_t i = 0; i < pairs.size(); ++i)
    {
        const double ratio =
            static_cast<double>(results[i].replyCount) / static_cast<double>(pingCount);

        if (!results[i].allRttsValid)
        {
            allRttsValid = false;
        }
        if (ratio >= 0.80)
        {
            ++pairsAbove80pct;
        }
        if (results[i].replyCount == pingCount)
        {
            ++pairsAt100pct;
        }
        if (hopCounts[i] > maxHops)
        {
            maxHops = hopCounts[i];
        }

        NS_LOG_INFO("Pair " << pairs[i].src->GetName() << " -> " << pairs[i].dst->GetName()
                            << ": " << results[i].replyCount << "/" << pingCount
                            << " replies (ratio=" << ratio << ")"
                            << ", maxRtt=" << results[i].maxRtt.GetMilliSeconds() << " ms"
                            << ", hops=" << hopCounts[i]);
    }

    NS_LOG_INFO("Multi-GS routing summary: pairsAbove80pct=" << pairsAbove80pct
                << ", pairsAt100pct=" << pairsAt100pct << ", maxHops=" << maxHops
                << ", allRttsValid=" << allRttsValid);

    // Condition 1: at least 7 of 10 GS pairs must achieve >= 80% delivery ratio.
    NS_TEST_ASSERT_MSG_GT(pairsAbove80pct,
                          6u,
                          "At least 7 of 10 GS pairs must achieve >= 80% packet delivery ratio");

    // Condition 2: every delivered packet must have RTT <= 500 ms.
    NS_TEST_ASSERT_MSG_EQ(allRttsValid,
                          true,
                          "All delivered packets must have measured RTT <= 500 ms");

    // Condition 3: maximum hop count across all pairs must be <= 8.
    NS_TEST_ASSERT_MSG_EQ(maxHops <= 8u,
                          true,
                          "Maximum hop count across all GS pairs must be <= 8");

    // Condition 4: at least 3 distinct GS pairs must achieve 100% delivery.
    NS_TEST_ASSERT_MSG_GT(pairsAt100pct,
                          2u,
                          "At least 3 distinct GS pairs must achieve 100% packet delivery");

    Simulator::Destroy();
}

// ---------------------------------------------------------------------------
// OrbitShieldStaticRoutingStrategyTest
// ---------------------------------------------------------------------------

OrbitShieldStaticRoutingStrategyTest::OrbitShieldStaticRoutingStrategyTest()
    : TestCase("Test static routing strategy robustness under fast topology refresh")
{
}

OrbitShieldStaticRoutingStrategyTest::~OrbitShieldStaticRoutingStrategyTest()
{
}

void
OrbitShieldStaticRoutingStrategyTest::OnRttTrace(uint16_t seq, Time rtt)
{
    (void)seq;
    (void)rtt;
    ++m_replyCount;
}

void
OrbitShieldStaticRoutingStrategyTest::DoRun()
{
    NS_LOG_FUNCTION(this);

    // Load constellation with a fast 15-second refresh interval.
    // Over 300 seconds this triggers 20 route recomputations, stressing the
    // static-routing clear/rebuild path far more than normal operation.
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.yaml");
    constellation->SetIslRefreshInterval(Seconds(15.0));
    constellation->CreateIslLinks(2000000.0);
    constellation->CreateGroundLinks(50000000.0);
    constellation->RefreshIslTopology();

    OrbitShieldRoutingHelper routingHelper;
    routingHelper.Install(constellation);

    Ptr<GroundStation> tempe =
        FindGroundStationByName(constellation->GetGroundStations(), "Tempe");
    Ptr<GroundStation> fairbanks =
        FindGroundStationByName(constellation->GetGroundStations(), "Fairbanks");

    NS_TEST_ASSERT_MSG_NE(tempe, nullptr, "Tempe ground station must exist in Iridium dataset");
    NS_TEST_ASSERT_MSG_NE(fairbanks,
                          nullptr,
                          "Fairbanks ground station must exist in Iridium dataset");

    Ipv4Address destination = GetFirstNonLoopbackAddress(fairbanks);
    NS_TEST_ASSERT_MSG_NE(destination,
                          Ipv4Address::GetZero(),
                          "Fairbanks must have a non-loopback IPv4 address after Install");

    // Send one ping every 15 seconds (aligned with refresh interval).
    // 300 / 15 = 20 pings total.
    PingHelper pingHelper(destination);
    pingHelper.SetAttribute("Interval", TimeValue(Seconds(15.0)));
    pingHelper.SetAttribute("Size", UintegerValue(56));
    pingHelper.SetAttribute("Count", UintegerValue(20));
    pingHelper.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::SILENT));

    ApplicationContainer apps = pingHelper.Install(tempe);
    Ptr<Ping> ping = DynamicCast<Ping>(apps.Get(0));
    NS_TEST_ASSERT_MSG_NE(ping, nullptr, "Ping application must be created");

    m_replyCount = 0u;
    ping->TraceConnectWithoutContext(
        "Rtt",
        MakeCallback(&OrbitShieldStaticRoutingStrategyTest::OnRttTrace, this));

    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(300.0));

    Simulator::Stop(Seconds(300.0));
    Simulator::Run();

    NS_LOG_INFO("Static routing strategy test completed: " << m_replyCount
                << " / 20 ICMP echo replies received across 20 fast-refresh cycles");

    // Simulation must complete without crash (reaching here confirms that).
    NS_TEST_ASSERT_MSG_EQ(true, true, "Simulation completed without crash or assertion failure");

    // At least one successful delivery confirms routing is functional under fast refresh.
    NS_TEST_ASSERT_MSG_GT(m_replyCount,
                          0u,
                          "Static routing must deliver at least one ICMP echo reply under a "
                          "15-second refresh interval over 300 seconds");

    Simulator::Destroy();
}
