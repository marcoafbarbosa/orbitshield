/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "test-satellite.h"
#include "ns3/orbitshield-module.h"
#include "ns3/test.h"

using namespace ns3;

SatelliteTestCase::SatelliteTestCase()
    : TestCase("Test Satellite basic functionality")
{
}

SatelliteTestCase::~SatelliteTestCase()
{
}

void
SatelliteTestCase::DoRun()
{
    // Let try simulating the orbit of the International Space Station
    // Got TLE from Celestrak sometime around 2022-03-12
    std::string ISS_NAME = "ISS (ZARYA)";
    std::string ISS_TLE_1 = "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996";
    std::string ISS_TLE_2 = "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227";

    // Get the TLE epoch as simulation start time
    perturb::Satellite tempSat = perturb::Satellite::from_tle(ISS_TLE_1, ISS_TLE_2);
    perturb::JulianDate simulationStartJD = tempSat.epoch();

    Ptr<Satellite> satellite = CreateObject<Satellite>(ISS_NAME, ISS_TLE_1, ISS_TLE_2, simulationStartJD);

    // Test satellite name
    NS_TEST_EXPECT_MSG_EQ(satellite->GetName(), ISS_NAME, "Satellite name should match");

    // Test time-aware position/velocity
    Ptr<SatelliteMobilityModel> mobility = CreateObject<SatelliteMobilityModel>();
    mobility->SetSatellite(satellite);
    satellite->AggregateObject(mobility);

    Vector pos0 = mobility->GetPosition();
    // Evaluate at an explicit time point instead of changing simulator time directly.
    Vector pos10 = satellite->GetPosition(Seconds(10.0));

    NS_TEST_EXPECT_MSG_NE(pos0, pos10, "Position should change after 10 seconds");
    Vector vel = mobility->GetVelocity();
    NS_TEST_EXPECT_MSG_NE(vel, Vector(0.0, 0.0, 0.0), "Velocity should be non-zero in LEO");
}
