/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_SCENARIO3_CONFIG_H
#define ORBITSHIELD_SCENARIO3_CONFIG_H

#include "constellation.h"
#include "ns3/ptr.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ns3
{

enum class OrbitShieldScenario3Direction
{
    FORWARD,
    REVERSE,
    BIDIRECTIONAL
};

struct OrbitShieldScenario3GroundPair
{
    std::string source;
    std::string destination;
};

struct OrbitShieldScenario3ConstellationConfig
{
    std::string ringFile;
};

struct OrbitShieldScenario3SimulationConfig
{
    double durationSeconds{3000.0};
    uint32_t seed{1};
    uint32_t run{1};
};

struct OrbitShieldScenario3TopologyConfig
{
    double islMaxRangeMeters{2000000.0};
    double groundMaxRangeMeters{50000000.0};
    double refreshIntervalSeconds{30.0};
};

struct OrbitShieldScenario3TrafficConfig
{
    double pingIntervalSeconds{30.0};
    uint32_t pingSizeBytes{56};
    std::vector<OrbitShieldScenario3GroundPair> pairs;
};

struct OrbitShieldScenario3AttackConfig
{
    std::vector<std::string> compromisedSatellites;
    std::vector<OrbitShieldScenario3GroundPair> targetPairs;
    OrbitShieldScenario3Direction direction{OrbitShieldScenario3Direction::BIDIRECTIONAL};
    double startSeconds{600.0};
    double stopSeconds{2400.0};
    double dropProbability{1.0};
};

struct OrbitShieldScenario3DetectionConfig
{
    bool enabled{true};
    double windowSeconds{120.0};
    uint32_t minSamples{3};
    double targetPdrThreshold{0.6};
    double scoreThreshold{1.0};
};

struct OrbitShieldScenario3MitigationConfig
{
    bool enabled{true};
    double applyDelaySeconds{30.0};
    uint32_t maxExcludedSatellites{4};
};

struct OrbitShieldScenario3TelemetryConfig
{
    std::string outputDir;
    double routeSnapshotIntervalSeconds{30.0};
    bool writeCsv{true};
};

struct OrbitShieldScenario3Config
{
    std::string profilePath;
    OrbitShieldScenario3ConstellationConfig constellation;
    OrbitShieldScenario3SimulationConfig simulation;
    OrbitShieldScenario3TopologyConfig topology;
    OrbitShieldScenario3TrafficConfig traffic;
    OrbitShieldScenario3AttackConfig attack;
    OrbitShieldScenario3DetectionConfig detection;
    OrbitShieldScenario3MitigationConfig mitigation;
    OrbitShieldScenario3TelemetryConfig telemetry;
};

std::string OrbitShieldScenario3DirectionToString(OrbitShieldScenario3Direction direction);

bool LoadOrbitShieldScenario3Config(const std::string& filename,
                                    OrbitShieldScenario3Config& config,
                                    std::string* errorMessage = nullptr);

bool ValidateOrbitShieldScenario3Config(const OrbitShieldScenario3Config& config,
                                        Ptr<Constellation> constellation,
                                        std::string* errorMessage = nullptr);

} // namespace ns3

#endif /* ORBITSHIELD_SCENARIO3_CONFIG_H */