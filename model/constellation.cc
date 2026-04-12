/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "constellation.h"
#include "satellite-link.h"
#include "satellite-net-device.h"
#include "isl-propagation-delay-model.h"
#include "satellite-mobility-model.h"
#include "orbitshield-utils.h"

#include "ns3/log.h"

#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <map>
#include <unordered_map>
#include <cmath>
#include <iomanip>
#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Constellation");

NS_OBJECT_ENSURE_REGISTERED(Constellation);

struct TleData
{
    std::string name;
    std::string line1;
    std::string line2;
};

TypeId
Constellation::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Constellation")
                            .SetParent<Object>()
                            .SetGroupName("OrbitShield");
    return tid;
}

Constellation::Constellation()
{
    NS_LOG_FUNCTION(this);
}

Constellation::~Constellation()
{
    NS_LOG_FUNCTION(this);
}

void
Constellation::LoadFromTleFile(const std::string& filename)
{
    NS_LOG_FUNCTION(this << filename);

    std::ifstream file(filename);
    if (!file.is_open())
    {
        NS_LOG_ERROR("Could not open TLE file: " << filename);
        return;
    }

    LoadFromTleFile(file);

    file.close();
}

void
Constellation::LoadFromTleFile(std::istream& file)
{
    std::vector<TleData> tleDataList;
    std::string line;
    std::string currentName;
    std::string currentLine1;

    // First pass: collect all TLE data
    while (std::getline(file, line))
    {
        // Remove carriage return if present (Windows line endings)
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty())
            continue;

        // TLE entries start with "0" for name
        if (line[0] == '0')
        {
            currentName = line.substr(2); // Skip "0 "
        }
        // Line 1 starts with "1"
        else if (line[0] == '1' && !currentName.empty())
        {
            currentLine1 = line;
        }
        // Line 2 starts with "2"
        else if (line[0] == '2' && !currentName.empty() && !currentLine1.empty())
        {
            tleDataList.push_back({currentName, currentLine1, line});

            // Reset for next satellite
            currentName.clear();
            currentLine1.clear();
        }
    }

    if (tleDataList.empty())
    {
        NS_LOG_WARN("No valid TLE data found in input");
        return;
    }

    // Find the maximum epoch
    perturb::JulianDate maxEpoch(perturb::DateTime(1900, 1, 1, 0, 0, 0)); // Start with a very old date
    for (const auto& tleData : tleDataList)
    {
        std::string line1 = tleData.line1;
        std::string line2 = tleData.line2;
        perturb::Satellite tempSat = perturb::Satellite::from_tle(line1, line2);
        const perturb::JulianDate epoch = tempSat.epoch();
        if (epoch > maxEpoch)
        {
            maxEpoch = epoch;
        }
    }
    m_simulationStartJD = maxEpoch;

    // Second pass: create satellites
    for (auto& tleData : tleDataList)
    {
        Ptr<Satellite> satellite = CreateObject<Satellite>(tleData.name, tleData.line1, tleData.line2, m_simulationStartJD);
        m_satellites.push_back(satellite);

        NS_LOG_INFO("Loaded satellite: " << tleData.name);
    }

    NS_LOG_INFO("Loaded " << m_satellites.size() << " satellites with simulation start at " << JulianDateToString(m_simulationStartJD));
}

const std::vector<Ptr<Satellite>>&
Constellation::GetSatellites() const
{
    NS_LOG_FUNCTION(this);
    return m_satellites;
}

namespace
{
static inline void TrimString(std::string& s)
{
    auto notSpace = [](int ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
}

static std::vector<std::string>
SplitAndTrim(const std::string& s, char delim)
{
    std::vector<std::string> tokens;
    std::istringstream stream(s);
    std::string item;
    while (std::getline(stream, item, delim))
    {
        TrimString(item);
        if (!item.empty())
        {
            tokens.push_back(item);
        }
    }
    return tokens;
}
} // anonymous namespace

static const std::vector<Ptr<Satellite>>&
EmptySatelliteVector()
{
    static const std::vector<Ptr<Satellite>> empty;
    return empty;
}

void
Constellation::LoadFromRingFile(const std::string& filename)
{
    NS_LOG_FUNCTION(this << filename);

    // Extract directory from filename for resolving relative TLE paths
    std::string basePath;
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        basePath = filename.substr(0, lastSlash + 1);
    }

    std::ifstream file(filename);
    if (!file.is_open())
    {
        NS_LOG_ERROR("Could not open ring file: " << filename);
        return;
    }
    LoadFromRingFile(file, basePath);

    file.close();
}

