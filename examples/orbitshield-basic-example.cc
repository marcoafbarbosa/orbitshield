/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

/**
 * \file
 * \ingroup orbitshield
 * Simple example of creating and configuring satellites using OrbitShield
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

    NS_LOG_INFO("Creating satellite constellation...");

    // Create a small satellite constellation with 3 satellites
    std::vector<Ptr<Satellite>> constellation;

    for (int i = 0; i < 3; ++i)
    {
        Ptr<Satellite> satellite = CreateObject<Satellite>();

        // Configure satellite parameters
        satellite->SetAltitude(400000.0 + i * 50000.0);  // Vary altitude
        satellite->SetInclination(51.6);                 // ISS-like inclination

        constellation.push_back(satellite);

        NS_LOG_INFO("Created Satellite " << i << ": Altitude="
                                         << satellite->GetAltitude() << "m, Inclination="
                                         << satellite->GetInclination() << "°");
    }

    NS_LOG_INFO("Constellation created with " << constellation.size() << " satellites");

    return 0;
}
