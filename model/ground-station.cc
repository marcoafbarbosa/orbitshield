/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "ground-station.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GroundStation");

NS_OBJECT_ENSURE_REGISTERED(GroundStation);

TypeId
GroundStation::GetTypeId()
{
    static TypeId tid = TypeId("ns3::GroundStation")
                            .SetParent<Node>()
                            .AddConstructor<GroundStation>()
                            .SetGroupName("OrbitShield");
    return tid;
}

GroundStation::GroundStation()
{
    NS_LOG_FUNCTION(this);
}

GroundStation::~GroundStation()
{
    NS_LOG_FUNCTION(this);
}

void
GroundStation::SetName(const std::string& name)
{
    m_name = name;
}

const std::string&
GroundStation::GetName() const
{
    return m_name;
}

void
GroundStation::SetLatitude(double latitude)
{
    m_latitude = latitude;
}

double
GroundStation::GetLatitude() const
{
    return m_latitude;
}

void
GroundStation::SetLongitude(double longitude)
{
    m_longitude = longitude;
}

double
GroundStation::GetLongitude() const
{
    return m_longitude;
}

} // namespace ns3