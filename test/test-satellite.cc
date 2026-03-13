/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "ns3/orbitshield-module.h"

#include "ns3/test.h"

using namespace ns3;

/**
 * \brief Test suite for Satellite class
 */
class SatelliteTestCase : public TestCase
{
  public:
    SatelliteTestCase();
    ~SatelliteTestCase() override;

  private:
    void DoRun() override;
};

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
    Ptr<Satellite> satellite = CreateObject<Satellite>(ISS_NAME, ISS_TLE_1, ISS_TLE_2);

    // Test satellite name
    NS_TEST_EXPECT_MSG_EQ(satellite->GetName(), ISS_NAME, "Satellite name should match");
}

/**
 * \brief Test suite for OrbitShield module
 */
class OrbitShieldTestSuite : public TestSuite
{
  public:
    OrbitShieldTestSuite();
};

OrbitShieldTestSuite::OrbitShieldTestSuite()
    : TestSuite("orbitshield", Type::UNIT)
{
    AddTestCase(new SatelliteTestCase(), Duration::QUICK);
}

// Create a static variable to instantiate the test suite
static OrbitShieldTestSuite orbitshieldTestSuite;
