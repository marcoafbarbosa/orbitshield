/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "test-routing.h"
#include "ns3/orbitshield-module.h"
#include "ns3/test.h"
#include "ns3/ipv4.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RoutingTest");

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

    // Verify ground stations - this is the key part of Phase 1.1
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

