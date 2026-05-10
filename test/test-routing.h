/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef TEST_ROUTING_H
#define TEST_ROUTING_H

#include "ns3/test.h"
#include "ns3/nstime.h"

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

/**
 * \brief Test case for refresh-safe routing recomputation
 *
 * Verifies that route recomputation is triggered across topology refreshes
 * and that at least one ICMP echo reply is received during a refresh window.
 */
class OrbitShieldRefreshSafeRoutingTest : public TestCase
{
  public:
    OrbitShieldRefreshSafeRoutingTest();
    ~OrbitShieldRefreshSafeRoutingTest() override;

  private:
    void DoRun() override;
    void OnRttTrace(uint16_t seq, Time rtt);

    uint32_t m_rttCount{0};
};

/**
 * \brief Test case for end-to-end Tempe->Fairbanks ping path validation
 *
 * Verifies ICMP delivery over a fixed 60-second simulation window at epoch,
 * bounds RTT to <= 500 ms, and confirms the route is multi-hop through
 * the satellite mesh.
 */
class OrbitShieldTempeFairbanksPingPathTest : public TestCase
{
  public:
    OrbitShieldTempeFairbanksPingPathTest();
    ~OrbitShieldTempeFairbanksPingPathTest() override;

  private:
    void DoRun() override;
    void OnRttTrace(uint16_t seq, Time rtt);

    uint32_t m_replyCount{0};
    Time m_minRtt{Seconds(0)};
    Time m_maxRtt{Seconds(0)};
};

#endif /* TEST_ROUTING_H */
