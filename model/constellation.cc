/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "constellation.h"
#include "satellite-link.h"
#include "satellite-net-device.h"
#include "isl-propagation-delay-model.h"
#include "satellite-mobility-model.h"
#include "orbitshield-utils.h"

#include "ns3/constant-position-mobility-model.h"
#include "ns3/geographic-positions.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

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

    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty())
        {
            continue;
        }

        if (line[0] == '0')
        {
            currentName = line.substr(2);
        }
        else if (line[0] == '1' && !currentName.empty())
        {
            currentLine1 = line;
        }
        else if (line[0] == '2' && !currentName.empty() && !currentLine1.empty())
        {
            tleDataList.push_back({currentName, currentLine1, line});
            currentName.clear();
            currentLine1.clear();
        }
    }

    if (tleDataList.empty())
    {
        NS_LOG_WARN("No valid TLE data found in input");
        return;
    }

    perturb::JulianDate maxEpoch(perturb::DateTime(1900, 1, 1, 0, 0, 0));
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

    for (auto& tleData : tleDataList)
    {
        Ptr<Satellite> satellite =
            CreateObject<Satellite>(tleData.name, tleData.line1, tleData.line2, m_simulationStartJD);
        m_satellites.push_back(satellite);

        NS_LOG_INFO("Loaded satellite: " << tleData.name);
    }

    NS_LOG_INFO("Loaded " << m_satellites.size()
                           << " satellites with simulation start at "
                           << JulianDateToString(m_simulationStartJD));
}

const std::vector<Ptr<Satellite>>&
Constellation::GetSatellites() const
{
    NS_LOG_FUNCTION(this);
    return m_satellites;
}

namespace
{
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
    m_groundStations.clear();

    std::map<uint32_t, std::vector<std::string>> ringNames;
    std::stringstream buffer;
    buffer << file.rdbuf();

    try
    {
        YAML::Node root = YAML::Load(buffer.str());

        if (root["constellationName"])
        {
            m_constellationName = root["constellationName"].as<std::string>();
        }

        if (root["tleFile"])
        {
            m_tleFile = root["tleFile"].as<std::string>();
        }

        if (root["ringCount"])
        {
            m_ringCount = root["ringCount"].as<uint32_t>();
        }

        if (const YAML::Node ringsNode = root["rings"])
        {
            if (!ringsNode.IsSequence())
            {
                NS_LOG_WARN("'rings' must be a YAML sequence");
            }
            else
            {
                for (const auto& ringNode : ringsNode)
                {
                    if (!ringNode["id"])
                    {
                        NS_LOG_WARN("Ignoring ring entry without 'id'");
                        continue;
                    }

                    uint32_t ringId = ringNode["id"].as<uint32_t>();
                    auto& names = ringNames[ringId];

                    const YAML::Node satellitesNode = ringNode["satellites"];
                    if (!satellitesNode || !satellitesNode.IsSequence())
                    {
                        NS_LOG_WARN("Ignoring ring " << ringId << " without a 'satellites' sequence");
                        continue;
                    }

                    for (const auto& satNode : satellitesNode)
                    {
                        const std::string satName = satNode.as<std::string>();
                        if (std::find(names.begin(), names.end(), satName) == names.end())
                        {
                            names.push_back(satName);
                        }
                    }
                }
            }
        }

        if (const YAML::Node groundStationsNode = root["groundStations"])
        {
            if (!groundStationsNode.IsSequence())
            {
                NS_LOG_WARN("'groundStations' must be a YAML sequence");
            }
            else
            {
                for (const auto& stationNode : groundStationsNode)
                {
                    if (!stationNode["name"] || !stationNode["latitude"] || !stationNode["longitude"])
                    {
                        NS_LOG_WARN("Ignoring incomplete ground station entry in YAML");
                        continue;
                    }

                    Ptr<GroundStation> station = CreateObject<GroundStation>();
                    station->SetName(stationNode["name"].as<std::string>());
                    station->SetLatitude(stationNode["latitude"].as<double>());
                    station->SetLongitude(stationNode["longitude"].as<double>());
                    m_groundStations.push_back(station);
                }
            }
        }
    }
    catch (const YAML::Exception& error)
    {
        NS_LOG_ERROR("Failed to parse constellation YAML: " << error.what());
        return;
    }

