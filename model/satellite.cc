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
            // .AddConstructor<Satellite>()
            // .AddAttribute("Altitude",
            //               "Orbital altitude in meters",
            //               DoubleValue(400000.0),
            //               MakeDoubleAccessor(&Satellite::SetAltitude, &Satellite::GetAltitude),
            //               MakeDoubleChecker<double>(0.0))
            // .AddAttribute("Inclination",
            //               "Orbital inclination in degrees",
            //               DoubleValue(45.0),
            //               MakeDoubleAccessor(&Satellite::SetInclination, &Satellite::GetInclination),
            //               MakeDoubleChecker<double>(0.0, 180.0));
            ;
    return tid;
}

Satellite::Satellite(std::string& name, std::string& tle1, std::string& tle2)
    : m_name(name), m_perturbSatellite(perturb::Satellite::from_tle(tle1, tle2))
{
    NS_LOG_FUNCTION(this);
    auto e = m_perturbSatellite.propagate_from_epoch(0, m_currentState);
    if(e != perturb::Sgp4Error::NONE)
    {
        NS_LOG_ERROR("Error initializing satellite state from TLE: " << (int)e);
    }
}

Satellite::~Satellite()
{
    NS_LOG_FUNCTION(this);
}


std::string
Satellite::GetName() const
{
    NS_LOG_FUNCTION(this);
    return m_name;
}

Vector3D
Satellite::GetPosition()
{
    NS_LOG_FUNCTION(this);
    perturb::StateVector sv;
    auto e = m_perturbSatellite.propagate_from_epoch(0, sv);
    if(e != perturb::Sgp4Error::NONE)
    {
        NS_LOG_ERROR("Error propagating satellite position: " << (int)e);
        return Vector3D(0.0, 0.0, 0.0);
    }
    return Vector3D(sv.position[0], sv.position[1], sv.position[2]);
}

}  // namespace ns3
