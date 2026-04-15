/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "test-constellation.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/geographic-positions.h"
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
    TestGroundDistanceOverheadEqualsAltitude();
    TestIslRefreshHonorsRange();
    TestGroundStationEcefPositionAfterDistanceCalc();
    TestSatelliteMobilityModelReturnsEcefFrame();
    TestGroundDistanceUsesConsistentCoordinateFrame();
}

void
ConstellationTestCase::TestGroundDistanceOverheadEqualsAltitude()
{
    Ptr<Constellation> constellation = CreateObject<Constellation>();

    // Satellite directly overhead the ground station at the same lat/lon.
    const double latitude = 12.3;
    const double longitude = -45.6;
    const double altitude = 700000.0; // 700 km

    Ptr<GroundStation> station = CreateObject<GroundStation>();
    station->SetName("GS-Overhead");
    station->SetLatitude(latitude);
    station->SetLongitude(longitude);

    std::string tle1 = "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996";
    std::string tle2 = "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227";
    perturb::JulianDate simulationStart(perturb::DateTime(2026, 1, 1, 0, 0, 0));
    Ptr<Satellite> satellite = CreateObject<Satellite>("SAT-Overhead", tle1, tle2, simulationStart);

    // Override satellite mobility to enforce exact overhead geometry for this test.
    Ptr<ConstantPositionMobilityModel> satMob = CreateObject<ConstantPositionMobilityModel>();
    const Vector satEcef = GeographicPositions::GeographicToCartesianCoordinates(
        latitude,
        longitude,
        altitude,
        GeographicPositions::EarthSpheroidType::WGS84);
    satMob->SetPosition(satEcef);
    satellite->AggregateObject(satMob);

    const double distance = constellation->CalculateSatelliteGroundDistance(satellite, station);
    NS_TEST_EXPECT_MSG_EQ_TOL(distance,
                              altitude,
                              1e-3,
                              "Satellite directly overhead a ground station should have distance equal to altitude");
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

    const char* ringFilename = "constellation-test.yaml";
    {
        std::ofstream ringFile(ringFilename);
        ringFile << "constellationName: TestConstellation\n";
        ringFile << "ringCount: 2\n";
        ringFile << "rings:\n";
        ringFile << "  - id: 0\n";
        ringFile << "    satellites:\n";
        ringFile << "      - SAT-1\n";
        ringFile << "  - id: 1\n";
        ringFile << "    satellites:\n";
        ringFile << "      - SAT-2\n";
        ringFile << "groundStations:\n";
        ringFile << "  - name: Test Ground Station\n";
        ringFile << "    latitude: 1.5\n";
        ringFile << "    longitude: -2.5\n";
    }

    constellation->LoadFromRingFile(ringFilename);
    NS_TEST_EXPECT_MSG_EQ(constellation->GetRingCount(), 2u, "Ring count should be 2");
    NS_TEST_EXPECT_MSG_EQ(constellation->GetConstellationName(),
                          std::string("TestConstellation"),
                          "Constellation name should be read from YAML metadata");

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

    const auto& groundStations = constellation->GetGroundStations();
    NS_TEST_EXPECT_MSG_EQ(groundStations.size(), 1u, "Expected one ground station in YAML metadata");
    NS_TEST_EXPECT_MSG_EQ(groundStations[0]->GetName(),
                          std::string("Test Ground Station"),
                          "Ground station name should match YAML data");
    NS_TEST_EXPECT_MSG_EQ_TOL(groundStations[0]->GetLatitude(),
                              1.5,
                              1e-9,
                              "Ground station latitude should match YAML data");
    NS_TEST_EXPECT_MSG_EQ_TOL(groundStations[0]->GetLongitude(),
                              -2.5,
                              1e-9,
                              "Ground station longitude should match YAML data");

    std::vector<Ptr<SatelliteLink>> links = constellation->CreateIslLinks(2000000.0);
    std::vector<Ptr<SatelliteLink>> groundLinks = constellation->CreateGroundLinks(50000000.0);
    links.insert(links.end(), groundLinks.begin(), groundLinks.end());

    NS_TEST_EXPECT_MSG_EQ(groundLinks.size(),
                          2u,
                          "Expected one ground link per satellite within large max range");

    std::string dot = constellation->ExportIslAsDot(links, true);
    NS_TEST_EXPECT_MSG_NE(dot.find("\"GS:Test Ground Station\""),
                          std::string::npos,
                          "DOT output should include a node for the ground station");
    bool hasGsEdge = (dot.find("\"SAT-1\" -- \"GS:Test Ground Station\"") != std::string::npos) ||
                     (dot.find("\"GS:Test Ground Station\" -- \"SAT-1\"") != std::string::npos);
    NS_TEST_EXPECT_MSG_EQ(hasGsEdge,
                          true,
                          "DOT output should include at least one satellite-ground edge");

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
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.yaml");

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

void
ConstellationTestCase::TestGroundStationEcefPositionAfterDistanceCalc()
{
    // After calling CalculateSatelliteGroundDistance, the ground station should have a
    // ConstantPositionMobilityModel aggregated to it, with its position set to the correct
    // WGS84 ECEF coordinates.
    //
    // At lat=0, lon=0 the ECEF position is (WGS84_equatorial_radius, 0, 0).

    Ptr<Constellation> constellation = CreateObject<Constellation>();

    Ptr<GroundStation> station = CreateObject<GroundStation>();
    station->SetName("GS-Equator");
    station->SetLatitude(0.0);
    station->SetLongitude(0.0);

    // Use a ConstantPositionMobilityModel for the satellite so this test does not depend
    // on coordinate-frame handling of SatelliteMobilityModel.
    std::string tle1 = "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996";
    std::string tle2 = "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227";
    perturb::JulianDate simulationStart(perturb::DateTime(2026, 1, 1, 0, 0, 0));
    Ptr<Satellite> satellite = CreateObject<Satellite>("SAT-EcefTest", tle1, tle2, simulationStart);

    Ptr<ConstantPositionMobilityModel> satMob = CreateObject<ConstantPositionMobilityModel>();
    satMob->SetPosition(Vector(7000000.0, 0.0, 0.0));
    satellite->AggregateObject(satMob);

    // Trigger distance computation, which attaches the ground station's mobility model
    constellation->CalculateSatelliteGroundDistance(satellite, station);

    // The ground station must have a MobilityModel now
    Ptr<MobilityModel> gsMob = station->GetObject<MobilityModel>();
    NS_TEST_ASSERT_MSG_NE(gsMob, nullptr,
        "Ground station should have a MobilityModel after CalculateSatelliteGroundDistance");

    Vector gsPos = gsMob->GetPosition();

    // WGS84 equatorial semi-major axis (meters)
    const double a = 6378137.0;
    NS_TEST_EXPECT_MSG_EQ_TOL(gsPos.x, a, 1.0,
        "Ground station ECEF x should equal WGS84 equatorial radius at lat=0, lon=0");
    NS_TEST_EXPECT_MSG_EQ_TOL(gsPos.y, 0.0, 1.0,
        "Ground station ECEF y should be 0 at lon=0");
    NS_TEST_EXPECT_MSG_EQ_TOL(gsPos.z, 0.0, 1.0,
        "Ground station ECEF z should be 0 at lat=0");
}

void
ConstellationTestCase::TestSatelliteMobilityModelReturnsEcefFrame()
{
    // SatelliteMobilityModel::GetPosition() is documented to delegate directly to
    // Satellite::GetPosition(Simulator::Now()), which now returns ECEF coordinates.
    //
    // This test verifies two properties without calling GetGroundTrackPosition:
    //   1. The MobilityModel position equals satellite->GetPosition(Simulator::Now()).
    //   2. The returned position has a plausible LEO orbital radius.
    //
    // Also verify that ECEF position converts to and from geodetic coordinates
    // consistently at the current simulation time.

    std::string tle1 = "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996";
    std::string tle2 = "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227";
    // Use the TLE epoch as simulation start so SGP4 propagation at t=0 gives the epoch state.
    // IMPORTANT: from_tle() mutates its string arguments in-place (sgp4::twoline2rv inserts
    // decimal points, replaces spaces).  Use separate copies for the epoch lookup so the
    // Satellite constructor receives the original, unmutated strings.
    std::string tle1_epoch_copy = tle1;
    std::string tle2_epoch_copy = tle2;
    perturb::JulianDate simulationStart = perturb::Satellite::from_tle(tle1_epoch_copy, tle2_epoch_copy).epoch();
    Ptr<Satellite> satellite = CreateObject<Satellite>("SAT-FrameTest", tle1, tle2, simulationStart);

    Ptr<SatelliteMobilityModel> satMob = CreateObject<SatelliteMobilityModel>();
    satMob->SetSatellite(satellite);
    satellite->AggregateObject(satMob);

    // Both calls must produce identical vectors.
    Vector mobilityPos = satMob->GetPosition();
    Vector directPos = satellite->GetPosition(Simulator::Now());

    NS_TEST_EXPECT_MSG_EQ_TOL(mobilityPos.x, directPos.x, 1.0,
        "SatelliteMobilityModel x must equal Satellite::GetPosition x");
    NS_TEST_EXPECT_MSG_EQ_TOL(mobilityPos.y, directPos.y, 1.0,
        "SatelliteMobilityModel y must equal Satellite::GetPosition y");
    NS_TEST_EXPECT_MSG_EQ_TOL(mobilityPos.z, directPos.z, 1.0,
        "SatelliteMobilityModel z must equal Satellite::GetPosition z");

    // Orbital radius must be in the plausible LEO range (6500-8000 km).
    double orbitalRadius = std::sqrt(mobilityPos.x * mobilityPos.x
                                     + mobilityPos.y * mobilityPos.y
                                     + mobilityPos.z * mobilityPos.z);
    NS_TEST_EXPECT_MSG_GT(orbitalRadius, 6500000.0,
        "Satellite orbital radius should be > 6500 km (above Earth surface)");
    NS_TEST_EXPECT_MSG_LT(orbitalRadius, 8000000.0,
        "Satellite orbital radius should be < 8000 km for ISS-like LEO orbit");

    Satellite::GroundTrackPosition gt = satellite->GetGroundTrackPosition(Seconds(0));
    Vector reconstructedEcef = GeographicPositions::GeographicToCartesianCoordinates(
        gt.latitude,
        gt.longitude,
        gt.altitude,
        GeographicPositions::EarthSpheroidType::WGS84);

    double ecefDiff = std::sqrt((reconstructedEcef.x - directPos.x) * (reconstructedEcef.x - directPos.x) +
                                (reconstructedEcef.y - directPos.y) * (reconstructedEcef.y - directPos.y) +
                                (reconstructedEcef.z - directPos.z) * (reconstructedEcef.z - directPos.z));
    NS_TEST_EXPECT_MSG_LT(ecefDiff,
                          10.0,
                          "GetPosition() should be ECEF and consistent with ground-track conversion");
}

void
ConstellationTestCase::TestGroundDistanceUsesConsistentCoordinateFrame()
{
    // With Satellite::GetPosition() now returning ECEF, the production path in
    // CalculateSatelliteGroundDistance() should be frame-consistent.
    //
    // For a station placed at the satellite's sub-point (same lat/lon, altitude 0),
    // distance must match satellite altitude and match manual ECEF Euclidean distance.

    std::string tle1 = "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996";
    std::string tle2 = "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227";
    std::string tle1_epoch = tle1;
    std::string tle2_epoch = tle2;
    perturb::JulianDate simStart = perturb::Satellite::from_tle(tle1_epoch, tle2_epoch).epoch();

    std::string tle1_sat = tle1;
    std::string tle2_sat = tle2;
    Ptr<Satellite> satellite = CreateObject<Satellite>("SAT-FrameConsistent", tle1_sat, tle2_sat, simStart);

    Satellite::GroundTrackPosition gt = satellite->GetGroundTrackPosition(Seconds(0));

    Ptr<GroundStation> station = CreateObject<GroundStation>();
    station->SetName("GS-SubPoint");
    station->SetLatitude(gt.latitude);
    station->SetLongitude(gt.longitude);

    Ptr<Constellation> constellation = CreateObject<Constellation>();
    double modelDistance = constellation->CalculateSatelliteGroundDistance(satellite, station);

    Vector satEcef = satellite->GetPosition(Seconds(0));
    Vector gsEcef = GeographicPositions::GeographicToCartesianCoordinates(
        gt.latitude,
        gt.longitude,
        0.0,
        GeographicPositions::EarthSpheroidType::WGS84);
    double manualDistance = std::sqrt((satEcef.x - gsEcef.x) * (satEcef.x - gsEcef.x) +
                                      (satEcef.y - gsEcef.y) * (satEcef.y - gsEcef.y) +
                                      (satEcef.z - gsEcef.z) * (satEcef.z - gsEcef.z));

    NS_TEST_EXPECT_MSG_EQ_TOL(modelDistance,
                              manualDistance,
                              1e-3,
                              "Model distance should match manual ECEF distance computation");
    NS_TEST_EXPECT_MSG_EQ_TOL(modelDistance,
                              gt.altitude,
                              gt.altitude * 0.02,
                              "Distance to sub-point ground station should match satellite altitude");
}