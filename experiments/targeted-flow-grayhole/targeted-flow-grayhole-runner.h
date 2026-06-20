/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_TARGETED_FLOW_GRAYHOLE_RUNNER_H
#define ORBITSHIELD_TARGETED_FLOW_GRAYHOLE_RUNNER_H

#include <cstdint>
#include <string>
#include <vector>

namespace ns3
{

struct OrbitShieldTargetedFlowGrayholeConfig;

struct OrbitShieldTargetedFlowGrayholeExperimentSummary
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

bool RunOrbitShieldTargetedFlowGrayholeExperiment(const OrbitShieldTargetedFlowGrayholeConfig& config,
                                       OrbitShieldTargetedFlowGrayholeExperimentSummary& summary,
                                       std::string* errorMessage = nullptr);

} // namespace ns3

#endif /* ORBITSHIELD_TARGETED_FLOW_GRAYHOLE_RUNNER_H */