/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "targeted-flow-grayhole-config.h"

#include "ns3/ground-station.h"
#include "ns3/satellite.h"

#include "ns3/log.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OrbitShieldTargetedFlowGrayholeConfig");

namespace
{

std::vector<std::string>
DefaultGroundStations()
{
    return {"Tempe", "Fairbanks", "Svalbard", "Izhevsk", "Punta Arenas"};
}

std::vector<OrbitShieldTargetedFlowGrayholeGroundPair>
DefaultTrafficPairs()
{
    std::vector<OrbitShieldTargetedFlowGrayholeGroundPair> pairs;
    const auto stations = DefaultGroundStations();
    for (std::size_t sourceIndex = 0; sourceIndex < stations.size(); ++sourceIndex)
    {
        for (std::size_t destinationIndex = sourceIndex + 1;
             destinationIndex < stations.size();
             ++destinationIndex)
        {
            pairs.push_back({stations[sourceIndex], stations[destinationIndex]});
        }
    }
    return pairs;
}

OrbitShieldTargetedFlowGrayholeConfig
MakeDefaultConfig()
{
    OrbitShieldTargetedFlowGrayholeConfig config;
    config.constellation.ringFile = "../../../data/iridium-20260312.yaml";
    config.traffic.pairs = DefaultTrafficPairs();
    config.attack.compromisedSatellites = {"IRIDIUM 113"};
    config.attack.targetPairs = {{"Tempe", "Fairbanks"}};
    config.telemetry.outputDir = "results/targeted-flow-grayhole";
    return config;
}

void
SetError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage)
    {
        *errorMessage = message;
    }
}

bool
IsAbsolutePath(const std::string& path)
{
    return !path.empty() && path[0] == '/';
}

std::string
DirectoryName(const std::string& path)
{
    const auto slash = path.find_last_of("/\\");
    if (slash == std::string::npos)
    {
        return "";
    }
    return path.substr(0, slash + 1);
}

