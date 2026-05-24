/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "test-satellite.h"
#include "test-constellation.h"
#include "test-isl.h"
#include "test-routing.h"

#include "ns3/test.h"

using namespace ns3;

/**
 * \brief OrbitShield module test suite
 */
class OrbitShieldTestSuite : public TestSuite
{
  public:
    OrbitShieldTestSuite();
};

OrbitShieldTestSuite::OrbitShieldTestSuite()
    : TestSuite("orbitshield", Type::UNIT)
{
    AddTestCase(new SatelliteTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new ConstellationTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new IslChannelTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new OrbitShieldIridiumTopologyTest(), TestCase::Duration::QUICK);
    AddTestCase(new OrbitShieldRoutingHelperTest(), TestCase::Duration::QUICK);
    AddTestCase(new OrbitShieldMultiLinkDeviceTest(), TestCase::Duration::QUICK);
    AddTestCase(new OrbitShieldGroundStationMultiLinkTest(), TestCase::Duration::QUICK);
    AddTestCase(new OrbitShieldIpv4AddressAssignmentTest(), TestCase::Duration::QUICK);
    AddTestCase(new OrbitShieldRefreshSafeRoutingTest(), TestCase::Duration::QUICK);
    AddTestCase(new OrbitShieldTempeFairbanksPingPathTest(), TestCase::Duration::QUICK);
    AddTestCase(new OrbitShieldDynamicRouteRefreshTest(), TestCase::Duration::QUICK);
    AddTestCase(new OrbitShieldMultiGroundStationRoutingTest(), TestCase::Duration::QUICK);
    AddTestCase(new OrbitShieldStaticRoutingStrategyTest(), TestCase::Duration::QUICK);
}

static OrbitShieldTestSuite g_orbitShieldTestSuite;
