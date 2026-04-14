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
 * Tool to visualize Inter-Satellite Links (ISLs) in a constellation.
 * Generates a Graphviz DOT format representation of the ISL topology.
 *
 * Usage: isl-visualizer --ringFile=<path> [--maxRange=<km>]
 *        [--simTime=<seconds>] [--outputFile=<path>]
 *
 * Example:
 *   ./ns3 run orbitshield-isl-visualizer -- --ringFile=contrib/orbitshield/data/iridium-20260312.rings
 *   ./ns3 run orbitshield-isl-visualizer -- --ringFile=contrib/orbitshield/data/iridium-20260312.rings --maxRange=5000 --simTime=600 --outputFile=output.dot
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/orbitshield-module.h"
#include "ns3/orbitshield-utils.h"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace ns3;

/**
 * \brief Main function
 */
int
main(int argc, char* argv[])
{
    std::string ringFile = "contrib/orbitshield/data/iridium-20260312.rings";
    std::string outputFile;
    double maxRangeKm = 5000.0;   // 5000 km default for LEO at ~700 km altitude
    double simTimeSec = 0.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("ringFile", "Path to constellation ring file", ringFile);
    cmd.AddValue("maxRange", "Maximum ISL range in kilometers", maxRangeKm);
    cmd.AddValue("simTime", "Simulation time in seconds used to compute positions/links", simTimeSec);
    cmd.AddValue("outputFile", "Optional path to write Graphviz DOT output", outputFile);
    cmd.Parse(argc, argv);

    if (ringFile.empty())
    {
        std::cerr << "Error: ringFile must be provided\n";
        return 1;
    }

    if (maxRangeKm <= 0.0)
    {
        std::cerr << "Error: maxRange must be positive\n";
        return 1;
    }

    // Convert from km to meters
    double maxRangeMeters = maxRangeKm * 1000.0;

    if (simTimeSec < 0.0)
    {
        std::cerr << "Error: simTime must be >= 0\n";
        return 1;
    }

    // Check if ring file exists
    std::ifstream fileCheck(ringFile);
    if (!fileCheck.good())
    {
        std::cerr << "Error: cannot open ring file: " << ringFile << "\n";
        return 1;
    }
    fileCheck.close();

    try
    {
        // Create constellation and load satellites from ring metadata
        Ptr<Constellation> constellation = CreateObject<Constellation>();
        constellation->LoadFromRingFile(ringFile);

        const auto& satellites = constellation->GetSatellites();

        if (satellites.empty())
        {
            std::cerr << "Error: no satellites loaded from ring file\n";
            return 1;
        }

        if (simTimeSec > 0.0)
        {
            Simulator::Stop(Seconds(simTimeSec));
            Simulator::Run();
        }

        // Create ISL links
        std::vector<Ptr<ns3::SatelliteLink>> links = constellation->CreateIslLinks(maxRangeMeters);

        // Export as DOT format
        std::string dotOutput = constellation->ExportIslAsDot(links, true);

        // Add metadata consumed by the world-map renderer.
        const perturb::JulianDate startJd = satellites.front()->GetSimulationStartJD();
        const perturb::JulianDate currentJd = startJd + (simTimeSec / 86400.0);
        const std::string utcString = JulianDateToString(currentJd);
        std::string constellationName = constellation->GetConstellationName();
        if (constellationName.empty())
        {
            constellationName = "Constellation";
        }

        std::ostringstream metadata;
        metadata << "// orbitshield.constellation=" << constellationName << "\n";
        metadata << "// orbitshield.utc=" << utcString << " UTC\n";
        metadata << "// orbitshield.sim_time_s=" << simTimeSec << "\n";
        dotOutput = metadata.str() + dotOutput;

        if (!outputFile.empty())
        {
            std::ofstream out(outputFile);
            if (!out.is_open())
            {
                std::cerr << "Error: cannot open output file: " << outputFile << "\n";
                return 1;
            }
            out << dotOutput;
            out.close();
        }
        else
        {
            std::cout << dotOutput;
        }

        Simulator::Destroy();
        return 0;
    }
    catch (const std::exception& e)
    {
        Simulator::Destroy();
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
