/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

/**
 * \file
 * \ingroup orbitshield
 * Simple example of creating and configuring a single satellite using the OrbitShield module.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/orbitshield-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OrbitShieldExample");

/**
 * \brief Main function demonstrating basic OrbitShield usage
 */
int
main(int argc, char* argv[])
{
    LogComponentEnable("OrbitShieldExample", LOG_LEVEL_INFO);

    NS_LOG_INFO("Creating a single satellite...");

    // Let try simulating the orbit of the International Space Station
    // Got TLE from Celestrak sometime around 2022-03-12
    std::string ISS_NAME = "ISS (ZARYA)";
    std::string ISS_TLE_1 = "1 25544U 98067A   22071.78032407  .00021395  00000-0  39008-3 0  9996";
    std::string ISS_TLE_2 = "2 25544  51.6424  94.0370 0004047 256.5103  89.8846 15.49386383330227";
    Ptr<Satellite> satellite = CreateObject<Satellite>(ISS_NAME, ISS_TLE_1, ISS_TLE_2);

    NS_LOG_INFO("Created Satellite " << satellite->GetName() << ": Position="
                                        << satellite->GetPosition());

    return 0;
}
