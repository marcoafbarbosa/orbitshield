/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "test-constellation.h"
#include "ns3/orbitshield-module.h"
#include "ns3/test.h"

#include <cstdio>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ConstellationTest");

/**
 * \brief Test suite for Constellation class
 */

ConstellationTestCase::ConstellationTestCase()
    : TestCase("Test Constellation loading from TLE")
{
}

ConstellationTestCase::~ConstellationTestCase()
{
}

void
ConstellationTestCase::DoRun()
{
    TestSimple();
    TestIslFallbackWithoutRings();
    TestIridium();
    TestIslRefreshHonorsRange();
}

void
ConstellationTestCase::TestSimple()
{
    const char* filename = "constellation-test.tle";

    // Write a minimal TLE file with two satellites
    {
        std::ofstream file(filename);
        file << "0 SAT-1\n";
        file << "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996\n";
        file << "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227\n";
        file << "0 SAT-2\n";
        file << "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996\n";
        file << "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227\n";
    }

    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromTleFile(filename);

    const auto& sats = constellation->GetSatellites();
    NS_TEST_EXPECT_MSG_EQ(sats.size(), 2u, "Expected 2 satellites to be loaded");
    NS_TEST_EXPECT_MSG_EQ(sats[0]->GetName(), std::string("SAT-1"), "First satellite name should match");
    NS_TEST_EXPECT_MSG_EQ(sats[1]->GetName(), std::string("SAT-2"), "Second satellite name should match");

    const char* ringFilename = "constellation-test.rings";
    {
        std::ofstream ringFile(ringFilename);
        ringFile << "constellationName=TestConstellation\n";
        ringFile << "ringCount=2\n";
        ringFile << "ring.0=SAT-1\n";
        ringFile << "ring.1=SAT-2\n";
    }

    constellation->LoadFromRingFile(ringFilename);
    NS_TEST_EXPECT_MSG_EQ(constellation->GetRingCount(), 2u, "Ring count should be 2");

    const auto& ring0 = constellation->GetSatellitesInRing(0);
    const auto& ring1 = constellation->GetSatellitesInRing(1);
    NS_TEST_EXPECT_MSG_EQ(ring0.size(), 1u, "Ring 0 should contain 1 satellite");
    NS_TEST_EXPECT_MSG_EQ(ring1.size(), 1u, "Ring 1 should contain 1 satellite");
    NS_TEST_EXPECT_MSG_EQ(ring0[0]->GetName(), std::string("SAT-1"), "Ring 0 should have SAT-1");
    NS_TEST_EXPECT_MSG_EQ(ring1[0]->GetName(), std::string("SAT-2"), "Ring 1 should have SAT-2");

    const auto& next0 = constellation->GetNextRingSatellites(0);
    const auto& prev0 = constellation->GetPreviousRingSatellites(0);
    NS_TEST_EXPECT_MSG_EQ(next0.size(), 1u, "Next ring from 0 should have 1 satellite");
    NS_TEST_EXPECT_MSG_EQ(prev0.size(), 1u, "Previous ring from 0 should have 1 satellite");
    NS_TEST_EXPECT_MSG_EQ(next0[0]->GetName(), std::string("SAT-2"), "Next ring from 0 should be SAT-2");
    NS_TEST_EXPECT_MSG_EQ(prev0[0]->GetName(), std::string("SAT-2"), "Previous ring from 0 should be SAT-2 (wrap-around)");

    std::remove(ringFilename);
    std::remove(filename);
}

void
ConstellationTestCase::TestIslFallbackWithoutRings()
{
    const char* filename = "constellation-fallback-test.tle";

    {
        std::ofstream file(filename);
        file << "0 SAT-A\n";
        file << "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996\n";
        file << "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227\n";
        file << "0 SAT-B\n";
        file << "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996\n";
        file << "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227\n";
    }

    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromTleFile(filename);

    const auto& sats = constellation->GetSatellites();
    NS_TEST_EXPECT_MSG_EQ(sats.size(), 2u, "Expected 2 satellites loaded for fallback test");

    std::vector<Ptr<SatelliteLink>> links = constellation->CreateIslLinks(2000000.0);
    NS_TEST_EXPECT_MSG_GT(links.size(), 0u, "Expected at least one ISL link to be created without ring data");

    std::string dot = constellation->ExportIslAsDot(links, true);
    NS_TEST_EXPECT_MSG_NE(dot.find("pos="), std::string::npos, "DOT output should include node positions for fallback ISL links");

    std::remove(filename);
}