void
Constellation::LoadFromRingFile(std::istream& file, const std::string& basePath)
{
    NS_LOG_FUNCTION(this);

    m_rings.clear();
    m_satelliteRingMap.clear();
    m_ringCount = 0;
    m_constellationName.clear();
    m_tleFile.clear();

    std::string line;
    std::map<uint32_t, std::vector<std::string>> ringNames;

    while (std::getline(file, line))
    {
        TrimString(line);
        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        TrimString(key);
        TrimString(value);

        if (key == "constellationName")
        {
            m_constellationName = value;
        }
        else if (key == "tleFile")
        {
            m_tleFile = value;
        }
        else if (key == "ringCount")
        {
            try
            {
                m_ringCount = static_cast<uint32_t>(std::stoul(value));
            }
            catch (...) {
                NS_LOG_WARN("Invalid ringCount value in ring file: " << value);
                m_ringCount = 0;
            }
        }
        else if (key.rfind("ring.", 0) == 0)
        {
            uint32_t ringId = 0;
            try
            {
                std::string idString = key.substr(5);
                ringId = static_cast<uint32_t>(std::stoul(idString));
            }
            catch (...) {
                NS_LOG_WARN("Invalid ring identifier in ring file: " << key);
                continue;
            }
            std::vector<std::string> names = SplitAndTrim(value, ',');
            for (const auto &satName : names)
            {
                if (ringNames[ringId].end() != std::find(ringNames[ringId].begin(), ringNames[ringId].end(), satName))
                {
                    continue;
                }
                ringNames[ringId].push_back(satName);
            }
        }
        else
        {
            NS_LOG_WARN("Unknown ring file key: " << key);
        }
    }

    // Load TLE data if specified
    if (!m_tleFile.empty())
    {
        std::string tlePath = basePath + m_tleFile;
        NS_LOG_INFO("Loading TLE data from: " << tlePath);
        m_satellites.clear(); // Clear existing satellites before loading new ones
        LoadFromTleFile(tlePath);
    }

    // Map TLE satellites into rings using existing m_satellites
    std::unordered_map<std::string, Ptr<Satellite>> nameToSat;
    for (const auto& sat : m_satellites)
    {
        nameToSat[sat->GetName()] = sat;
    }

    std::map<uint32_t, std::vector<Ptr<Satellite>>> realRings;

    for (const auto& [ringId, names] : ringNames)
    {
        for (const auto& name : names)
        {
            auto it = nameToSat.find(name);
            if (it != nameToSat.end())
            {
                realRings[ringId].push_back(it->second);
                m_satelliteRingMap[name] = ringId;
            }
            else
            {
                NS_LOG_WARN("Ring file references unknown satellite " << name);
            }
        }
    }

    m_rings = std::move(realRings);

    if (m_ringCount == 0)
    {
        m_ringCount = static_cast<uint32_t>(m_rings.size());
    }
}

uint32_t
Constellation::GetRingCount() const
{
    return m_ringCount;
}

std::optional<uint32_t>
Constellation::GetRingOfSatellite(const std::string& satName) const
{
    auto it = m_satelliteRingMap.find(satName);
    if (it == m_satelliteRingMap.end())
    {
        return std::nullopt;
    }
    return it->second;
}

const std::vector<Ptr<Satellite>>&
Constellation::GetSatellitesInRing(uint32_t ringId) const
{
    auto it = m_rings.find(ringId);
    return it == m_rings.end() ? EmptySatelliteVector() : it->second;
}

const std::vector<Ptr<Satellite>>&
Constellation::GetPreviousRingSatellites(uint32_t ringId) const
{
    if (m_ringCount == 0)
        return EmptySatelliteVector();

    uint32_t prev = (ringId + m_ringCount - 1) % m_ringCount;
    return GetSatellitesInRing(prev);
}

const std::vector<Ptr<Satellite>>&
Constellation::GetNextRingSatellites(uint32_t ringId) const
{
    if (m_ringCount == 0)
        return EmptySatelliteVector();

    uint32_t next = (ringId + 1) % m_ringCount;
    return GetSatellitesInRing(next);
}

