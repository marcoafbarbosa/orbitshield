/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_SCENARIO3_TELEMETRY_H
#define ORBITSHIELD_SCENARIO3_TELEMETRY_H

#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ns3
{

struct OrbitShieldScenario3FlowSample
{
    Time time;
    std::string flowId;
    std::string source;
    std::string destination;
    uint32_t sent{0};
    uint32_t replies{0};
    double pdr{0.0};
    Time rtt{Seconds(0)};
    bool attackActive{false};
};

struct OrbitShieldScenario3RouteSnapshot
{
    Time time;
    std::string flowId;
    std::vector<std::string> path;
};

struct OrbitShieldScenario3ForwardingEvent
{
    Time time;
    uint32_t nodeId{0};
    std::string nodeName;
    Ipv4Address source;
    Ipv4Address destination;
    std::string targetPairId;
    std::string reason;
    bool dropped{false};
};

struct OrbitShieldScenario3NodeLabel
{
    Time time;
    std::string nodeName;
    bool compromised{false};
    bool flagged{false};
};

struct OrbitShieldScenario3MitigationEvent
{
    Time time;
    std::string nodeName;
    std::string action;
    std::string reason;
};

class OrbitShieldScenario3Telemetry
{
  public:
    void SetOutputDir(const std::string& outputDir);
    const std::string& GetOutputDir() const;
    void SetWriteCsv(bool enabled);
    bool GetWriteCsv() const;

    void RecordFlowSample(Time time,
                          const std::string& flowId,
                          const std::string& source,
                          const std::string& destination,
                          uint32_t sent,
                          uint32_t replies,
                          Time rtt,
                          Time attackStart,
                          Time attackStop);
    void RecordRouteSnapshot(Time time,
                             const std::string& flowId,
                             const std::vector<std::string>& path);
    void RecordForwardingEvent(Time time,
                               uint32_t nodeId,
                               const std::string& nodeName,
                               Ipv4Address source,
                               Ipv4Address destination,
                               const std::string& targetPairId,
                               const std::string& reason,
                               bool dropped);
    void RecordNodeLabel(Time time,
                         const std::string& nodeName,
                         bool compromised,
                         bool flagged);
    void RecordMitigationEvent(Time time,
                               const std::string& nodeName,
                               const std::string& action,
                               const std::string& reason);

    const std::vector<OrbitShieldScenario3FlowSample>& GetFlowSamples() const;
    const std::vector<OrbitShieldScenario3RouteSnapshot>& GetRouteSnapshots() const;
    const std::vector<OrbitShieldScenario3ForwardingEvent>& GetForwardingEvents() const;
    const std::vector<OrbitShieldScenario3NodeLabel>& GetNodeLabels() const;
    const std::vector<OrbitShieldScenario3MitigationEvent>& GetMitigationEvents() const;

    bool WriteCsv(std::string* errorMessage = nullptr) const;
    static bool IsAttackActive(Time time, Time attackStart, Time attackStop);

  private:
    std::string m_outputDir;
    bool m_writeCsv{true};
    std::vector<OrbitShieldScenario3FlowSample> m_flowSamples;
    std::vector<OrbitShieldScenario3RouteSnapshot> m_routeSnapshots;
    std::vector<OrbitShieldScenario3ForwardingEvent> m_forwardingEvents;
    std::vector<OrbitShieldScenario3NodeLabel> m_nodeLabels;
    std::vector<OrbitShieldScenario3MitigationEvent> m_mitigationEvents;
};

} // namespace ns3

#endif /* ORBITSHIELD_SCENARIO3_TELEMETRY_H */