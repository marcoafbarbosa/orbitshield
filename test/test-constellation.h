/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef TEST_CONSTELLATION_H
#define TEST_CONSTELLATION_H

#include "ns3/test.h"

using namespace ns3;

/**
 * \brief Test case for Constellation class
 */
class ConstellationTestCase : public TestCase
{
  public:
    ConstellationTestCase();
    ~ConstellationTestCase() override;

  private:
    void DoRun() override;

    void TestSimple();
    void TestIridium();
};

#endif /* TEST_CONSTELLATION_H */
