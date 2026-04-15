/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

/**
 * \file
 * \ingroup orbitshield
 * Example of loading multiple satellites from a YAML constellation description file.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/orbitshield-module.h"

#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OrbitShieldLoadFromYamlExample");

/**
 * \brief Main function demonstrating loading multiple satellites from a YAML constellation description file
 */
int
main(int argc, char* argv[])
{
    LogComponentEnable("OrbitShieldLoadFromYamlExample", LOG_LEVEL_INFO);

    NS_LOG_INFO("Loading satellites from Iridium constellation description file using Constellation...");

    // Create a constellation and load satellites from the YAML description file
    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromRingFile("contrib/orbitshield/data/iridium-20260312.yaml");

    // Get the satellites from the constellation
    const auto& satellites = constellation->GetSatellites();

    NS_LOG_INFO("Loaded " << satellites.size() << " satellites");

    // Log information about each satellite
    for (const auto& satellite : satellites)
    {
        NS_LOG_INFO("Satellite " << satellite->GetName() << ": Position=" << satellite->GetPosition());
    }

    for (const auto& groundStation : constellation->GetGroundStations())
    {
        NS_LOG_INFO("Ground station " << groundStation->GetName() << ": lat="
                                      << groundStation->GetLatitude() << ", lon="
                                      << groundStation->GetLongitude());
    }

    NS_LOG_INFO("Finished processing all satellites");

    // Create ISL links with 2000 km max range
    double maxRange = 10000000.0; // 10000 km in meters
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