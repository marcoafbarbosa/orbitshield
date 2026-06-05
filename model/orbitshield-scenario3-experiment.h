/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_SCENARIO3_EXPERIMENT_H
#define ORBITSHIELD_SCENARIO3_EXPERIMENT_H

#include "orbitshield-scenario3-config.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ns3
{

struct OrbitShieldScenario3ExperimentSummary
{
    double baselinePdr{0.0};
    double attackPdr{0.0};
    double postMitigationPdr{0.0};
    uint32_t dropEvents{0};
    uint32_t mitigationEvents{0};
    std::vector<std::string> flaggedSatellites;
    std::vector<std::string> excludedSatellites;
    std::string outputDir;
};

bool RunOrbitShieldScenario3Experiment(const OrbitShieldScenario3Config& config,
                                       OrbitShieldScenario3ExperimentSummary& summary,
                                       std::string* errorMessage = nullptr);

} // namespace ns3

#endif /* ORBITSHIELD_SCENARIO3_EXPERIMENT_H */