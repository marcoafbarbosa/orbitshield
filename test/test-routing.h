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

#endif /* TEST_ROUTING_H */
