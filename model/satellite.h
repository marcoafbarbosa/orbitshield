/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_SATELLITE_H
#define ORBITSHIELD_SATELLITE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/vector.h"

#include <perturb/perturb.hpp>

namespace ns3
{

/**
 * \brief A satellite node for orbital constellation simulations
 *
 * This class represents a single satellite in an orbital constellation,
 * providing the foundation for simulating satellite communications and
 * inter-satellite links (ISLs).
 */
class Satellite : public Node
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Constructor
     */
    Satellite(const std::string& name, std::string& tle1, std::string& tle2, perturb::JulianDate simulationStartJD);

    /**
     * \brief Destructor
     */
    ~Satellite() override;

    /**
     * \brief Get the satellite's name
     * \return the satellite name
     */
    std::string GetName() const;

    /**
     * \brief Get the satellite's position in ECEF coordinates at a given time
     * \param at the simulation time
     * \return the position as a Vector3D in meters
     */
    Vector3D GetPosition(Time at) const;

    /**
     * \brief Get the satellite's velocity in ECI coordinates at a given time
     * \param at the simulation time
     * \return the velocity as a Vector3D in meters per second
     */
    Vector3D GetVelocity(Time at) const;

    /**
     * \brief Get the satellite's position in ECEF at the current simulation time
     * \return the position as a Vector3D in meters
     */
    Vector3D GetPosition();

    /**
     * \brief Get the satellite's epoch time
     * \return the epoch time as a Time object
     */
    Time GetTleEpochAsNs3Time() const;

    /**
     * \brief Get the satellite's epoch time as a JulianDate
     * \return the epoch time as a perturb::JulianDate
     */
    perturb::JulianDate GetTleEpochAsJulianDate() const;

    /**
     * \brief Get the satellite's classical orbital elements at the current simulation time
     * \return the orbital elements as a perturb::ClassicalOrbitalElements struct
     */
    perturb::ClassicalOrbitalElements GetOrbitalElements() const;

    /**
     * \brief Ground track position in geographic coordinates
     */
    struct GroundTrackPosition
    {
        double latitude;  //!< in degrees [-90, +90]
        double longitude; //!< in degrees [-180, +180)
        double altitude;  //!< in meters above the ellipsoid
    };

    /**
     * \brief Get the simulation start time in Julian date
     * \return the simulation start time as a perturb::JulianDate
     */
    perturb::JulianDate GetSimulationStartJD() const;

    /**
     * \brief Get satellite ground track lat/lon/alt for simulation time.
     * \param at simulation time
     */
    GroundTrackPosition GetGroundTrackPosition(Time at) const;

    /**
     * \brief Get satellite ground track lat/lon/alt for current simulator time.
     */
    GroundTrackPosition GetGroundTrackPosition() const;

  private:
    const std::string m_name;                     //!< Satellite name
    mutable perturb::Satellite m_perturbSatellite; //!< Perturb satellite object for orbital calculations
    mutable perturb::StateVector m_currentState;   //!< Current state of the satellite for position calculations
    perturb::JulianDate m_simulationStartJD;       //!< Simulation start time in Julian date
};

}  // namespace ns3

#endif /* ORBITSHIELD_SATELLITE_H */
