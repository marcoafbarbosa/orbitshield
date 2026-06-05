/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef TEST_ROUTING_H
#define TEST_ROUTING_H

#include "ns3/test.h"
#include "ns3/nstime.h"

/**
 * \brief Test case for OrbitShield Routing implementation
 */
class OrbitShieldIridiumTopologyTest : public ns3::TestCase
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
class OrbitShieldRoutingHelperTest : public ns3::TestCase
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
class OrbitShieldMultiLinkDeviceTest : public ns3::TestCase
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
class OrbitShieldGroundStationMultiLinkTest : public ns3::TestCase
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
class OrbitShieldIpv4AddressAssignmentTest : public ns3::TestCase
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
class OrbitShieldRefreshSafeRoutingTest : public ns3::TestCase
{
  public:
    OrbitShieldRefreshSafeRoutingTest();
    ~OrbitShieldRefreshSafeRoutingTest() override;

  private:
    void DoRun() override;
    void OnRttTrace(uint16_t seq, ns3::Time rtt);

    uint32_t m_rttCount{0};
};

/**
 * \brief Test case for end-to-end Tempe->Fairbanks ping path validation
 *
 * Verifies ICMP delivery over a fixed 60-second simulation window at epoch,
 * bounds RTT to <= 500 ms, and confirms the route is multi-hop through
 * the satellite mesh.
 */
class OrbitShieldTempeFairbanksPingPathTest : public ns3::TestCase
{
  public:
    OrbitShieldTempeFairbanksPingPathTest();
    ~OrbitShieldTempeFairbanksPingPathTest() override;

  private:
    void DoRun() override;
    void OnRttTrace(uint16_t seq, ns3::Time rtt);

    uint32_t m_replyCount{0};
    ns3::Time m_minRtt{ns3::Seconds(0)};
    ns3::Time m_maxRtt{ns3::Seconds(0)};
};

/**
 * \brief Test case for dynamic route refresh across topology changes
 *
 * Verifies that routes are properly updated when the constellation topology
 * refreshes over a 600-second window with 60-second refresh intervals.
 * Sends one ICMP echo from Tempe to Fairbanks per refresh interval and
 * validates that at least one echo reply is received within the first 120 seconds.
 */
class OrbitShieldDynamicRouteRefreshTest : public ns3::TestCase
{
  public:
    OrbitShieldDynamicRouteRefreshTest();
    ~OrbitShieldDynamicRouteRefreshTest() override;

  private:
    void DoRun() override;
    void OnRttTrace(uint16_t seq, ns3::Time rtt);

    uint32_t m_replyCount{0};
    ns3::Time m_minRtt{ns3::Seconds(0)};
    ns3::Time m_maxRtt{ns3::Seconds(0)};
};

/**
 * \brief Test case for multi-ground-station routing across the Iridium constellation
 *
 * Verifies that all 5 ground stations (Tempe, Fairbanks, Svalbard, Izhevsk, Punta Arenas)
 * can route traffic between all 10 pairwise combinations over a 300-second simulation window
 * with 30-second topology refresh intervals.
 *
 * Pass conditions:
 * - At least 7 of 10 GS pairs achieve a delivery ratio >= 80%.
 * - Every delivered packet has RTT <= 500 ms.
 * - Maximum hop count across all delivered packets is <= 8.
 * - At least 3 distinct GS pairs achieve 100% delivery.
 * - Simulation completes without crash or assertion failure.
 */
class OrbitShieldMultiGroundStationRoutingTest : public ns3::TestCase
{
  public:
    OrbitShieldMultiGroundStationRoutingTest();
    ~OrbitShieldMultiGroundStationRoutingTest() override;

  private:
    void DoRun() override;
};

/**
 * \brief Test case to validate static routing strategy under fast topology refresh
 *
 * Confirms that static route recomputation remains robust when topology refreshes
 * occur frequently (every 15 seconds over 300 seconds = 20 recomputations).
 * Validates no crash, assertion failure, or routing breakdown under rapid refresh.
 */
class OrbitShieldStaticRoutingStrategyTest : public ns3::TestCase
{
  public:
    OrbitShieldStaticRoutingStrategyTest();
    ~OrbitShieldStaticRoutingStrategyTest() override;

  private:
    void DoRun() override;
    void OnRttTrace(uint16_t seq, ns3::Time rtt);

    uint32_t m_replyCount{0};
};

/**
 * \brief Test case for Scenario 3 YAML profile loading and validation
 */
class OrbitShieldScenario3ConfigTest : public ns3::TestCase
{
  public:
    OrbitShieldScenario3ConfigTest();
    ~OrbitShieldScenario3ConfigTest() override;

  private:
    void DoRun() override;
};

/**
 * \brief Test case for current route path membership introspection
 */
class OrbitShieldRouteMembershipTest : public ns3::TestCase
{
  public:
    OrbitShieldRouteMembershipTest();
    ~OrbitShieldRouteMembershipTest() override;

  private:
    void DoRun() override;
};

/**
 * \brief Test case for route-conditioned grayhole forwarding policy
 */
class OrbitShieldGrayholePolicyTest : public ns3::TestCase
{
  public:
    OrbitShieldGrayholePolicyTest();
    ~OrbitShieldGrayholePolicyTest() override;

  private:
    void DoRun() override;
};

/**
 * \brief Test case for Scenario 3 telemetry records and CSV artifacts
 */
class OrbitShieldScenario3TelemetryTest : public ns3::TestCase
{
  public:
    OrbitShieldScenario3TelemetryTest();
    ~OrbitShieldScenario3TelemetryTest() override;

  private:
    void DoRun() override;
};

#endif /* TEST_ROUTING_H */