std::vector<std::string>
SplitPath(const std::string& path)
{
    std::vector<std::string> parts;
    std::string current;
    for (char character : path)
    {
        if (character == '/')
        {
            if (!current.empty())
            {
                parts.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(character);
    }
    if (!current.empty())
    {
        parts.push_back(current);
    }
    return parts;
}

std::string
NormalizePath(const std::string& path)
{
    const bool absolute = IsAbsolutePath(path);
    std::vector<std::string> normalized;
    for (const auto& part : SplitPath(path))
    {
        if (part == ".")
        {
            continue;
        }
        if (part == "..")
        {
            if (!normalized.empty() && normalized.back() != "..")
            {
                normalized.pop_back();
            }
            else if (!absolute)
            {
                normalized.push_back(part);
            }
            continue;
        }
        normalized.push_back(part);
    }

    std::ostringstream out;
    if (absolute)
    {
        out << '/';
    }
    for (std::size_t partIndex = 0; partIndex < normalized.size(); ++partIndex)
    {
        if (partIndex > 0)
        {
            out << '/';
        }
        out << normalized[partIndex];
    }
    const std::string result = out.str();
    return result.empty() ? (absolute ? "/" : ".") : result;
}

std::string
ResolvePath(const std::string& basePath, const std::string& path)
{
    if (path.empty() || IsAbsolutePath(path))
    {
        return NormalizePath(path);
    }
    return NormalizePath(basePath + path);
}

bool
FileIsReadable(const std::string& path)
{
    std::ifstream file(path);
    return file.good();
}

bool
RequireMap(const YAML::Node& node, const std::string& name, std::string* errorMessage)
{
    if (!node || node.IsMap())
    {
        return true;
    }
    SetError(errorMessage, name + " must be a YAML map");
    return false;
}

bool
LoadPairList(const YAML::Node& node,
             std::vector<OrbitShieldTargetedFlowGrayholeGroundPair>& pairs,
             const std::string& fieldName,
             std::string* errorMessage)
{
    if (!node)
    {
        return true;
    }
    if (!node.IsSequence())
    {
        SetError(errorMessage, fieldName + " must be a YAML sequence");
        return false;
    }

    std::vector<OrbitShieldTargetedFlowGrayholeGroundPair> parsed;
    for (const auto& pairNode : node)
    {
        if (!pairNode.IsMap() || !pairNode["source"] || !pairNode["destination"])
        {
            SetError(errorMessage, fieldName + " entries must contain source and destination");
            return false;
        }
        parsed.push_back({pairNode["source"].as<std::string>(),
                          pairNode["destination"].as<std::string>()});
    }
    pairs = std::move(parsed);
    return true;
}

bool
LoadStringList(const YAML::Node& node,
               std::vector<std::string>& values,
               const std::string& fieldName,
               std::string* errorMessage)
{
    if (!node)
    {
        return true;
    }
    if (!node.IsSequence())
    {
        SetError(errorMessage, fieldName + " must be a YAML sequence");
        return false;
    }
    std::vector<std::string> parsed;
    for (const auto& valueNode : node)
    {
        parsed.push_back(valueNode.as<std::string>());
    }
    values = std::move(parsed);
    return true;
}

bool
ParseDirection(const std::string& value,
               OrbitShieldTargetedFlowGrayholeDirection& direction,
               std::string* errorMessage)
{
    if (value == "forward")
    {
        direction = OrbitShieldTargetedFlowGrayholeDirection::FORWARD;
        return true;
    }
    if (value == "reverse")
    {
        direction = OrbitShieldTargetedFlowGrayholeDirection::REVERSE;
        return true;
    }
    if (value == "bidirectional")
    {
        direction = OrbitShieldTargetedFlowGrayholeDirection::BIDIRECTIONAL;
        return true;
    }
    SetError(errorMessage, "attack.direction must be forward, reverse, or bidirectional");
    return false;
}

bool
RequirePositive(double value, const std::string& fieldName, std::string* errorMessage)
{
    if (value > 0.0)
    {
        return true;
    }
    SetError(errorMessage, fieldName + " must be > 0");
    return false;
}

bool
RequireUnitInterval(double value, const std::string& fieldName, std::string* errorMessage)
{
    if (value >= 0.0 && value <= 1.0)
    {
        return true;
    }
    SetError(errorMessage, fieldName + " must be in [0, 1]");
    return false;
}

bool
ValidateRawValues(const OrbitShieldTargetedFlowGrayholeConfig& config, std::string* errorMessage)
{
    if (!RequirePositive(config.simulation.durationSeconds, "simulation.durationSeconds", errorMessage) ||
        !RequirePositive(config.topology.islMaxRangeMeters, "topology.islMaxRangeMeters", errorMessage) ||
        !RequirePositive(config.topology.groundMaxRangeMeters,
                         "topology.groundMaxRangeMeters",
                         errorMessage) ||
        !RequirePositive(config.topology.refreshIntervalSeconds,
                         "topology.refreshIntervalSeconds",
                         errorMessage) ||
        !RequirePositive(config.traffic.pingIntervalSeconds,
                         "traffic.pingIntervalSeconds",
                         errorMessage) ||
        !RequirePositive(config.detection.windowSeconds, "detection.windowSeconds", errorMessage) ||
        !RequirePositive(config.telemetry.routeSnapshotIntervalSeconds,
                         "telemetry.routeSnapshotIntervalSeconds",
                         errorMessage))
    {
        return false;
    }
    if (config.simulation.seed == 0 || config.simulation.run == 0)
    {
        SetError(errorMessage, "simulation.seed and simulation.run must be >= 1");
        return false;
    }
    if (config.traffic.pingSizeBytes == 0 || config.traffic.pingSizeBytes > 65507)
    {
        SetError(errorMessage, "traffic.pingSizeBytes must be in [1, 65507]");
        return false;
    }
    if (config.traffic.pairs.empty())
    {
        SetError(errorMessage, "traffic.pairs must not be empty");
        return false;
    }
    if (config.attack.compromisedSatellites.empty())
    {
        SetError(errorMessage, "attack.compromisedSatellites must not be empty");
        return false;
    }
    if (config.attack.targetPairs.empty())
    {
        SetError(errorMessage, "attack.targetPairs must not be empty");
        return false;
    }
    if (config.attack.startSeconds < 0.0 || config.attack.startSeconds >= config.simulation.durationSeconds)
    {
        SetError(errorMessage, "attack.startSeconds must be >= 0 and < simulation.durationSeconds");
        return false;
    }
    if (config.attack.stopSeconds <= config.attack.startSeconds ||
        config.attack.stopSeconds > config.simulation.durationSeconds)
    {
        SetError(errorMessage, "attack.stopSeconds must be > attack.startSeconds and <= simulation.durationSeconds");
        return false;
    }
    if (!RequireUnitInterval(config.attack.dropProbability, "attack.dropProbability", errorMessage) ||
        !RequireUnitInterval(config.detection.targetPdrThreshold,
                             "detection.targetPdrThreshold",
                             errorMessage))
    {
        return false;
    }
    if (config.detection.windowSeconds < config.topology.refreshIntervalSeconds)
    {
        SetError(errorMessage,
                 "detection.windowSeconds must be >= topology.refreshIntervalSeconds");
        return false;
    }
    if (config.detection.minSamples == 0)
    {
        SetError(errorMessage, "detection.minSamples must be >= 1");
        return false;
    }
    if (config.detection.scoreThreshold < 0.0)
    {
        SetError(errorMessage, "detection.scoreThreshold must be >= 0");
        return false;
    }
    if (config.mitigation.applyDelaySeconds < 0.0)
    {
        SetError(errorMessage, "mitigation.applyDelaySeconds must be >= 0");
        return false;
    }
    if (config.constellation.ringFile.empty() || !FileIsReadable(config.constellation.ringFile))
    {
        SetError(errorMessage, "constellation.ringFile must reference a readable YAML file");
        return false;
    }
    if (config.telemetry.outputDir.empty())
    {
        SetError(errorMessage, "telemetry.outputDir must not be empty");
        return false;
    }
    return true;
}

std::unordered_set<std::string>
GroundStationNames(Ptr<Constellation> constellation)
{
    std::unordered_set<std::string> names;
    if (!constellation)
    {
        return names;
    }
    for (const auto& station : constellation->GetGroundStations())
    {
        if (station)
        {
            names.insert(station->GetName());
        }
    }
    return names;
}

std::unordered_set<std::string>
SatelliteNames(Ptr<Constellation> constellation)
{
    std::unordered_set<std::string> names;
    if (!constellation)
    {
        return names;
    }
    for (const auto& satellite : constellation->GetSatellites())
    {
        if (satellite)
        {
            names.insert(satellite->GetName());
        }
    }
    return names;
}

bool
ValidateGroundPairNames(const std::vector<OrbitShieldTargetedFlowGrayholeGroundPair>& pairs,
                        const std::unordered_set<std::string>& groundNames,
                        const std::string& fieldName,
                        std::string* errorMessage)
{
    for (const auto& pair : pairs)
    {
        if (groundNames.count(pair.source) == 0)
        {
            SetError(errorMessage, fieldName + " references unknown source ground station " + pair.source);
            return false;
        }
        if (groundNames.count(pair.destination) == 0)
        {
            SetError(errorMessage,
                     fieldName + " references unknown destination ground station " + pair.destination);
            return false;
        }
    }
    return true;
}

std::string
PairKey(const OrbitShieldTargetedFlowGrayholeGroundPair& pair)
{
    return pair.source + "->" + pair.destination;
}

} // namespace

std::string
OrbitShieldTargetedFlowGrayholeDirectionToString(OrbitShieldTargetedFlowGrayholeDirection direction)
{
    switch (direction)
    {
    case OrbitShieldTargetedFlowGrayholeDirection::FORWARD:
        return "forward";
    case OrbitShieldTargetedFlowGrayholeDirection::REVERSE:
        return "reverse";
    case OrbitShieldTargetedFlowGrayholeDirection::BIDIRECTIONAL:
        return "bidirectional";
    }
    return "bidirectional";
}

bool
LoadOrbitShieldTargetedFlowGrayholeConfig(const std::string& filename,
                               OrbitShieldTargetedFlowGrayholeConfig& config,
                               std::string* errorMessage)
{
    NS_LOG_FUNCTION(filename);

    std::ifstream file(filename);
    if (!file.is_open())
    {
        SetError(errorMessage, "Could not open targeted-flow grayhole profile: " + filename);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string basePath = DirectoryName(filename);

    OrbitShieldTargetedFlowGrayholeConfig parsed = MakeDefaultConfig();
    parsed.profilePath = filename;

    try
    {
        YAML::Node root = YAML::Load(buffer.str());
        if (!root || !root.IsMap())
        {
            SetError(errorMessage, "targeted-flow grayhole profile must be a YAML map");
            return false;
        }

        const YAML::Node constellationNode = root["constellation"];
        if (!RequireMap(constellationNode, "constellation", errorMessage))
        {
            return false;
        }
        if (constellationNode && constellationNode["ringFile"])
        {
            parsed.constellation.ringFile = constellationNode["ringFile"].as<std::string>();
        }

        const YAML::Node simulationNode = root["simulation"];
        if (!RequireMap(simulationNode, "simulation", errorMessage))
        {
            return false;
        }
        if (simulationNode)
        {
            if (simulationNode["durationSeconds"])
            {
                parsed.simulation.durationSeconds = simulationNode["durationSeconds"].as<double>();
            }
            if (simulationNode["seed"])
            {
                parsed.simulation.seed = simulationNode["seed"].as<uint32_t>();
            }
            if (simulationNode["run"])
            {
                parsed.simulation.run = simulationNode["run"].as<uint32_t>();
            }
        }

        const YAML::Node topologyNode = root["topology"];
        if (!RequireMap(topologyNode, "topology", errorMessage))
        {
            return false;
        }
        if (topologyNode)
        {
            if (topologyNode["islMaxRangeMeters"])
            {
                parsed.topology.islMaxRangeMeters = topologyNode["islMaxRangeMeters"].as<double>();
            }
            if (topologyNode["groundMaxRangeMeters"])
            {
                parsed.topology.groundMaxRangeMeters = topologyNode["groundMaxRangeMeters"].as<double>();
            }
            if (topologyNode["refreshIntervalSeconds"])
            {
                parsed.topology.refreshIntervalSeconds = topologyNode["refreshIntervalSeconds"].as<double>();
            }
        }

        const YAML::Node trafficNode = root["traffic"];
        if (!RequireMap(trafficNode, "traffic", errorMessage))
        {
            return false;
        }
        if (trafficNode)
        {
            if (trafficNode["pingIntervalSeconds"])
            {
                parsed.traffic.pingIntervalSeconds = trafficNode["pingIntervalSeconds"].as<double>();
            }
            if (trafficNode["pingSizeBytes"])
            {
                parsed.traffic.pingSizeBytes = trafficNode["pingSizeBytes"].as<uint32_t>();
            }
            if (!LoadPairList(trafficNode["pairs"], parsed.traffic.pairs, "traffic.pairs", errorMessage))
            {
                return false;
            }
        }

        const YAML::Node attackNode = root["attack"];
        if (!RequireMap(attackNode, "attack", errorMessage))
        {
            return false;
        }
        if (attackNode)
        {
            if (!LoadStringList(attackNode["compromisedSatellites"],
                                parsed.attack.compromisedSatellites,
                                "attack.compromisedSatellites",
                                errorMessage) ||
                !LoadPairList(attackNode["targetPairs"],
                              parsed.attack.targetPairs,
                              "attack.targetPairs",
                              errorMessage))
            {
                return false;
            }
            if (attackNode["direction"] &&
                !ParseDirection(attackNode["direction"].as<std::string>(),
                                parsed.attack.direction,
                                errorMessage))
            {
                return false;
            }
            if (attackNode["startSeconds"])
            {
                parsed.attack.startSeconds = attackNode["startSeconds"].as<double>();
            }
            if (attackNode["stopSeconds"])
            {
                parsed.attack.stopSeconds = attackNode["stopSeconds"].as<double>();
            }
            if (attackNode["dropProbability"])
            {
                parsed.attack.dropProbability = attackNode["dropProbability"].as<double>();
            }
        }

        const YAML::Node detectionNode = root["detection"];
        if (!RequireMap(detectionNode, "detection", errorMessage))
        {
            return false;
        }
        if (detectionNode)
        {
            if (detectionNode["enabled"])
            {
                parsed.detection.enabled = detectionNode["enabled"].as<bool>();
            }
            if (detectionNode["windowSeconds"])
            {
                parsed.detection.windowSeconds = detectionNode["windowSeconds"].as<double>();
            }
            if (detectionNode["minSamples"])
            {
                parsed.detection.minSamples = detectionNode["minSamples"].as<uint32_t>();
            }
            if (detectionNode["targetPdrThreshold"])
            {
                parsed.detection.targetPdrThreshold = detectionNode["targetPdrThreshold"].as<double>();
            }
            if (detectionNode["scoreThreshold"])
            {
                parsed.detection.scoreThreshold = detectionNode["scoreThreshold"].as<double>();
            }
        }

        const YAML::Node mitigationNode = root["mitigation"];
        if (!RequireMap(mitigationNode, "mitigation", errorMessage))
        {
            return false;
        }
        if (mitigationNode)
        {
            if (mitigationNode["enabled"])
            {
                parsed.mitigation.enabled = mitigationNode["enabled"].as<bool>();
            }
            if (mitigationNode["applyDelaySeconds"])
            {
                parsed.mitigation.applyDelaySeconds = mitigationNode["applyDelaySeconds"].as<double>();
            }
            if (mitigationNode["maxExcludedSatellites"])
            {
                parsed.mitigation.maxExcludedSatellites = mitigationNode["maxExcludedSatellites"].as<uint32_t>();
            }
        }

        const YAML::Node telemetryNode = root["telemetry"];
        if (!RequireMap(telemetryNode, "telemetry", errorMessage))
        {
            return false;
        }
        if (telemetryNode)
        {
            if (telemetryNode["outputDir"])
            {
                parsed.telemetry.outputDir = telemetryNode["outputDir"].as<std::string>();
            }
            if (telemetryNode["routeSnapshotIntervalSeconds"])
            {
                parsed.telemetry.routeSnapshotIntervalSeconds =
                    telemetryNode["routeSnapshotIntervalSeconds"].as<double>();
            }
            if (telemetryNode["writeCsv"])
            {
                parsed.telemetry.writeCsv = telemetryNode["writeCsv"].as<bool>();
            }
        }
    }
    catch (const YAML::Exception& error)
    {
        SetError(errorMessage, std::string("Failed to parse targeted-flow grayhole profile: ") + error.what());
        return false;
    }

    parsed.constellation.ringFile = ResolvePath(basePath, parsed.constellation.ringFile);
    parsed.telemetry.outputDir = ResolvePath(basePath, parsed.telemetry.outputDir);

    if (!ValidateRawValues(parsed, errorMessage))
    {
        return false;
    }

    config = std::move(parsed);
    return true;
}

bool
ValidateOrbitShieldTargetedFlowGrayholeConfig(const OrbitShieldTargetedFlowGrayholeConfig& config,
                                   Ptr<Constellation> constellation,
                                   std::string* errorMessage)
{
    if (!constellation)
    {
        SetError(errorMessage, "Constellation must not be null");
        return false;
    }

    const auto groundNames = GroundStationNames(constellation);
    const auto satelliteNames = SatelliteNames(constellation);
    if (groundNames.empty())
    {
        SetError(errorMessage, "Constellation has no ground stations");
        return false;
    }
    if (satelliteNames.empty())
    {
        SetError(errorMessage, "Constellation has no satellites");
        return false;
    }

    if (!ValidateGroundPairNames(config.traffic.pairs, groundNames, "traffic.pairs", errorMessage) ||
        !ValidateGroundPairNames(config.attack.targetPairs,
                                 groundNames,
                                 "attack.targetPairs",
                                 errorMessage))
    {
        return false;
    }

    std::unordered_set<std::string> trafficPairKeys;
    for (const auto& pair : config.traffic.pairs)
    {
        trafficPairKeys.insert(PairKey(pair));
        trafficPairKeys.insert(PairKey({pair.destination, pair.source}));
    }
    for (const auto& pair : config.attack.targetPairs)
    {
        if (trafficPairKeys.count(PairKey(pair)) == 0)
        {
            SetError(errorMessage, "attack.targetPairs must be a subset of traffic.pairs");
            return false;
        }
    }

    for (const auto& satelliteName : config.attack.compromisedSatellites)
    {
        if (satelliteNames.count(satelliteName) == 0)
        {
            SetError(errorMessage,
                     "attack.compromisedSatellites references unknown satellite " + satelliteName);
            return false;
        }
    }

    return true;
}

} // namespace ns3