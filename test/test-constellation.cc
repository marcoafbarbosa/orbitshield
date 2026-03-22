/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "test-constellation.h"
#include "ns3/orbitshield-module.h"
#include "ns3/test.h"

#include <cstdio>
#include <fstream>

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
    TestIridium();
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

    // Clean up the temporary file
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