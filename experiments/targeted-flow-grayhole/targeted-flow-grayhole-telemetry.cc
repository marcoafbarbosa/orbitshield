/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "targeted-flow-grayhole-telemetry.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

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

bool
DirectoryExists(const std::string& path)
{
    struct stat status;
    return stat(path.c_str(), &status) == 0 && S_ISDIR(status.st_mode);
}

bool
EnsureDirectory(const std::string& path, std::string* errorMessage)
{
    if (path.empty() || DirectoryExists(path))
    {
        return true;
    }

    std::string current;
    if (path.front() == '/')
    {
        current = "/";
    }

    std::size_t segmentStart = path.front() == '/' ? 1 : 0;
    while (segmentStart <= path.size())
    {
        const std::size_t slash = path.find('/', segmentStart);
        const std::string segment = path.substr(segmentStart, slash - segmentStart);
        if (!segment.empty())
        {
            if (!current.empty() && current.back() != '/')
            {
                current += '/';
            }
            current += segment;
            if (!DirectoryExists(current) && mkdir(current.c_str(), 0755) != 0 && errno != EEXIST)
            {
                SetError(errorMessage,
                         "Could not create telemetry directory " + current + ": " +
                             std::strerror(errno));
                return false;
            }
        }
        if (slash == std::string::npos)
        {
            break;
        }
        segmentStart = slash + 1;
    }
    return DirectoryExists(path);
}

std::string
CsvEscape(const std::string& value)
{
    const bool needsQuoting = value.find_first_of(",\n\"") != std::string::npos;
    if (!needsQuoting)
    {
        return value;
    }

    std::string escaped = "\"";
    for (char character : value)
    {
        if (character == '"')
        {
            escaped += "\"\"";
        }
        else
        {
            escaped.push_back(character);
        }
    }
    escaped += '"';
    return escaped;
}

std::string
JoinPath(const std::vector<std::string>& path)
{
    std::ostringstream out;
    for (std::size_t pathIndex = 0; pathIndex < path.size(); ++pathIndex)
    {
        if (pathIndex > 0)
        {
            out << '>';
        }
        out << path[pathIndex];
    }
    return out.str();
}

bool
OpenCsv(const std::string& outputDir,
        const std::string& filename,
        std::ofstream& output,
        std::string* errorMessage)
{
    output.open(outputDir + "/" + filename);
    if (!output.is_open())
    {
        SetError(errorMessage, "Could not open telemetry CSV " + outputDir + "/" + filename);
        return false;
    }
    return true;
}

} // namespace

void
OrbitShieldTargetedFlowGrayholeTelemetry::SetOutputDir(const std::string& outputDir)
{
    m_outputDir = outputDir;
}

const std::string&
OrbitShieldTargetedFlowGrayholeTelemetry::GetOutputDir() const
{
    return m_outputDir;
}

void
OrbitShieldTargetedFlowGrayholeTelemetry::SetWriteCsv(bool enabled)
{
    m_writeCsv = enabled;
}

bool
OrbitShieldTargetedFlowGrayholeTelemetry::GetWriteCsv() const
{
    return m_writeCsv;
}

void
OrbitShieldTargetedFlowGrayholeTelemetry::RecordFlowSample(Time time,
                                                const std::string& flowId,
                                                const std::string& source,
                                                const std::string& destination,
                                                uint32_t sent,
                                                uint32_t replies,
                                                Time rtt,
                                                Time attackStart,
                                                Time attackStop)
{
    OrbitShieldTargetedFlowGrayholeFlowSample sample;
    sample.time = time;
    sample.flowId = flowId;
    sample.source = source;
    sample.destination = destination;
    sample.sent = sent;
    sample.replies = replies;
    sample.pdr = sent == 0 ? 0.0 : static_cast<double>(replies) / static_cast<double>(sent);
    sample.rtt = rtt;
    sample.attackActive = IsAttackActive(time, attackStart, attackStop);
    m_flowSamples.push_back(sample);
}

void
OrbitShieldTargetedFlowGrayholeTelemetry::RecordRouteSnapshot(Time time,
                                                   const std::string& flowId,
                                                   const std::vector<std::string>& path)
{
    m_routeSnapshots.push_back({time, flowId, path});
}

void
OrbitShieldTargetedFlowGrayholeTelemetry::RecordForwardingEvent(Time time,
                                                     uint32_t nodeId,
                                                     const std::string& nodeName,
                                                     Ipv4Address source,
                                                     Ipv4Address destination,
                                                     const std::string& targetPairId,
                                                     const std::string& reason,
                                                     bool dropped)
{
    m_forwardingEvents.push_back({time,
                                  nodeId,
                                  nodeName,
                                  source,
                                  destination,
                                  targetPairId,
                                  reason,
                                  dropped});
}

void
OrbitShieldTargetedFlowGrayholeTelemetry::RecordNodeLabel(Time time,
                                               const std::string& nodeName,
                                               bool compromised,
                                               bool flagged)
{
    m_nodeLabels.push_back({time, nodeName, compromised, flagged});
}

