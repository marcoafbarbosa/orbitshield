/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "test-satellite.h"
#include "ns3/core-module.h"
#include "ns3/orbitshield-module.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SatelliteTest");

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
    TestSimple();
    TestWithConstellation();
}


void
SatelliteTestCase::TestSimple()
{
    // Let try simulating the orbit of the International Space Station
    // Got TLE from Celestrak sometime around 2022-03-12
    std::string ISS_NAME = "ISS (ZARYA)";
    std::string ISS_TLE_1 = "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996";
    std::string ISS_TLE_2 = "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227";

    // Get the TLE epoch as simulation start time
    // Get the TLE epoch as simulation start time.
    // IMPORTANT: perturb::Satellite::from_tle() mutates its string arguments in-place
    // (sgp4::twoline2rv inserts decimal points and replaces spaces).  Use separate copies
    // for the epoch lookup so the Satellite constructor receives the original, unmutated strings.
    std::string tle1_for_epoch = ISS_TLE_1;
    std::string tle2_for_epoch = ISS_TLE_2;
    perturb::Satellite tempSat = perturb::Satellite::from_tle(tle1_for_epoch, tle2_for_epoch);
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

    auto gt = satellite->GetGroundTrackPosition();
    NS_TEST_EXPECT_MSG_GT(gt.latitude, -90.0, "Latitude should be >= -90");
    NS_TEST_EXPECT_MSG_LT(gt.latitude, 90.0, "Latitude should be <= 90");
    NS_TEST_EXPECT_MSG_GT(gt.longitude, -180.0, "Longitude should be >= -180");
    NS_TEST_EXPECT_MSG_LT(gt.longitude, 180.0, "Longitude should be < 180");

    NS_LOG_INFO("Ground track lat/lon: " << gt.latitude << ", " << gt.longitude << " alt=" << gt.altitude);
}

void
SatelliteTestCase::TestWithConstellation()
{
    // Create a constellation and load satellites from the TLE file
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromTleFile("contrib/orbitshield/data/iridium-20260312.txt");

    // Get the satellites from the constellation
    const auto& satellites = constellation->GetSatellites();

    for(const auto& satellite : satellites)
    {
        Time epochTime = satellite->GetTleEpochAsNs3Time();
        Vector pos0 = satellite->GetPosition(epochTime);
        Vector vel0 = satellite->GetVelocity(epochTime);
        // trivial position and velocity check
        NS_TEST_EXPECT_MSG_NE(pos0, Vector(0.0, 0.0, 0.0), "Satellite position should not be zero");
        NS_TEST_EXPECT_MSG_NE(vel0, Vector(0.0, 0.0, 0.0), "Satellite velocity should not be zero");

        // Check that altitude is within expected range for LEO satellites (between 160 km and 2000 km)
        double altitude0 = pos0.GetLength() - 6371000.0; // Earth radius
        // NS_LOG_INFO("Satellite " << satellite->GetName() << " altitude at epoch: " << altitude0 << " meters");
        NS_TEST_EXPECT_MSG_GT(altitude0, 200000.0, "Satellite altitude should be above 200 km");
        NS_TEST_EXPECT_MSG_LT(altitude0, 800000.0, "Satellite altitude should be below 800 km");

        double speed = vel0.GetLength();
        // NS_LOG_INFO("Satellite " << satellite->GetName() << " speed at epoch: " << speed << " m/s");
        NS_TEST_EXPECT_MSG_GT(speed, 7000.0, "Satellite speed should be above 7000 m/s");
        NS_TEST_EXPECT_MSG_LT(speed, 8000.0, "Satellite speed should be below 8000 m/s");

        Time t1 = epochTime + Seconds(1.0); // 1 second later
        Vector pos1 = satellite->GetPosition(t1);
        NS_TEST_EXPECT_MSG_NE(pos0, pos1, "Satellite position should change after 1 second");

        // Position is now ECEF while GetVelocity() still returns inertial-frame velocity.
        // Therefore, do not expect exact pos(t+1) ~= pos(t) + vel*dt.  Instead, validate that
        // displacement over 1 second is physically plausible for LEO and close to velocity magnitude.
        double distanceTraveled = CalculateDistance(pos0, pos1);
        NS_TEST_EXPECT_MSG_GT(distanceTraveled,
                      5000.0,
                      "Satellite should travel more than 5 km in one second in LEO");
        NS_TEST_EXPECT_MSG_LT(distanceTraveled,
                      9000.0,
                      "Satellite should travel less than 9 km in one second in LEO");
        NS_TEST_EXPECT_MSG_EQ_TOL(distanceTraveled,
                      speed,
                      600.0,
                      "ECEF displacement over 1 second should remain close to inertial speed");

        double eccentricity = satellite->GetOrbitalElements().eccentricity;
        // NS_LOG_INFO("Satellite " << satellite->GetName() << " eccentricity: " << eccentricity);

        if(eccentricity < 0.0015)
        {
            // In ECEF, Earth rotates during the orbit, so the satellite is not exactly opposite.
            // Still, for near-circular LEO, half an orbit should place it far from the start point.
            double orbitalPeriod = 2.0 * M_PI * (altitude0 + 6371000.0) / speed; // T = 2 * pi * r / v
            Time tHalfOrbit = epochTime + Seconds(orbitalPeriod / 2.0);
            Vector posHalfOrbit = satellite->GetPosition(tHalfOrbit);
            double distanceFromStart = CalculateDistance(posHalfOrbit, pos0);
            NS_TEST_EXPECT_MSG_GT(distanceFromStart,
                                  1000000.0,
                                  "Satellite should be far from its start position after half an orbit");
        }
        else
        {
            // NS_LOG_INFO("Satellite " << satellite->GetName() << " has eccentricity " << eccentricity << ", skipping half orbit position check");
        }

        // Test ground track position
        auto gt = satellite->GetGroundTrackPosition(epochTime);
        NS_TEST_EXPECT_MSG_GT(gt.latitude, -90.0, "Ground track latitude should be > -90");
        NS_TEST_EXPECT_MSG_LT(gt.latitude, 90.0, "Ground track latitude should be < 90");
        NS_TEST_EXPECT_MSG_GT(gt.longitude, -180.0, "Ground track longitude should be > -180");
        NS_TEST_EXPECT_MSG_LT(gt.longitude, 180.0, "Ground track longitude should be < 180");
        NS_TEST_EXPECT_MSG_GT(gt.altitude, 0.0, "Ground track altitude should be positive");

        // NS_LOG_INFO("Satellite " << satellite->GetName() << " ground track: lat=" << gt.latitude
        //                          << " lon=" << gt.longitude << " alt=" << gt.altitude << "m");
    }

    std::vector<Ptr<SatelliteLink>> links = constellation->CreateIslLinks(2000000.0);
    std::string dot = constellation->ExportIslAsDot(links, true);
    NS_TEST_EXPECT_MSG_NE(dot.find("pos="), std::string::npos, "DOT output should include node positions");
}