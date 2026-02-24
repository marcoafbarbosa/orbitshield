/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "satellite.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Satellite");

NS_OBJECT_ENSURE_REGISTERED(Satellite);

TypeId
Satellite::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Satellite")
            .SetParent<Node>()
            .SetGroupName("OrbitShield")
            .AddConstructor<Satellite>()
            .AddAttribute("Altitude",
                          "Orbital altitude in meters",
                          DoubleValue(400000.0),
                          MakeDoubleAccessor(&Satellite::SetAltitude, &Satellite::GetAltitude),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("Inclination",
                          "Orbital inclination in degrees",
                          DoubleValue(45.0),
                          MakeDoubleAccessor(&Satellite::SetInclination, &Satellite::GetInclination),
                          MakeDoubleChecker<double>(0.0, 180.0));
    return tid;
}

Satellite::Satellite()
    : m_altitude(400000.0),
      m_inclination(45.0)
{
    NS_LOG_FUNCTION(this);
}

Satellite::~Satellite()
{
    NS_LOG_FUNCTION(this);
}

void
Satellite::SetAltitude(double altitude)
{
    NS_LOG_FUNCTION(this << altitude);
    m_altitude = altitude;
}

double
Satellite::GetAltitude() const
{
    NS_LOG_FUNCTION(this);
    return m_altitude;
}

void
Satellite::SetInclination(double inclination)
{
    NS_LOG_FUNCTION(this << inclination);
    m_inclination = inclination;
}

double
Satellite::GetInclination() const
{
    NS_LOG_FUNCTION(this);
    return m_inclination;
}

}  // namespace ns3
