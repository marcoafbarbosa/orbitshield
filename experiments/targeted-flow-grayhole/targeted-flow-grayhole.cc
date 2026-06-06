/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "ns3/core-module.h"
#include "ns3/orbitshield-module.h"
#include "ns3/orbitshield-scenario3-config.h"
#include "ns3/orbitshield-scenario3-experiment.h"

#include <algorithm>
#include <iostream>
#include <sstream>

using namespace ns3;

namespace
{

bool
ParseBoolOverride(const std::string& value, bool& parsed)
{
    if (value == "true" || value == "1")
    {
        parsed = true;
        return true;
    }
    if (value == "false" || value == "0")
    {
        parsed = false;
        return true;
    }
    return false;
}

std::string
JoinNames(const std::vector<std::string>& names)
{
    std::ostringstream out;
    for (std::size_t nameIndex = 0; nameIndex < names.size(); ++nameIndex)
    {
        if (nameIndex > 0)
        {
            out << ", ";
        }
        out << names[nameIndex];
    }
    return out.str();
}

} // namespace

int
main(int argc, char* argv[])
{
    std::string configPath = "contrib/orbitshield/data/scenarios/scenario3-grayhole.yaml";
    double durationSeconds = -1.0;
    double refreshIntervalSeconds = -1.0;
    double attackDropProbability = -1.0;
    std::string mitigationEnabledOverride;
    std::string outputDir;

    CommandLine cmd(__FILE__);
    cmd.AddValue("config", "Path to Scenario 3 YAML profile", configPath);
    cmd.AddValue("durationSeconds", "Override simulation duration", durationSeconds);
    cmd.AddValue("refreshIntervalSeconds", "Override topology refresh interval", refreshIntervalSeconds);
    cmd.AddValue("attackDropProbability", "Override attack drop probability", attackDropProbability);
    cmd.AddValue("mitigationEnabled", "Override mitigation enabled flag: true or false", mitigationEnabledOverride);
    cmd.AddValue("outputDir", "Override telemetry output directory", outputDir);
    cmd.Parse(argc, argv);

    OrbitShieldScenario3Config config;
    std::string error;
    if (!LoadOrbitShieldScenario3Config(configPath, config, &error))
    {
        std::cerr << "Failed to load Scenario 3 profile: " << error << std::endl;
        return 1;
    }

    if (durationSeconds > 0.0)
    {
        config.simulation.durationSeconds = durationSeconds;
        if (config.attack.startSeconds >= durationSeconds)
        {
            config.attack.startSeconds = durationSeconds * 0.2;
            config.attack.stopSeconds = durationSeconds * 0.8;
        }
        else if (config.attack.stopSeconds > durationSeconds)
        {
            config.attack.stopSeconds = durationSeconds;
        }
    }
    if (refreshIntervalSeconds > 0.0)
    {
        config.topology.refreshIntervalSeconds = refreshIntervalSeconds;
    }
    if (attackDropProbability >= 0.0)
    {
        config.attack.dropProbability = attackDropProbability;
    }
    if (!mitigationEnabledOverride.empty())
    {
        bool mitigationEnabled = false;
        if (!ParseBoolOverride(mitigationEnabledOverride, mitigationEnabled))
        {
            std::cerr << "Invalid mitigationEnabled override: " << mitigationEnabledOverride << std::endl;
            return 1;
        }
        config.mitigation.enabled = mitigationEnabled;
    }
    if (!outputDir.empty())
    {
        config.telemetry.outputDir = outputDir;
    }

    OrbitShieldScenario3ExperimentSummary summary;
    if (!RunOrbitShieldScenario3Experiment(config, summary, &error))
    {
        std::cerr << "Scenario 3 run failed: " << error << std::endl;
        return 1;
    }

    std::cout << "Scenario 3 grayhole run complete" << std::endl;
    std::cout << "Profile: " << configPath << std::endl;
    std::cout << "Duration: " << config.simulation.durationSeconds << " seconds" << std::endl;
    std::cout << "Target pairs: " << config.attack.targetPairs.size() << std::endl;
    std::cout << "Compromised satellites: " << JoinNames(config.attack.compromisedSatellites)
              << std::endl;
    std::cout << "Flagged satellites: " << JoinNames(summary.flaggedSatellites) << std::endl;
    std::cout << "Excluded satellites: " << JoinNames(summary.excludedSatellites) << std::endl;
    std::cout << "Target PDR baseline/attack/post: " << summary.baselinePdr << "/"
              << summary.attackPdr << "/" << summary.postMitigationPdr << std::endl;
    std::cout << "Telemetry output: " << summary.outputDir << std::endl;
    return 0;
}