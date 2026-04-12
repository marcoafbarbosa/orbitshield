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
 * Usage: isl-visualizer <ring-file> <max-range-meters> [output-file]
 *
 * Example:
 *   isl-visualizer contrib/orbitshield/data/iridium-20260312.rings 2000000
 *   isl-visualizer contrib/orbitshield/data/iridium-20260312.rings 2000000 output.dot
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/orbitshield-module.h"

#include <iostream>
#include <fstream>
#include <cstdlib>

#ifdef HAVE_BOOST_PROGRAM_OPTIONS
#include <boost/program_options.hpp>
namespace po = boost::program_options;
#endif

using namespace ns3;

#ifdef HAVE_BOOST_PROGRAM_OPTIONS
/**
 * \brief Print usage information using Boost program options help text
 * \param programName The name of the program
 * \param desc The Boost options description
 */
void
PrintUsage(const char* programName, const po::options_description& desc)
{
    std::cerr << "Usage: " << programName << " [options] <ring-file> <max-range-meters> [output-file]\n"
              << "\n"
              << desc << "\n"
              << "Output:\n"
              << "  Outputs Graphviz DOT format representation to stdout by default.\n"
              << "  If output-file is provided, the DOT graph is written there.\n"
              << "\n"
              << "Examples:\n"
              << "  " << programName << " contrib/orbitshield/data/iridium-20260312.rings 2000000\n"
              << "  " << programName << " contrib/orbitshield/data/iridium-20260312.rings 2000000 output.dot\n"
              << "  dot -Tpng output.dot -o output.png\n";
}
#else
/**
 * \brief Print usage information
 * \param programName The name of the program
 */
void
PrintUsage(const char* programName)
{
    std::cerr << "Usage: " << programName << " <ring-file> <max-range-meters> [output-file]\n"
              << "\n"
              << "Arguments:\n"
              << "  <ring-file>          Path to constellation ring file (e.g., contrib/orbitshield/data/iridium-20260312.rings)\n"
              << "  <max-range-meters>   Maximum ISL range in meters (e.g., 2000000 for 2000 km)\n"
              << "  [output-file]        Optional file path to write Graphviz DOT output\n"
              << "\n"
              << "Output:\n"
              << "  Outputs Graphviz DOT format representation to stdout by default.\n"
              << "  If output-file is provided, the DOT graph is written there.\n"
              << "\n"
              << "Examples:\n"
              << "  " << programName << " contrib/orbitshield/data/iridium-20260312.rings 2000000\n"
              << "  " << programName << " contrib/orbitshield/data/iridium-20260312.rings 2000000 output.dot\n"
              << "  dot -Tpng output.dot -o output.png\n";
}
#endif

/**
 * \brief Main function
 */
int
main(int argc, char* argv[])
{
    std::string ringFile;
    std::string outputFile;
    double maxRange = 0.0;

#ifdef HAVE_BOOST_PROGRAM_OPTIONS
    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "Show this help message")
        ("ring-file", po::value<std::string>(), "Path to constellation ring file")
        ("max-range-meters", po::value<double>(), "Maximum ISL range in meters")
        ("output-file", po::value<std::string>(), "Optional path to write Graphviz DOT output");

    po::positional_options_description positional;
    positional.add("ring-file", 1);
    positional.add("max-range-meters", 1);
    positional.add("output-file", 1);

    po::variables_map vm;
    try
    {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(positional).run(), vm);
        if (vm.count("help"))
        {
            PrintUsage(argv[0], desc);
            return 0;
        }
        po::notify(vm);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n\n";
        PrintUsage(argv[0], desc);
        return 1;
    }

    if (!vm.count("ring-file") || !vm.count("max-range-meters"))
    {
        PrintUsage(argv[0], desc);
        return 1;
    }

    ringFile = vm["ring-file"].as<std::string>();
    maxRange = vm["max-range-meters"].as<double>();
    if (vm.count("output-file"))
    {
        outputFile = vm["output-file"].as<std::string>();
    }
#else
    if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help"))
    {
        PrintUsage(argv[0]);
        return 0;
    }

    if (argc < 3 || argc > 4)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    ringFile = argv[1];
    if (argc == 4)
    {
        outputFile = argv[3];
    }

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
    catch (const std::exception&)
    {
        std::cerr << "Error: invalid max-range-meters value: " << argv[2] << "\n";
        return 1;
    }
#endif

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

        // Create ISL links
        std::vector<Ptr<ns3::SatelliteLink>> links = constellation->CreateIslLinks(maxRange);

        // Export as DOT format
        std::string dotOutput = constellation->ExportIslAsDot(links, true);

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

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
