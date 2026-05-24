/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef TEST_CONSTELLATION_H
#define TEST_CONSTELLATION_H

#include "ns3/test.h"

/**
 * \brief Test case for Constellation class
 */
class ConstellationTestCase : public ns3::TestCase
{
  public:
    ConstellationTestCase();
    ~ConstellationTestCase() override;

  private:
    void DoRun() override;

    void TestSimple();
    void TestIslFallbackWithoutRings();
    void TestIridium();
    void TestGroundDistanceOverheadEqualsAltitude();
    void TestIslRefreshHonorsRange();
    void TestGroundStationEcefPositionAfterDistanceCalc();
    void TestSatelliteMobilityModelReturnsEcefFrame();
    void TestGroundDistanceUsesConsistentCoordinateFrame();
};

#endif /* TEST_CONSTELLATION_H */