    if (!m_tleFile.empty())
    {
        std::string tlePath = basePath + m_tleFile;
        NS_LOG_INFO("Loading TLE data from: " << tlePath);
        m_satellites.clear();
        LoadFromTleFile(tlePath);
    }

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

std::string
Constellation::GetConstellationName() const
{
    return m_constellationName;
}

const std::vector<Ptr<GroundStation>>&
Constellation::GetGroundStations() const
{
    return m_groundStations;
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

    // Cache ISL range for use in periodic refresh.
    m_islMaxRange = maxRange;

    std::vector<Ptr<SatelliteLink>> links;

    // Build potential connection candidates for each satellite
    std::unordered_map<Ptr<Satellite>, std::vector<Ptr<Satellite>>> potentialConnections;

    if (m_rings.empty())
    {
        NS_LOG_WARN("No ring structure available for ISL topology creation; falling back to nearest-neighbor topology");

        for (const auto& sat : m_satellites)
        {
            std::vector<std::pair<double, Ptr<Satellite>>> distances;
            for (const auto& other : m_satellites)
            {
                if (sat == other)
                {
                    continue;
                }
                distances.emplace_back(CalculateSatelliteDistance(sat, other), other);
            }

            std::sort(distances.begin(), distances.end(), [](const auto& a, const auto& b) {
                return a.first < b.first;
            });

            for (std::size_t k = 0; k < std::min<std::size_t>(2, distances.size()); ++k)
            {
                potentialConnections[sat].push_back(distances[k].second);
            }
        }
    }
    else
    {
        for (const auto& [ringId, ringSats] : m_rings)
    {
        if (ringSats.empty())
            continue;

        // Intra-ring connections: each satellite connects to the two closest satellites
        for (std::size_t i = 0; i < ringSats.size(); ++i)
        {
            Ptr<Satellite> sat = ringSats[i];
            std::vector<std::pair<double, Ptr<Satellite>>> distances;

            // Calculate distances to all other satellites in the same ring
            for (std::size_t j = 0; j < ringSats.size(); ++j)
            {
                if (i == j)
                    continue;

                Ptr<Satellite> otherSat = ringSats[j];
                double distance = CalculateSatelliteDistance(sat, otherSat);
                distances.emplace_back(distance, otherSat);
            }

            // Sort by distance and take the two closest
            std::sort(distances.begin(), distances.end());
            for (std::size_t k = 0; k < std::min(std::size_t(2), distances.size()); ++k)
            {
                potentialConnections[sat].push_back(distances[k].second);
            }
        }

        // Inter-ring connections: each satellite connects to closest in adjacent rings
        std::vector<Ptr<Satellite>> prevRing = GetPreviousRingSatellites(ringId);
        std::vector<Ptr<Satellite>> nextRing = GetNextRingSatellites(ringId);

        for (Ptr<Satellite> sat : ringSats)
        {
            // Find closest satellite in previous ring
            if (!prevRing.empty())
            {
                Ptr<Satellite> closestPrev = FindClosestSatellite(sat, prevRing);
                if (closestPrev)
                {
                    potentialConnections[sat].push_back(closestPrev);
                }
            }

            // Find closest satellite in next ring
            if (!nextRing.empty())
            {
                Ptr<Satellite> closestNext = FindClosestSatellite(sat, nextRing);
                if (closestNext)
                {
                    potentialConnections[sat].push_back(closestNext);
                }
            }
        }
    }
    }

    // Remove duplicate candidate entries for each satellite.
    for (auto& [sat, candidates] : potentialConnections)
    {
        std::sort(candidates.begin(), candidates.end(), [](const Ptr<Satellite>& a, const Ptr<Satellite>& b) {
            return a->GetName() < b->GetName();
        });
        candidates.erase(std::unique(candidates.begin(), candidates.end(), [](const Ptr<Satellite>& a, const Ptr<Satellite>& b) {
            return a->GetName() == b->GetName();
        }), candidates.end());
    }

    // Now establish bidirectional connections
    std::set<std::pair<std::string, std::string>> processedPairs;

    for (const auto& [satA, candidatesA] : potentialConnections)
    {
        for (Ptr<Satellite> satB : candidatesA)
        {
            if (!satB)
                continue;

            const std::string nameA = satA->GetName();
            const std::string nameB = satB->GetName();
            std::pair<std::string, std::string> pairKey = nameA < nameB ? std::make_pair(nameA, nameB)
                                                                         : std::make_pair(nameB, nameA);

            if (processedPairs.count(pairKey))
                continue;

            processedPairs.insert(pairKey);

            // Check if satB also has satA in its potential connections
            auto it = potentialConnections.find(satB);
            if (it != potentialConnections.end())
            {
                const auto& candidatesB = it->second;
                if (std::find(candidatesB.begin(), candidatesB.end(), satA) != candidatesB.end())
                {
                    // Both satellites have each other in their potential lists, create the link
                    if (CreateIslLink(satA, satB, maxRange))
                    {
                        Ptr<SatelliteNetDevice> devA = GetOrCreateSatelliteNetDevice(satA);
                        Ptr<SatelliteNetDevice> devB = GetOrCreateSatelliteNetDevice(satB);
                        if (devA && devB)
                        {
                            Ptr<SatelliteLink> link = CreateObject<SatelliteLink>(devA, devB);
                            link->SetMaxRange(maxRange);
                            link->SetPropagationDelayModel(CreateObject<IslPropagationDelayModel>());
                            links.push_back(link);
                        }
                    }
                }
            }
        }
    }

    NS_LOG_INFO("Created " << links.size() << " ISL links based on ring topology");

    // Schedule automatic topology refresh driven by the ns-3 Simulator.
    ScheduleTopologyRefresh();

    return links;
}

std::vector<Ptr<SatelliteLink>>
Constellation::CreateGroundLinks(double maxRange)
{
    NS_LOG_FUNCTION(this << maxRange);

    // Cache ground-link range for use in periodic refresh.
    m_groundMaxRange = maxRange;

    std::vector<Ptr<SatelliteLink>> links;
    for (const auto& station : m_groundStations)
    {
        if (!station)
        {
            continue;
        }

        for (const auto& sat : m_satellites)
        {
            if (!sat)
            {
                continue;
            }

            // When ring metadata is loaded, skip satellites not assigned to any ring.
            if (!m_satelliteRingMap.empty() &&
                m_satelliteRingMap.find(sat->GetName()) == m_satelliteRingMap.end())
            {
                continue;
            }

            if (CreateGroundLink(sat, station, maxRange))
            {
                Ptr<SatelliteNetDevice> satDev = GetOrCreateSatelliteNetDevice(sat);
                Ptr<SatelliteNetDevice> gsDev = GetOrCreateGroundStationNetDevice(station);
                if (satDev && gsDev)
                {
                    Ptr<SatelliteLink> link = CreateObject<SatelliteLink>(satDev, gsDev);
                    link->SetMaxRange(maxRange);
                    link->SetPropagationDelayModel(CreateObject<IslPropagationDelayModel>());
                    links.push_back(link);
                }
            }
        }
    }

    NS_LOG_INFO("Created " << links.size() << " satellite-ground links");

    ScheduleTopologyRefresh();

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

    // Define color palette for rings
    std::vector<std::string> ringColors = {
        "lightblue", "lightgreen", "lightcoral", "lightgoldenrod", "lightpink",
        "lightcyan", "lightsalmon", "lightseagreen", "lightsteelblue", "lightyellow",
        "lavender", "mistyrose", "palegreen", "paleturquoise", "peachpuff"
    };

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

        // Get ring color
        std::string color = "lightblue"; // default
        auto ringOpt = GetRingOfSatellite(satName);
        if (ringOpt.has_value())
        {
            uint32_t ringId = ringOpt.value();
            color = ringColors[ringId % ringColors.size()];
        }

        dot << "  \"" << satName << "\" [label=\"" << satName << "\", pos=\""
            << std::fixed << std::setprecision(2) << x << "," << y << "!\", "
            << "style=filled, fillcolor=" << color << ", tooltip=\"lat="
            << gt.latitude << " lon=" << gt.longitude << " alt=" << gt.altitude << "m\"];\n";
    }

