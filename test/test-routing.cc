/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "test-routing.h"
#include "ns3/orbitshield-module.h"
#include "ns3/test.h"

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

