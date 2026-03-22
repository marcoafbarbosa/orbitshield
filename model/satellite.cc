/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "satellite.h"
#include "orbitshield-utils.h"

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
            .SetGroupName("OrbitShield");
    return tid;
}

Satellite::Satellite(const std::string& name, std::string& tle1, std::string& tle2, perturb::JulianDate simulationStartJD)
    : m_name(name), m_perturbSatellite(perturb::Satellite::from_tle(tle1, tle2)), m_simulationStartJD(simulationStartJD)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Creating Satellite " << m_name << " with simulation start: " << JulianDateToString(m_simulationStartJD));
    auto tleEpoch = m_perturbSatellite.epoch();
    double delta_to_start = (tleEpoch - m_simulationStartJD) * 1440.0;
    auto e = m_perturbSatellite.propagate_from_epoch(delta_to_start, m_currentState);
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
Satellite::GetPosition(Time at) const
{
    NS_LOG_FUNCTION(this << at);
    perturb::StateVector sv{};
    sv.position = {0.0, 0.0, 0.0};
    sv.velocity = {0.0, 0.0, 0.0};

    // Calculate the real Julian date for the requested simulation time
    perturb::JulianDate realJD = m_simulationStartJD + at.GetSeconds() / (24.0 * 60.0 * 60.0);
    // Calculate time difference in minutes from TLE epoch to real time
    auto tleEpoch = m_perturbSatellite.epoch();
    double timeDeltaFromTleEpoch = (realJD - tleEpoch) * 1440.0;
    auto e = m_perturbSatellite.propagate_from_epoch(timeDeltaFromTleEpoch, sv);
    if(e != perturb::Sgp4Error::NONE)
    {
        NS_LOG_ERROR("Error propagating satellite position for " << at.GetSeconds() << "s : " << (int)e);
        return Vector3D(0.0, 0.0, 0.0);
    }
    // convert from km to meters
    return Vector3D(sv.position[0] * 1000.0, sv.position[1] * 1000.0, sv.position[2] * 1000.0);
}

Vector3D
Satellite::GetVelocity(Time at) const
{
    NS_LOG_FUNCTION(this << at);
    perturb::StateVector sv{};
    sv.position = {0.0, 0.0, 0.0};
    sv.velocity = {0.0, 0.0, 0.0};
    // Calculate the real Julian date for the requested simulation time
    perturb::JulianDate realJD = m_simulationStartJD + at.GetSeconds() / 86400.0;
    // Calculate time difference in minutes from TLE epoch to real time
    auto tleEpoch = m_perturbSatellite.epoch();
    double minutesFromEpoch = (realJD - tleEpoch) * 1440.0;
    auto e = m_perturbSatellite.propagate_from_epoch(minutesFromEpoch, sv);
    if(e != perturb::Sgp4Error::NONE)
    {
        NS_LOG_ERROR("Error propagating satellite velocity for " << at.GetSeconds() << "s : " << (int)e);
        return Vector3D(0.0, 0.0, 0.0);
    }
    return Vector3D(sv.velocity[0] * 1000.0, sv.velocity[1] * 1000.0, sv.velocity[2] * 1000.0);
}

Vector3D
Satellite::GetPosition()
{
    NS_LOG_FUNCTION(this);
    return GetPosition(Simulator::Now());
}

}  // namespace ns3
