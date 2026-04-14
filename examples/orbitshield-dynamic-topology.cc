/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file
 * \ingroup orbitshield
 *
 * Example demonstrating dynamic ISL topology updates as time progresses.
 * Shows how satellite constellation connectivity changes over time due to orbital motion.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/orbitshield-module.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace ns3;

/**
 * \brief Main function
 */
int
main(int argc, char* argv[])
{
    // Parse command line arguments
    std::string ringFile = "contrib/orbitshield/data/iridium-20260312.yaml";
    double maxRange = 7500000.0; // 7500 km
    Time refreshInterval = Seconds(10); // 10 seconds
    int timeSteps = 60; // iterations
    Time timeDelta = Seconds(60); // 1 minute per step

    CommandLine cmd(__FILE__);
    cmd.AddValue("ringFile", "Path to constellation ring file", ringFile);
    cmd.AddValue("maxRange", "Maximum ISL range in meters", maxRange);
    cmd.AddValue("refreshInterval", "ISL refresh interval", refreshInterval);
    cmd.AddValue("timeSteps", "Number of time steps to simulate", timeSteps);
    cmd.AddValue("timeDelta", "Time delta per step", timeDelta);
    cmd.Parse(argc, argv);

    // Create and configure constellation
    Ptr<Constellation> constellation = CreateObject<Constellation>();

    // Load satellites from ring file
    constellation->LoadFromRingFile(ringFile);

    const auto& satellites = constellation->GetSatellites();
    std::cout << "Loaded " << satellites.size() << " satellites from " << ringFile << std::endl;

    Ptr<Satellite> trackedSatellite;
    std::optional<uint32_t> trackedRingId;
    for (const auto& sat : satellites)
    {
        auto ringId = constellation->GetRingOfSatellite(sat->GetName());
        if (ringId.has_value())
        {
            trackedSatellite = sat;
            trackedRingId = ringId;
            break;
        }
    }

    if (!trackedSatellite)
    {
        std::cerr << "No ring-associated satellite found in constellation." << std::endl;
        return 1;
    }

    std::cout << "Tracking satellite " << trackedSatellite->GetName() << " in ring "
              << trackedRingId.value() << std::endl;

    // Configure ISL refresh interval before creating links so the first event
    // is scheduled with the correct interval.
    constellation->SetIslRefreshInterval(refreshInterval);
    std::cout << "ISL refresh interval: " << refreshInterval.GetSeconds() << " seconds"
              << std::endl;

    // Initialize ISL topology; this also schedules the first automatic refresh.
    std::vector<Ptr<SatelliteLink>> initialIsls = constellation->CreateIslLinks(maxRange);
    std::cout << "\nInitial ISL topology: " << initialIsls.size() << " links\n" << std::endl;

    // Advance time and observe topology changes for a fixed ring satellite
    std::cout << "Time (hours) | ISL Count | Active links from " << trackedSatellite->GetName()
              << std::endl;
    std::cout << "-------------|-----------|--------------------------------" << std::endl;

    for (int step = 0; step < timeSteps; ++step)
    {
        // Advance the simulator; the constellation will refresh ISL topology automatically.
        Simulator::Stop(timeDelta);
        Simulator::Run();

        Time currentTime = Simulator::Now();
        const auto& currentIsls = constellation->GetCurrentIsls();

        double timeHours = currentTime.GetSeconds() / 3600.0;

        struct NeighborInfo
        {
            std::string name;
            std::optional<uint32_t> ringId;
            double distanceMeters;
        };

        const Vector trackedPosition = trackedSatellite->GetPosition(currentTime);
        std::vector<NeighborInfo> neighbors;
        for (const auto& link : currentIsls)
        {
            if (!link || !link->IsActive())
            {
                continue;
            }

            auto dev0 = DynamicCast<SatelliteNetDevice>(link->GetDevice(0));
            auto dev1 = DynamicCast<SatelliteNetDevice>(link->GetDevice(1));
            if (!dev0 || !dev1)
            {
                continue;
            }

            auto sat0 = DynamicCast<Satellite>(dev0->GetNode());
            auto sat1 = DynamicCast<Satellite>(dev1->GetNode());
            if (!sat0 || !sat1)
            {
                continue;
            }

            if (sat0 == trackedSatellite)
            {
                const Vector neighborPosition = sat1->GetPosition(currentTime);
                neighbors.push_back({sat1->GetName(),
                                     constellation->GetRingOfSatellite(sat1->GetName()),
                                     CalculateDistance(trackedPosition, neighborPosition)});
            }
            else if (sat1 == trackedSatellite)
            {
                const Vector neighborPosition = sat0->GetPosition(currentTime);
                neighbors.push_back({sat0->GetName(),
                                     constellation->GetRingOfSatellite(sat0->GetName()),
                                     CalculateDistance(trackedPosition, neighborPosition)});
            }
        }

        std::sort(neighbors.begin(),
                  neighbors.end(),
                  [](const NeighborInfo& a, const NeighborInfo& b) { return a.name < b.name; });

        std::cout << std::fixed << std::setprecision(2) << std::setw(12) << timeHours << " | "
                  << std::setw(9) << currentIsls.size() << " | ";
        if (neighbors.empty())
        {
            std::cout << "(none)";
        }
        else
        {
            for (std::size_t i = 0; i < neighbors.size(); ++i)
            {
                if (i > 0)
                {
                    std::cout << ", ";
                }
                std::cout << neighbors[i].name << " (ring "
                          << (neighbors[i].ringId.has_value() ? std::to_string(neighbors[i].ringId.value())
                                                               : std::string("n/a"))
                          << ", " << std::setprecision(1) << (neighbors[i].distanceMeters / 1000.0)
                          << " km)";
            }
        }
        std::cout << std::endl;
    }

    std::cout << "\nSimulation complete." << std::endl;
    Simulator::Destroy();
    return 0;
}
