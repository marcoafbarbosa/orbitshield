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

#include <vector>

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

    // Create ISL links with 2000 km max range
    double maxRange = 2000000.0; // 2000 km in meters
    auto links = constellation->CreateIslLinks(maxRange);

    NS_LOG_INFO("Created " << links.size() << " ISL links");

    // Print active ISLs only
    std::size_t activeCount = 0;
    for (const auto& link : links)
    {
        if (link->IsActive())
        {
            Ptr<SatelliteNetDevice> devA = DynamicCast<SatelliteNetDevice>(link->GetDevice(0));
            Ptr<SatelliteNetDevice> devB = DynamicCast<SatelliteNetDevice>(link->GetDevice(1));
            Ptr<Satellite> satA = DynamicCast<Satellite>(devA->GetNode());
            Ptr<Satellite> satB = DynamicCast<Satellite>(devB->GetNode());
            std::string nameA = satA->GetName();
            std::string nameB = satB->GetName();

            Ptr<MobilityModel> mobA = satA->GetObject<MobilityModel>();
            Ptr<MobilityModel> mobB = satB->GetObject<MobilityModel>();
            double distance = mobA->GetDistanceFrom(mobB) / 1000.0; // km

            NS_LOG_INFO("ISL between " << nameA << " and " << nameB << ": active (distance: " << distance << " km)");
            activeCount++;
        }
    }

    NS_LOG_INFO("Total active ISLs: " << activeCount);

    return 0;
}