std::vector<Ptr<SatelliteLink>>
Constellation::CreateIslLinks(double maxRange)
{
    NS_LOG_FUNCTION(this << maxRange);

    std::vector<Ptr<SatelliteLink>> links;

    for (std::size_t i = 0; i < m_satellites.size(); ++i)
    {
        for (std::size_t j = i + 1; j < m_satellites.size(); ++j)
        {
            Ptr<Satellite> satA = m_satellites[i];
            Ptr<Satellite> satB = m_satellites[j];

            Ptr<SatelliteNetDevice> devA = CreateObject<SatelliteNetDevice>();
            Ptr<SatelliteNetDevice> devB = CreateObject<SatelliteNetDevice>();

            devA->SetNode(satA);
            devB->SetNode(satB);

            if (!satA->GetObject<SatelliteMobilityModel>())
            {
                Ptr<SatelliteMobilityModel> mobility = CreateObject<SatelliteMobilityModel>();
                mobility->SetSatellite(satA);
                satA->AggregateObject(mobility);
            }
            if (!satB->GetObject<SatelliteMobilityModel>())
            {
                Ptr<SatelliteMobilityModel> mobility = CreateObject<SatelliteMobilityModel>();
                mobility->SetSatellite(satB);
                satB->AggregateObject(mobility);
            }

            satA->AddDevice(devA);
            satB->AddDevice(devB);

            Ptr<SatelliteLink> link = CreateObject<SatelliteLink>(devA, devB);
            link->SetMaxRange(maxRange);
            link->SetPropagationDelayModel(CreateObject<IslPropagationDelayModel>());

            links.push_back(link);
        }
    }

    return links;
}

std::string
Constellation::ExportIslAsDot(const std::vector<Ptr<SatelliteLink>>& links, bool activeOnly) const
{
    NS_LOG_FUNCTION(this << "links.size=" << links.size() << "activeOnly=" << activeOnly);

    std::ostringstream dot;

    // Start graph
    dot << "graph ISLTopology {\n";
    dot << "  layout=neato;\n";
    dot << "  outputorder=edgesfirst;\n";
    dot << "  splines=true;\n";
    dot << "  overlap=false;\n";
    dot << "  sep=1.0;\n";
    dot << "  node [shape=ellipse, style=filled, fillcolor=lightblue];\n";

    // Collect unique satellites and compute ground track positions
    std::map<std::string, Satellite::GroundTrackPosition> groundTrack;
    for (const auto& link : links)
    {
        if (activeOnly && !link->IsActive())
            continue;

        for (std::size_t i = 0; i < 2; ++i)
        {
            Ptr<NetDevice> device = link->GetDevice(i);
            if (device)
            {
                Ptr<Node> node = device->GetNode();
                if (node)
                {
                    Ptr<Satellite> satellite = DynamicCast<Satellite>(node);
                    if (satellite)
                    {
                        const std::string& satName = satellite->GetName();
                        if (groundTrack.find(satName) == groundTrack.end())
                        {
                            groundTrack[satName] = satellite->GetGroundTrackPosition();
                        }
                    }
                }
            }
        }
    }

    // Add satellite nodes with geographic positions for visualization
    for (const auto& [satName, gt] : groundTrack)
    {
        // map lon/lat into 2D layout coordinates
        double x = gt.longitude;
        double y = gt.latitude;
        constexpr double scale = 3.0;
        x *= scale;
        y *= scale;

        dot << "  \"" << satName << "\" [label=\"" << satName << "\", pos=\""
            << std::fixed << std::setprecision(2) << x << "," << y << "!\", tooltip=\"lat="
            << gt.latitude << " lon=" << gt.longitude << " alt=" << gt.altitude << "m\"];\n";
    }

    // Add ISL edges with distance labels
    for (const auto& link : links)
    {
        if (activeOnly && !link->IsActive())
            continue;

        Ptr<NetDevice> devA = link->GetDevice(0);
        Ptr<NetDevice> devB = link->GetDevice(1);

        if (!devA || !devB)
            continue;

        Ptr<Node> nodeA = devA->GetNode();
        Ptr<Node> nodeB = devB->GetNode();

        if (!nodeA || !nodeB)
            continue;

        Ptr<Satellite> satA = DynamicCast<Satellite>(nodeA);
        Ptr<Satellite> satB = DynamicCast<Satellite>(nodeB);

        if (!satA || !satB)
            continue;

        std::string nameA = satA->GetName();
        std::string nameB = satB->GetName();

        // Calculate distance
        Vector3D posA = satA->GetPosition();
        Vector3D posB = satB->GetPosition();
        double distance = std::sqrt(std::pow(posA.x - posB.x, 2) +
                                    std::pow(posA.y - posB.y, 2) +
                                    std::pow(posA.z - posB.z, 2));
        double distanceKm = distance / 1000.0;

        // Format distance label
        std::ostringstream distLabel;
        distLabel << std::fixed << std::setprecision(1) << distanceKm << " km";

        // Add edge
        dot << "  \"" << nameA << "\" -- \"" << nameB << "\" [label=\"" << distLabel.str() << "\"];\n";
    }

    // Close graph
    dot << "}\n";

    return dot.str();
}

}  // namespace ns3