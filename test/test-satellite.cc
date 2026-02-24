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
    Ptr<Satellite> satellite = CreateObject<Satellite>();

    // Test default altitude
    NS_TEST_EXPECT_MSG_EQ(satellite->GetAltitude(), 400000.0, "Default altitude should be 400 km");

    // Test setting altitude
    satellite->SetAltitude(500000.0);
    NS_TEST_EXPECT_MSG_EQ(satellite->GetAltitude(), 500000.0, "Altitude should be 500 km");

    // Test default inclination
    NS_TEST_EXPECT_MSG_EQ(satellite->GetInclination(), 45.0, "Default inclination should be 45 degrees");

    // Test setting inclination
    satellite->SetInclination(51.6);
    NS_TEST_EXPECT_MSG_EQ(satellite->GetInclination(), 51.6, "Inclination should be 51.6 degrees");
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
