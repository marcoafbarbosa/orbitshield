/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_TARGETED_FLOW_GRAYHOLE_CONFIG_H
#define ORBITSHIELD_TARGETED_FLOW_GRAYHOLE_CONFIG_H

#include "ns3/constellation.h"
#include "ns3/ptr.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ns3
{

enum class OrbitShieldTargetedFlowGrayholeDirection
{
    FORWARD,
    REVERSE,
    BIDIRECTIONAL
};

struct OrbitShieldTargetedFlowGrayholeGroundPair
{
    std::string source;
    std::string destination;
};

struct OrbitShieldTargetedFlowGrayholeConstellationConfig
{
    std::string ringFile;
};

struct OrbitShieldTargetedFlowGrayholeSimulationConfig
{
    double durationSeconds{3000.0};
    uint32_t seed{1};
    uint32_t run{1};
};

struct OrbitShieldTargetedFlowGrayholeTopologyConfig
{
    double islMaxRangeMeters{2000000.0};
    double groundMaxRangeMeters{50000000.0};
    double refreshIntervalSeconds{30.0};
};

struct OrbitShieldTargetedFlowGrayholeTrafficConfig
{
    double pingIntervalSeconds{30.0};
    uint32_t pingSizeBytes{56};
    std::vector<OrbitShieldTargetedFlowGrayholeGroundPair> pairs;
};

struct OrbitShieldTargetedFlowGrayholeAttackConfig
{
    std::vector<std::string> compromisedSatellites;
    std::vector<OrbitShieldTargetedFlowGrayholeGroundPair> targetPairs;
    OrbitShieldTargetedFlowGrayholeDirection direction{OrbitShieldTargetedFlowGrayholeDirection::BIDIRECTIONAL};
    double startSeconds{600.0};
    double stopSeconds{2400.0};
    double dropProbability{1.0};
};

struct OrbitShieldTargetedFlowGrayholeDetectionConfig
{
    bool enabled{true};
    double windowSeconds{120.0};
    uint32_t minSamples{3};
    double targetPdrThreshold{0.6};
    double scoreThreshold{1.0};
};

struct OrbitShieldTargetedFlowGrayholeMitigationConfig
{
    bool enabled{true};
    double applyDelaySeconds{30.0};
    uint32_t maxExcludedSatellites{4};
};

struct OrbitShieldTargetedFlowGrayholeTelemetryConfig
{
    std::string outputDir;
    double routeSnapshotIntervalSeconds{30.0};
    bool writeCsv{true};
};

struct OrbitShieldTargetedFlowGrayholeConfig
{
    std::string profilePath;
    OrbitShieldTargetedFlowGrayholeConstellationConfig constellation;
    OrbitShieldTargetedFlowGrayholeSimulationConfig simulation;
    OrbitShieldTargetedFlowGrayholeTopologyConfig topology;
    OrbitShieldTargetedFlowGrayholeTrafficConfig traffic;
    OrbitShieldTargetedFlowGrayholeAttackConfig attack;
    OrbitShieldTargetedFlowGrayholeDetectionConfig detection;
    OrbitShieldTargetedFlowGrayholeMitigationConfig mitigation;
    OrbitShieldTargetedFlowGrayholeTelemetryConfig telemetry;
};

std::string OrbitShieldTargetedFlowGrayholeDirectionToString(OrbitShieldTargetedFlowGrayholeDirection direction);

bool LoadOrbitShieldTargetedFlowGrayholeConfig(const std::string& filename,
                                               OrbitShieldTargetedFlowGrayholeConfig& config,
                                               std::string* errorMessage = nullptr);

bool ValidateOrbitShieldTargetedFlowGrayholeConfig(const OrbitShieldTargetedFlowGrayholeConfig& config,
                                                   Ptr<Constellation> constellation,
                                                   std::string* errorMessage = nullptr);

} // namespace ns3

#endif /* ORBITSHIELD_TARGETED_FLOW_GRAYHOLE_CONFIG_H */