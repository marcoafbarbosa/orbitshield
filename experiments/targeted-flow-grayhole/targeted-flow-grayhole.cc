/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "ns3/core-module.h"
#include "ns3/orbitshield-module.h"

#include "targeted-flow-grayhole-config.h"
#include "targeted-flow-grayhole-runner.h"

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
    std::string configPath = "contrib/orbitshield/experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml";
    double durationSeconds = -1.0;
    double refreshIntervalSeconds = -1.0;
    double attackDropProbability = -1.0;
    std::string mitigationEnabledOverride;
    std::string outputDir;

    CommandLine cmd(__FILE__);
    cmd.AddValue("config", "Path to targeted-flow grayhole YAML profile", configPath);
    cmd.AddValue("durationSeconds", "Override simulation duration", durationSeconds);
    cmd.AddValue("refreshIntervalSeconds", "Override topology refresh interval", refreshIntervalSeconds);
    cmd.AddValue("attackDropProbability", "Override attack drop probability", attackDropProbability);
    cmd.AddValue("mitigationEnabled", "Override mitigation enabled flag: true or false", mitigationEnabledOverride);
    cmd.AddValue("outputDir", "Override telemetry output directory", outputDir);
    cmd.Parse(argc, argv);

    OrbitShieldTargetedFlowGrayholeConfig config;
    std::string error;
    if (!LoadOrbitShieldTargetedFlowGrayholeConfig(configPath, config, &error))
    {
        std::cerr << "Failed to load targeted-flow grayhole profile: " << error << std::endl;
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

    OrbitShieldTargetedFlowGrayholeExperimentSummary summary;
    if (!RunOrbitShieldTargetedFlowGrayholeExperiment(config, summary, &error))
    {
        std::cerr << "Targeted-flow grayhole run failed: " << error << std::endl;
        return 1;
    }

    std::cout << "Targeted-flow grayhole run complete" << std::endl;
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