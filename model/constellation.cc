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

}  // namespace ns3