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
    Satellite(std::string& name, std::string& tle1, std::string& tle2);

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
     * \brief Get the satellite's position in ECI coordinates at a given time
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
     * \brief Get the satellite's position in ECI at the current simulation time
     * \return the position as a Vector3D in meters
     */
    Vector3D GetPosition();

  private:
    std::string m_name;                     //!< Satellite name
    mutable perturb::Satellite m_perturbSatellite; //!< Perturb satellite object for orbital calculations
    mutable perturb::StateVector m_currentState;   //!< Current state of the satellite for position calculations
};

}  // namespace ns3

#endif /* ORBITSHIELD_SATELLITE_H */
