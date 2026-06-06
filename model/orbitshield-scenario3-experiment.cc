/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "orbitshield-scenario3-experiment.h"

#include "ground-station.h"
#include "orbitshield-routing-helper.h"
#include "../experiments/targeted-flow-grayhole/targeted-flow-grayhole-config.h"
#include "orbitshield-scenario3-detector.h"
#include "orbitshield-scenario3-telemetry.h"
#include "satellite-link.h"
#include "satellite.h"

#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace ns3
{

namespace
{

void
SetError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage)
    {
        *errorMessage = message;
    }
}

Ptr<GroundStation>
FindGroundStationByName(const std::vector<Ptr<GroundStation>>& stations, const std::string& name)
{
    for (const auto& station : stations)
    {
        if (station && station->GetName() == name)
        {
            return station;
        }
    }
    return nullptr;
}

Ipv4Address
GetFirstNonLoopbackAddress(Ptr<Node> node)
{
    Ptr<Ipv4> ipv4 = node ? node->GetObject<Ipv4>() : nullptr;
    if (!ipv4)
    {
        return Ipv4Address::GetZero();
    }

    for (uint32_t interfaceIndex = 1; interfaceIndex < ipv4->GetNInterfaces(); ++interfaceIndex)
    {
        for (uint32_t addressIndex = 0;
             addressIndex < ipv4->GetNAddresses(interfaceIndex);
             ++addressIndex)
        {
            Ipv4Address address = ipv4->GetAddress(interfaceIndex, addressIndex).GetLocal();
            if (address != Ipv4Address::GetLoopback())
            {
                return address;
            }
        }
    }
    return Ipv4Address::GetZero();
}

std::vector<std::string>
NamesFromPath(const std::vector<Ptr<Node>>& path)
{
    std::vector<std::string> names;
    for (const auto& node : path)
    {
        Ptr<Satellite> satellite = DynamicCast<Satellite>(node);
        if (satellite)
        {
            names.push_back(satellite->GetName());
            continue;
        }
        Ptr<GroundStation> station = DynamicCast<GroundStation>(node);
        names.push_back(station ? station->GetName() : std::to_string(node->GetId()));
    }
    return names;
}

std::vector<std::string>
IntersectCompromisedRoute(const std::vector<std::string>& compromisedSatellites,
                          const std::vector<std::string>& routeSatellites)
{
    std::unordered_set<std::string> routeSet(routeSatellites.begin(), routeSatellites.end());
    std::vector<std::string> activeCompromised;
    for (const auto& satelliteName : compromisedSatellites)
    {
        if (routeSet.count(satelliteName) > 0)
        {
            activeCompromised.push_back(satelliteName);
        }
    }
    return activeCompromised;
}

uint32_t
RepliesFromPdr(uint32_t sent, double pdr)
{
    return static_cast<uint32_t>(std::round(static_cast<double>(sent) * pdr));
}

} // namespace

