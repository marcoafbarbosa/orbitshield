/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_CONSTELLATION_H
#define ORBITSHIELD_CONSTELLATION_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "satellite.h"

#include <vector>

namespace ns3
{

/**
 * \brief A constellation of satellites for orbital simulations
 *
 * This class represents a collection of satellites forming a constellation,
 * providing methods to load satellites from TLE data and manage constellation-level operations.
 */
class Constellation : public Object
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
    Constellation();

    /**
     * \brief Destructor
     */
    ~Constellation() override;

    /**
     * \brief Load satellites from a TLE file
     * \param filename Path to the TLE file
     */
    void LoadFromTleFile(const std::string& filename);

    /**
     * \brief Get the collection of satellites
     * \return Vector of satellite pointers
     */
    const std::vector<Ptr<Satellite>>& GetSatellites() const;

  private:
    std::vector<Ptr<Satellite>> m_satellites; //!< Collection of satellites in the constellation
};

}  // namespace ns3

#endif /* ORBITSHIELD_CONSTELLATION_H */