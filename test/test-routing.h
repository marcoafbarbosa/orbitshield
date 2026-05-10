/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef TEST_ROUTING_H
#define TEST_ROUTING_H

#include "ns3/test.h"

using namespace ns3;

/**
 * \brief Test case for OrbitShield Routing implementation
 */
class OrbitShieldIridiumTopologyTest : public TestCase
{
  public:
    OrbitShieldIridiumTopologyTest();
    ~OrbitShieldIridiumTopologyTest() override;

  private:
    void DoRun() override;
};

/**
 * \brief Test case for OrbitShield routing helper API
 */
class OrbitShieldRoutingHelperTest : public TestCase
{
  public:
    OrbitShieldRoutingHelperTest();
    ~OrbitShieldRoutingHelperTest() override;

  private:
    void DoRun() override;
};

/**
 * \brief Test case for multi-link SatelliteNetDevice support
 */
class OrbitShieldMultiLinkDeviceTest : public TestCase
{
  public:
    OrbitShieldMultiLinkDeviceTest();
    ~OrbitShieldMultiLinkDeviceTest() override;

  private:
    void DoRun() override;
};

/**
 * \brief Test case for ground station multi-link support
 */
class OrbitShieldGroundStationMultiLinkTest : public TestCase
{
  public:
    OrbitShieldGroundStationMultiLinkTest();
    ~OrbitShieldGroundStationMultiLinkTest() override;

  private:
    void DoRun() override;
};

/**
 * \brief Test case for sequential /30 IPv4 address assignment
 *
 * Verifies that OrbitShieldRoutingHelper::Install() assigns IPv4 addresses
 * to all ISL and GSL link interfaces using strictly sequential /30 subnets
 * starting at 10.0.0.0, with ISLs allocated first and GSLs continuing
 * from the next block with no gap or offset.
 */
class OrbitShieldIpv4AddressAssignmentTest : public TestCase
{
  public:
    OrbitShieldIpv4AddressAssignmentTest();
    ~OrbitShieldIpv4AddressAssignmentTest() override;

  private:
    void DoRun() override;
};

#endif /* TEST_ROUTING_H */