    // Add ground station nodes as fixed-position geographic anchors.
    for (const auto& groundStation : m_groundStations)
    {
        if (!groundStation)
        {
            continue;
        }

        const std::string& stationName = groundStation->GetName();
        double x = groundStation->GetLongitude();
        double y = groundStation->GetLatitude();
        constexpr double scale = 3.0;
        x *= scale;
        y *= scale;

        dot << "  \"GS:" << stationName << "\" [label=\"" << stationName << "\", pos=\""
            << std::fixed << std::setprecision(2) << x << "," << y << "!\", "
            << "shape=box, style=filled, fillcolor=khaki, tooltip=\"type=ground-station lat="
            << groundStation->GetLatitude() << " lon=" << groundStation->GetLongitude() << "\"] ;\n";
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

        auto nodeLabel = [](const Ptr<Node>& node) {
            if (!node)
            {
                return std::string("Unknown");
            }
            if (Ptr<Satellite> sat = DynamicCast<Satellite>(node))
            {
                return sat->GetName();
            }
            if (Ptr<GroundStation> station = DynamicCast<GroundStation>(node))
            {
                return std::string("GS:") + station->GetName();
            }
            return std::string("Node-") + std::to_string(node->GetId());
        };

        std::string nameA = nodeLabel(nodeA);
        std::string nameB = nodeLabel(nodeB);

        // Calculate distance
        Ptr<MobilityModel> mobA = nodeA->GetObject<MobilityModel>();
        Ptr<MobilityModel> mobB = nodeB->GetObject<MobilityModel>();
        if (!mobA || !mobB)
        {
            continue;
        }
        double distance = mobA->GetDistanceFrom(mobB);
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

double
Constellation::CalculateSatelliteDistance(Ptr<Satellite> satA, Ptr<Satellite> satB)
{
    // Evaluate link feasibility at current simulator time.
    Vector3D posA = satA->GetPosition(Simulator::Now());
    Vector3D posB = satB->GetPosition(Simulator::Now());

    double dx = posA.x - posB.x;
    double dy = posA.y - posB.y;
    double dz = posA.z - posB.z;

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double
Constellation::CalculateSatelliteGroundDistance(Ptr<Satellite> sat, Ptr<GroundStation> station)
{
    if (!sat || !station)
    {
        return std::numeric_limits<double>::max();
    }

    Ptr<MobilityModel> satMob = sat->GetObject<MobilityModel>();
    if (!satMob)
    {
        Ptr<SatelliteMobilityModel> satModel = CreateObject<SatelliteMobilityModel>();
        satModel->SetSatellite(sat);
        sat->AggregateObject(satModel);
        satMob = satModel;
    }

    Ptr<MobilityModel> stationMob = station->GetObject<MobilityModel>();
    Ptr<ConstantPositionMobilityModel> fixed = DynamicCast<ConstantPositionMobilityModel>(stationMob);
    if (!fixed)
    {
        fixed = CreateObject<ConstantPositionMobilityModel>();
        station->AggregateObject(fixed);
    }

    const Vector ecef = GeographicPositions::GeographicToCartesianCoordinates(
        station->GetLatitude(),
        station->GetLongitude(),
        0.0,
        GeographicPositions::EarthSpheroidType::WGS84);
    fixed->SetPosition(ecef);

    return satMob->GetDistanceFrom(fixed);
}

Ptr<Satellite>
Constellation::FindClosestSatellite(Ptr<Satellite> reference, const std::vector<Ptr<Satellite>>& candidates)
{
    if (candidates.empty())
        return nullptr;

    Ptr<Satellite> closest = nullptr;
    double minDistance = std::numeric_limits<double>::max();

    for (Ptr<Satellite> candidate : candidates)
    {
        if (candidate == reference)
        {
            continue;
        }

        double distance = CalculateSatelliteDistance(reference, candidate);
        if (distance < minDistance)
        {
            minDistance = distance;
            closest = candidate;
        }
    }

    return closest;
}

Ptr<SatelliteNetDevice>
Constellation::GetOrCreateSatelliteNetDevice(Ptr<Satellite> satellite)
{
    if (!satellite)
    {
        return nullptr;
    }

    for (uint32_t i = 0; i < satellite->GetNDevices(); ++i)
    {
        Ptr<SatelliteNetDevice> device = DynamicCast<SatelliteNetDevice>(satellite->GetDevice(i));
        if (device)
        {
            return device;
        }
    }

    Ptr<SatelliteNetDevice> device = CreateObject<SatelliteNetDevice>();
    satellite->AddDevice(device);
    device->SetNode(satellite);
    return device;
}

Ptr<SatelliteNetDevice>
Constellation::GetOrCreateGroundStationNetDevice(Ptr<GroundStation> station)
{
    if (!station)
    {
        return nullptr;
    }

    for (uint32_t i = 0; i < station->GetNDevices(); ++i)
    {
        Ptr<SatelliteNetDevice> device = DynamicCast<SatelliteNetDevice>(station->GetDevice(i));
        if (device)
        {
            return device;
        }
    }

    Ptr<SatelliteNetDevice> device = CreateObject<SatelliteNetDevice>();
    station->AddDevice(device);
    device->SetNode(station);
    return device;
}

bool
Constellation::CreateIslLink(Ptr<Satellite> satA, Ptr<Satellite> satB, double maxRange)
{
    if (!satA || !satB || satA == satB)
    {
        return false;
    }

    // Check if distance is within range
    double distance = CalculateSatelliteDistance(satA, satB);
    if (distance > maxRange)
    {
        return false;
    }

    // Create mobility models if they don't exist
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

    // Ensure network devices exist on both satellites.
    Ptr<SatelliteNetDevice> devA = GetOrCreateSatelliteNetDevice(satA);
    Ptr<SatelliteNetDevice> devB = GetOrCreateSatelliteNetDevice(satB);
    return devA && devB;
}

bool
Constellation::CreateGroundLink(Ptr<Satellite> sat, Ptr<GroundStation> station, double maxRange)
{
    if (!sat || !station)
    {
        return false;
    }

    double distance = CalculateSatelliteGroundDistance(sat, station);
    if (distance > maxRange)
    {
        return false;
    }

    Ptr<SatelliteNetDevice> satDev = GetOrCreateSatelliteNetDevice(sat);
    Ptr<SatelliteNetDevice> gsDev = GetOrCreateGroundStationNetDevice(station);
    return satDev && gsDev;
}

const std::vector<Ptr<SatelliteLink>>&
Constellation::GetCurrentIsls() const
{
    return m_currentIsls;
}

const std::vector<Ptr<SatelliteLink>>&
Constellation::GetCurrentGroundLinks() const
{
    return m_currentGroundLinks;
}

void
Constellation::SetIslRefreshInterval(Time interval)
{
    NS_LOG_FUNCTION(this << interval);
    NS_ASSERT_MSG(interval.GetSeconds() > 0, "ISL refresh interval must be positive");
    m_islRefreshInterval = interval;
    if (m_refreshEvent.IsPending())
    {
        ScheduleTopologyRefresh();
    }
}

Time
Constellation::GetIslRefreshInterval() const
{
    return m_islRefreshInterval;
}

void
Constellation::RefreshIslTopology()
{
    NS_LOG_FUNCTION(this);
    // Clear and regenerate dynamic topologies; link creation will reschedule the next refresh.
    m_currentIsls.clear();
    m_currentGroundLinks.clear();

    if (m_islMaxRange > 0.0)
    {
        m_currentIsls = CreateIslLinks(m_islMaxRange);
    }
    if (m_groundMaxRange > 0.0)
    {
        m_currentGroundLinks = CreateGroundLinks(m_groundMaxRange);
    }

    NS_LOG_INFO("Refreshed ISL topology at time " << Simulator::Now().GetSeconds()
                                                   << "s, created " << m_currentIsls.size()
                                                   << " ISLs and " << m_currentGroundLinks.size()
                                                   << " ground links");
}

void
Constellation::ScheduleTopologyRefresh()
{
    NS_LOG_FUNCTION(this);
    m_refreshEvent.Cancel();
    if (m_islMaxRange > 0.0 || m_groundMaxRange > 0.0)
    {
        m_refreshEvent = Simulator::Schedule(m_islRefreshInterval,
                                             &Constellation::RefreshIslTopology,
                                             this);
    }
}

}  // namespace ns3