void
ConstellationTestCase::TestIridium()
{
    // Create a constellation and load satellites from the TLE file
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromTleFile("contrib/orbitshield/data/iridium-20260312.txt");

    const auto& satellites = constellation->GetSatellites();
    NS_TEST_EXPECT_MSG_GT(satellites.size(), 0u, "Expected at least one satellite to be loaded");
    
    auto expectedStartJD = satellites[0]->GetSimulationStartJD();
    for (const auto& sat : satellites)    {
        // ensure all satellites have the same simulation start time
        auto satStartJD = sat->GetSimulationStartJD();
        bool equal = (satStartJD >= expectedStartJD) && (satStartJD <= expectedStartJD);
        NS_TEST_EXPECT_MSG_EQ(equal, true, "All satellites should have the same simulation start time");

        // ensure simulation start time is equal or greater than the TLE epoch time
        auto tleEpochTime = sat->GetTleEpochAsJulianDate();
        bool validEpoch = (satStartJD >= tleEpochTime);
        NS_TEST_EXPECT_MSG_EQ(validEpoch, true, "Simulation start time should be equal or greater than TLE epoch time");

        // ensure satellite positions can be queried without error
        Vector3D pos = sat->GetPosition();
        NS_LOG_INFO("Satellite " << sat->GetName() << " initial position: " << pos);
    }


}

void
ConstellationTestCase::TestIslRefreshHonorsRange()
{
    Simulator::Destroy();

    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.rings");

    const double maxRange = 3000000.0; // 3000 km
    const Time step = Seconds(60);
    const uint32_t steps = 180; // 3 hours

    // SetIslRefreshInterval must be called before CreateIslLinks so that
    // the first topology-refresh event is scheduled with the right interval.
    constellation->SetIslRefreshInterval(step);

    // Seed cached range used by refresh logic; also schedules the first refresh event.
    auto initialLinks = constellation->CreateIslLinks(maxRange);
    NS_TEST_ASSERT_MSG_GT(initialLinks.size(), 0u, "Expected non-empty initial ISL topology for iridium dataset");

    bool sawTopologyChange = false;
    bool sawRangeViolation = false;
    std::set<std::string> previousTopology;

    for (uint32_t i = 0; i < steps; ++i)
    {
        // Advance the simulator one step; the constellation refreshes topology automatically.
        Simulator::Stop(step);
        Simulator::Run();

        Time now = Simulator::Now();
        const auto& links = constellation->GetCurrentIsls();

        std::set<std::string> currentTopology;

        for (const auto& link : links)
        {
            NS_TEST_ASSERT_MSG_NE(link, nullptr, "ISL list must not contain null links");

            auto dev0 = DynamicCast<SatelliteNetDevice>(link->GetDevice(0));
            auto dev1 = DynamicCast<SatelliteNetDevice>(link->GetDevice(1));
            NS_TEST_ASSERT_MSG_NE(dev0, nullptr, "Link endpoint 0 must be a SatelliteNetDevice");
            NS_TEST_ASSERT_MSG_NE(dev1, nullptr, "Link endpoint 1 must be a SatelliteNetDevice");

            auto sat0 = DynamicCast<Satellite>(dev0->GetNode());
            auto sat1 = DynamicCast<Satellite>(dev1->GetNode());
            NS_TEST_ASSERT_MSG_NE(sat0, nullptr, "Link endpoint 0 node must be a Satellite");
            NS_TEST_ASSERT_MSG_NE(sat1, nullptr, "Link endpoint 1 node must be a Satellite");

            const std::string name0 = sat0->GetName();
            const std::string name1 = sat1->GetName();
            const std::string key = (name0 < name1) ? (name0 + "|" + name1) : (name1 + "|" + name0);
            currentTopology.insert(key);

            Vector3D p0 = sat0->GetPosition(now);
            Vector3D p1 = sat1->GetPosition(now);
            const double dx = p0.x - p1.x;
            const double dy = p0.y - p1.y;
            const double dz = p0.z - p1.z;
            const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (distance > maxRange)
            {
                sawRangeViolation = true;
                std::ostringstream oss;
                oss << "ISL pair " << key << " violates maxRange at t=" << now.GetSeconds()
                    << "s: distance=" << distance << " m, maxRange=" << maxRange << " m";
                NS_TEST_EXPECT_MSG_EQ(false, true, oss.str());
            }
        }

        if (i > 0 && currentTopology != previousTopology)
        {
            sawTopologyChange = true;
        }
        previousTopology = std::move(currentTopology);
    }

    NS_TEST_EXPECT_MSG_EQ(sawRangeViolation,
                          false,
                          "Refreshed ISL topology must keep all links within maxRange at each step");
    NS_TEST_EXPECT_MSG_EQ(sawTopologyChange,
                          true,
                          "Topology should change over time when refresh is enabled");

    Simulator::Destroy();
}