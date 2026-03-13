/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

/**
 * \file
 * \ingroup orbitshield
 * Example of loading multiple satellites from TLE data in the iridium file using Constellation.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/orbitshield-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OrbitShieldLoadFromTleExample");

/**
 * \brief Main function demonstrating loading multiple satellites from TLE file using Constellation
 */
int
main(int argc, char* argv[])
{
    LogComponentEnable("OrbitShieldLoadFromTleExample", LOG_LEVEL_INFO);

    NS_LOG_INFO("Loading satellites from iridium TLE file using Constellation...");

    // Create a constellation and load satellites from the TLE file
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromTleFile("contrib/orbitshield/data/iridium-20260312.txt");

    // Get the satellites from the constellation
    const auto& satellites = constellation->GetSatellites();

    NS_LOG_INFO("Loaded " << satellites.size() << " satellites");

    // Log information about each satellite
    for (const auto& satellite : satellites)
    {
        NS_LOG_INFO("Satellite " << satellite->GetName() << ": Position=" << satellite->GetPosition());
    }

    NS_LOG_INFO("Finished processing all satellites");

    return 0;
}
