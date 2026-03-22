/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef TEST_SATELLITE_H
#define TEST_SATELLITE_H

#include "ns3/test.h"

using namespace ns3;

/**
 * \brief Test case for Satellite class
 */
class SatelliteTestCase : public TestCase
{
  public:
    SatelliteTestCase();
    ~SatelliteTestCase() override;

  private:
    void DoRun() override;

    void TestSimple();
    void TestWithConstellation();
};

#endif /* TEST_SATELLITE_H */