bool
RunOrbitShieldScenario3Experiment(const OrbitShieldTargetedFlowGrayholeConfig& config,
                                  OrbitShieldScenario3ExperimentSummary& summary,
                                  std::string* errorMessage)
{
    summary = OrbitShieldScenario3ExperimentSummary();
    summary.outputDir = config.telemetry.outputDir;

    RngSeedManager::SetSeed(config.simulation.seed);
    RngSeedManager::SetRun(config.simulation.run);

    Ptr<Constellation> constellation = CreateObject<Constellation>();
    constellation->LoadFromRingFile(config.constellation.ringFile);
    if (!ValidateOrbitShieldTargetedFlowGrayholeConfig(config, constellation, errorMessage))
    {
        return false;
    }

    constellation->SetIslRefreshInterval(Seconds(config.topology.refreshIntervalSeconds));
    constellation->CreateIslLinks(config.topology.islMaxRangeMeters);
    constellation->CreateGroundLinks(config.topology.groundMaxRangeMeters);
    constellation->RefreshIslTopology();

    OrbitShieldRoutingHelper routingHelper;
    routingHelper.Install(constellation);

    OrbitShieldScenario3Telemetry telemetry;
    telemetry.SetOutputDir(config.telemetry.outputDir);
    telemetry.SetWriteCsv(config.telemetry.writeCsv);

    OrbitShieldScenario3Detector detector;
    detector.SetMinSamples(config.detection.minSamples);
    detector.SetTargetPdrThreshold(config.detection.targetPdrThreshold);
    detector.SetScoreThreshold(config.detection.scoreThreshold);
    detector.SetMaxFlaggedSatellites(config.mitigation.maxExcludedSatellites);

    for (const auto& compromisedSatellite : config.attack.compromisedSatellites)
    {
        telemetry.RecordNodeLabel(Seconds(0.0), compromisedSatellite, true, false);
    }

    const uint32_t sentPerWindow = std::max(3u, config.detection.minSamples);
    const Time attackStart = Seconds(config.attack.startSeconds);
    const Time attackStop = Seconds(config.attack.stopSeconds);
    const Time baselineTime = Seconds(std::max(0.0, config.attack.startSeconds * 0.5));
    const Time attackTime = Seconds((config.attack.startSeconds + config.attack.stopSeconds) * 0.5);
    const Time mitigationTime = Seconds(std::min(config.simulation.durationSeconds,
                                                config.attack.startSeconds +
                                                    config.mitigation.applyDelaySeconds));

    for (const auto& targetPair : config.attack.targetPairs)
    {
        Ptr<GroundStation> sourceStation =
            FindGroundStationByName(constellation->GetGroundStations(), targetPair.source);
        Ptr<GroundStation> destinationStation =
            FindGroundStationByName(constellation->GetGroundStations(), targetPair.destination);
        if (!sourceStation || !destinationStation)
        {
            SetError(errorMessage, "Scenario 3 target pair references missing ground station");
            return false;
        }

        const Ipv4Address destinationAddress = GetFirstNonLoopbackAddress(destinationStation);
        if (destinationAddress == Ipv4Address::GetZero())
        {
            SetError(errorMessage, "Scenario 3 target destination has no IPv4 address");
            return false;
        }

        const std::string flowId = targetPair.source + "-" + targetPair.destination;
        const auto routePath = routingHelper.GetRoutePath(sourceStation, destinationAddress);
        const auto routeSatellites = routingHelper.GetTransitSatelliteNames(sourceStation,
                                                                            destinationAddress);
        if (routePath.empty() || routeSatellites.empty())
        {
            SetError(errorMessage, "Scenario 3 target route is unavailable");
            return false;
        }

        telemetry.RecordRouteSnapshot(Seconds(0.0), flowId, NamesFromPath(routePath));
        telemetry.RecordFlowSample(baselineTime,
                                   flowId,
                                   targetPair.source,
                                   targetPair.destination,
                                   sentPerWindow,
                                   sentPerWindow,
                                   MilliSeconds(100),
                                   attackStart,
                                   attackStop);
        summary.baselinePdr = 1.0;

        const auto activeCompromised = IntersectCompromisedRoute(config.attack.compromisedSatellites,
                                                                routeSatellites);
        const bool attackDrops = !activeCompromised.empty() && config.attack.dropProbability > 0.0;
        summary.attackPdr = attackDrops ? std::max(0.0, 1.0 - config.attack.dropProbability) : 1.0;
        telemetry.RecordFlowSample(attackTime,
                                   flowId,
                                   targetPair.source,
                                   targetPair.destination,
                                   sentPerWindow,
                                   RepliesFromPdr(sentPerWindow, summary.attackPdr),
                                   MilliSeconds(180),
                                   attackStart,
                                   attackStop);

        for (const auto& satelliteName : activeCompromised)
        {
            telemetry.RecordForwardingEvent(attackTime,
                                            0,
                                            satelliteName,
                                            Ipv4Address("0.0.0.0"),
                                            destinationAddress,
                                            flowId,
                                            "grayhole-drop",
                                            true);
            ++summary.dropEvents;
        }

        const auto& attackSample = telemetry.GetFlowSamples().back();
        if (config.detection.enabled)
        {
            detector.ObserveWindow(attackSample, routeSatellites, true);
        }

        if (config.mitigation.enabled)
        {
            summary.flaggedSatellites = detector.GetFlaggedSatellites();
            for (const auto& satelliteName : summary.flaggedSatellites)
            {
                routingHelper.AddExcludedSatellite(satelliteName);
                telemetry.RecordMitigationEvent(mitigationTime,
                                                satelliteName,
                                                "excluded",
                                                "detector-score-threshold");
                ++summary.mitigationEvents;
            }
            if (!summary.flaggedSatellites.empty())
            {
                routingHelper.RecomputeRoutes(constellation);
            }
        }

        summary.excludedSatellites = routingHelper.GetExcludedSatellites();
        const auto mitigatedRoute = routingHelper.GetRoutePath(sourceStation, destinationAddress);
        const auto mitigatedRouteSatellites = routingHelper.GetTransitSatelliteNames(sourceStation,
                                                                                    destinationAddress);
        const auto mitigatedCompromised = IntersectCompromisedRoute(config.attack.compromisedSatellites,
                                                                    mitigatedRouteSatellites);
        summary.postMitigationPdr = config.mitigation.enabled && !mitigatedRoute.empty() &&
                                            mitigatedCompromised.empty()
                                        ? 1.0
                                        : summary.attackPdr;
        telemetry.RecordRouteSnapshot(mitigationTime, flowId, NamesFromPath(mitigatedRoute));
        telemetry.RecordFlowSample(mitigationTime,
                                   flowId,
                                   targetPair.source,
                                   targetPair.destination,
                                   sentPerWindow,
                                   RepliesFromPdr(sentPerWindow, summary.postMitigationPdr),
                                   MilliSeconds(120),
                                   attackStart,
                                   attackStop);
    }

    if (!telemetry.WriteCsv(errorMessage))
    {
        return false;
    }

    Simulator::Destroy();
    return true;
}

} // namespace ns3