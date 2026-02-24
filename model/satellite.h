/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_SATELLITE_H
#define ORBITSHIELD_SATELLITE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"

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
    Satellite();

    /**
     * \brief Destructor
     */
    ~Satellite() override;

    /**
     * \brief Set the satellite's orbital altitude in meters
     * \param altitude the altitude in meters
     */
    void SetAltitude(double altitude);

    /**
     * \brief Get the satellite's orbital altitude in meters
     * \return the altitude in meters
     */
    double GetAltitude() const;

    /**
     * \brief Set the satellite's orbital inclination in degrees
     * \param inclination the inclination in degrees
     */
    void SetInclination(double inclination);

    /**
     * \brief Get the satellite's orbital inclination in degrees
     * \return the inclination in degrees
     */
    double GetInclination() const;

  private:
    double m_altitude;     //!< Orbital altitude in meters
    double m_inclination;  //!< Orbital inclination in degrees
};

}  // namespace ns3

#endif /* ORBITSHIELD_SATELLITE_H */
