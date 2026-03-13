/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "constellation.h"

#include "ns3/log.h"

#include <fstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Constellation");

NS_OBJECT_ENSURE_REGISTERED(Constellation);

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

    std::string line;
    std::string currentName;
    std::string currentLine1;

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
            Ptr<Satellite> satellite = CreateObject<Satellite>(currentName, currentLine1, line);
            m_satellites.push_back(satellite);

            NS_LOG_INFO("Loaded satellite: " << currentName);

            // Reset for next satellite
            currentName.clear();
            currentLine1.clear();
        }
    }

    file.close();
    NS_LOG_INFO("Loaded " << m_satellites.size() << " satellites from " << filename);
}

const std::vector<Ptr<Satellite>>&
Constellation::GetSatellites() const
{
    NS_LOG_FUNCTION(this);
    return m_satellites;
}

}  // namespace ns3