void
OrbitShieldTargetedFlowGrayholeTelemetry::RecordMitigationEvent(Time time,
                                                     const std::string& nodeName,
                                                     const std::string& action,
                                                     const std::string& reason)
{
    m_mitigationEvents.push_back({time, nodeName, action, reason});
}

const std::vector<OrbitShieldTargetedFlowGrayholeFlowSample>&
OrbitShieldTargetedFlowGrayholeTelemetry::GetFlowSamples() const
{
    return m_flowSamples;
}

const std::vector<OrbitShieldTargetedFlowGrayholeRouteSnapshot>&
OrbitShieldTargetedFlowGrayholeTelemetry::GetRouteSnapshots() const
{
    return m_routeSnapshots;
}

const std::vector<OrbitShieldTargetedFlowGrayholeForwardingEvent>&
OrbitShieldTargetedFlowGrayholeTelemetry::GetForwardingEvents() const
{
    return m_forwardingEvents;
}

const std::vector<OrbitShieldTargetedFlowGrayholeNodeLabel>&
OrbitShieldTargetedFlowGrayholeTelemetry::GetNodeLabels() const
{
    return m_nodeLabels;
}

const std::vector<OrbitShieldTargetedFlowGrayholeMitigationEvent>&
OrbitShieldTargetedFlowGrayholeTelemetry::GetMitigationEvents() const
{
    return m_mitigationEvents;
}

bool
OrbitShieldTargetedFlowGrayholeTelemetry::WriteCsv(std::string* errorMessage) const
{
    if (!m_writeCsv)
    {
        return true;
    }
    if (m_outputDir.empty())
    {
        SetError(errorMessage, "Telemetry output directory is empty");
        return false;
    }
    if (!EnsureDirectory(m_outputDir, errorMessage))
    {
        return false;
    }

    std::ofstream flowSamples;
    if (!OpenCsv(m_outputDir, "flow_samples.csv", flowSamples, errorMessage))
    {
        return false;
    }
    flowSamples << "time_seconds,flow_id,source,destination,sent,replies,pdr,rtt_ms,attack_active\n";
    for (const auto& sample : m_flowSamples)
    {
        flowSamples << sample.time.GetSeconds() << ',' << CsvEscape(sample.flowId) << ','
                    << CsvEscape(sample.source) << ',' << CsvEscape(sample.destination) << ','
                    << sample.sent << ',' << sample.replies << ',' << sample.pdr << ','
                    << sample.rtt.GetMilliSeconds() << ',' << (sample.attackActive ? 1 : 0) << '\n';
    }

    std::ofstream routeSnapshots;
    if (!OpenCsv(m_outputDir, "route_snapshots.csv", routeSnapshots, errorMessage))
    {
        return false;
    }
    routeSnapshots << "time_seconds,flow_id,path\n";
    for (const auto& snapshot : m_routeSnapshots)
    {
        routeSnapshots << snapshot.time.GetSeconds() << ',' << CsvEscape(snapshot.flowId) << ','
                       << CsvEscape(JoinPath(snapshot.path)) << '\n';
    }

    std::ofstream forwardingEvents;
    if (!OpenCsv(m_outputDir, "forwarding_events.csv", forwardingEvents, errorMessage))
    {
        return false;
    }
    forwardingEvents << "time_seconds,node_id,node_name,source,destination,target_pair_id,reason,dropped\n";
    for (const auto& event : m_forwardingEvents)
    {
        forwardingEvents << event.time.GetSeconds() << ',' << event.nodeId << ','
                         << CsvEscape(event.nodeName) << ',' << event.source << ','
                         << event.destination << ',' << CsvEscape(event.targetPairId) << ','
                         << CsvEscape(event.reason) << ',' << (event.dropped ? 1 : 0) << '\n';
    }

    std::ofstream nodeLabels;
    if (!OpenCsv(m_outputDir, "node_labels.csv", nodeLabels, errorMessage))
    {
        return false;
    }
    nodeLabels << "time_seconds,node_name,compromised,flagged\n";
    for (const auto& label : m_nodeLabels)
    {
        nodeLabels << label.time.GetSeconds() << ',' << CsvEscape(label.nodeName) << ','
                   << (label.compromised ? 1 : 0) << ',' << (label.flagged ? 1 : 0) << '\n';
    }

    std::ofstream mitigationEvents;
    if (!OpenCsv(m_outputDir, "mitigation_events.csv", mitigationEvents, errorMessage))
    {
        return false;
    }
    mitigationEvents << "time_seconds,node_name,action,reason\n";
    for (const auto& event : m_mitigationEvents)
    {
        mitigationEvents << event.time.GetSeconds() << ',' << CsvEscape(event.nodeName) << ','
                         << CsvEscape(event.action) << ',' << CsvEscape(event.reason) << '\n';
    }

    return true;
}

bool
OrbitShieldTargetedFlowGrayholeTelemetry::IsAttackActive(Time time, Time attackStart, Time attackStop)
{
    return time >= attackStart && time < attackStop;
}

} // namespace ns3