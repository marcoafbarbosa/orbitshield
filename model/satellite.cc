/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "satellite.h"
#include "orbitshield-utils.h"

#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/angles.h"

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Satellite");

NS_OBJECT_ENSURE_REGISTERED(Satellite);

namespace
{
// Normalize angle to [0, 360)
double
NormalizeDegrees(double angle)
{
    double normalized = fmod(angle, 360.0);
    if (normalized < 0.0)
    {
        normalized += 360.0;
    }
    return normalized;
}

// Compute Greenwich Mean Sidereal Time (degrees) from Julian date.
// Source: https://aa.usno.navy.mil/faq/JD_formula
// (approximation for UTC/UT1, good for visualization purposes)
double
JulianDateToGmstDegrees(const perturb::JulianDate& jd)
{
    // Julian date as decimal days
    double julianDay = jd.jd + jd.jd_frac;

    // Number of days since J2000.0
    double d = julianDay - 2451545.0;
    double d0 = floor(julianDay + 0.5) - 0.5 - 2451545.0; // at 0h UT
    double t = d / 36525.0;

    double gmstHours = 6.697374558 + 0.06570982441908 * d0 +
                       1.00273790935 * ((julianDay - floor(julianDay + 0.5) + 0.5) * 24.0) +
                       0.000026 * t * t;
    double gmstDeg = NormalizeDegrees(gmstHours * 15.0);
    return gmstDeg;
}

// Convert ECI (approx TEME) coordinates to ECEF via GMST rotation.
Vector3D
EciToEcef(const Vector3D& eci, const perturb::JulianDate& jd)
{
    double gmstDeg = JulianDateToGmstDegrees(jd);
    double gmstRad = DegreesToRadians(gmstDeg);

    double cosTheta = cos(gmstRad);
    double sinTheta = sin(gmstRad);

    double x = cosTheta * eci.x + sinTheta * eci.y;
    double y = -sinTheta * eci.x + cosTheta * eci.y;
    double z = eci.z;

    return Vector3D(x, y, z);
}

} // anonymous namespace

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
    // Perturb returns TEME/ECI in km; convert to meters first.
    const Vector3D eciPosition(sv.position[0] * 1000.0, sv.position[1] * 1000.0, sv.position[2] * 1000.0);

    // Return ECEF because ns-3 mobility and distance calculations are Earth-fixed.
    return EciToEcef(eciPosition, realJD);
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

Time Satellite::GetTleEpochAsNs3Time() const
{
    NS_LOG_FUNCTION(this);
    auto epochJD = m_perturbSatellite.epoch();
    // Convert Julian date to Time (seconds since simulation start)
    double secondsSinceStart = (epochJD - m_simulationStartJD) * 24.0 * 60.0 * 60.0;
    return Time(secondsSinceStart);
}

perturb::JulianDate Satellite::GetTleEpochAsJulianDate() const
{
    NS_LOG_FUNCTION(this);
    return m_perturbSatellite.epoch();
}

perturb::ClassicalOrbitalElements Satellite::GetOrbitalElements() const
{
    NS_LOG_FUNCTION(this);
    perturb::StateVector sv{};
    sv.position = {0.0, 0.0, 0.0};
    sv.velocity = {0.0, 0.0, 0.0};
    m_perturbSatellite.propagate_from_epoch(0.0, sv);
    return perturb::ClassicalOrbitalElements(sv);
}


perturb::JulianDate Satellite::GetSimulationStartJD() const
{
    NS_LOG_FUNCTION(this);
    return m_simulationStartJD;
}

Satellite::GroundTrackPosition
Satellite::GetGroundTrackPosition(Time at) const
{
    NS_LOG_FUNCTION(this << at);

    // GetPosition() already returns ECEF coordinates.
    Vector3D ecefPosition = GetPosition(at);

    // Convert to geodetic coordinates in WGS84
    Vector geo = GeographicPositions::CartesianToGeographicCoordinates(Vector(ecefPosition.x, ecefPosition.y, ecefPosition.z), GeographicPositions::WGS84);

    GroundTrackPosition result;
    result.latitude = geo.x;
    result.longitude = geo.y;
    result.altitude = geo.z;

    return result;
}

Satellite::GroundTrackPosition
Satellite::GetGroundTrackPosition() const
{
    NS_LOG_FUNCTION(this);
    return GetGroundTrackPosition(Simulator::Now());
}

}  // namespace ns3