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
 * Usage: isl-visualizer <ring-file> <max-range-meters>
 *
 * Example:
 *   isl-visualizer contrib/orbitshield/data/iridium-20260312.rings 2000000
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/orbitshield-module.h"

#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace ns3;

/**
 * \brief Print usage information
 * \param programName The name of the program
 */
void
PrintUsage(const char* programName)
{
    std::cerr << "Usage: " << programName << " <ring-file> <max-range-meters>\n" 
              << "\n"
              << "Arguments:\n"
              << "  <ring-file>          Path to constellation ring file (e.g., contrib/orbitshield/data/iridium-20260312.rings)\n"
              << "  <max-range-meters>   Maximum ISL range in meters (e.g., 2000000 for 2000 km)\n"
              << "\n"
              << "Output:\n"
              << "  Outputs Graphviz DOT format representation to stdout.\n"
              << "  Redirect to a file for later visualization:\n"
              << "    " << programName << " <tle-file> <max-range> > output.dot\n"
              << "    dot -Tpng output.dot -o output.png\n";
}

/**
 * \brief Main function
 */
int
main(int argc, char* argv[])
{
    if (argc != 3)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    std::string ringFile = argv[1];
    double maxRange = 0.0;

    // Parse max range
    try
    {
        maxRange = std::stod(argv[2]);
        if (maxRange <= 0)
        {
            std::cerr << "Error: max-range-meters must be positive\n";
            return 1;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: invalid max-range-kms value: " << argv[2] << "\n";
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
            std::cerr << "Error: no satellites loaded from TLE file\n";
            return 1;
        }

        // Create ISL links
        std::vector<Ptr<ns3::SatelliteLink>> links = constellation->CreateIslLinks(maxRange);

        // Export as DOT format and output to stdout
        std::string dotOutput = constellation->ExportIslAsDot(links, true);
        std::cout << dotOutput;

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
