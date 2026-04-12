/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_CONSTELLATION_H
#define ORBITSHIELD_CONSTELLATION_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "satellite.h"

#include <perturb/perturb.hpp>

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ns3
{

class SatelliteLink;

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
     * \brief Load satellites from a TLE file
     * \param file Input file stream
     */
    void LoadFromTleFile(std::istream& file);

    /**
     * \brief Get the collection of satellites
     * \return Vector of satellite pointers
     */
    const std::vector<Ptr<Satellite>>& GetSatellites() const;

    /**
     * \brief Create pairwise ISL links between each satellite pair in the constellation.
     * \param maxRange maximum distance (meters) for active links.
     * \return vector of created links.
     */
    std::vector<Ptr<SatelliteLink>> CreateIslLinks(double maxRange);

    /**
     * \brief Export ISL topology as a Graphviz DOT format string.
     * \param links Vector of satellite links to export.
     * \param activeOnly If true, only export active links; otherwise export all links.
     * \return A string containing the DOT graph representation.
     */
    std::string ExportIslAsDot(const std::vector<Ptr<SatelliteLink>>& links, bool activeOnly = true) const;

    // Ring metadata (optional) for constellation structure
    void LoadFromRingFile(const std::string& filename);
    void LoadFromRingFile(std::istream& file, const std::string& basePath = "");

    uint32_t GetRingCount() const;
    std::optional<uint32_t> GetRingOfSatellite(const std::string& satName) const;
    const std::vector<Ptr<Satellite>>& GetSatellitesInRing(uint32_t ringId) const;
    const std::vector<Ptr<Satellite>>& GetPreviousRingSatellites(uint32_t ringId) const;
    const std::vector<Ptr<Satellite>>& GetNextRingSatellites(uint32_t ringId) const;

  private:
    std::vector<Ptr<Satellite>> m_satellites; //!< Collection of satellites in the constellation
    perturb::JulianDate m_simulationStartJD;  //!< Global simulation start time

    uint32_t m_ringCount = 0;
    std::map<uint32_t, std::vector<Ptr<Satellite>>> m_rings; //!< ring-id -> satellites
    std::unordered_map<std::string, uint32_t> m_satelliteRingMap; //!< satellite name -> ring-id
    std::string m_constellationName;
    std::string m_tleFile; //!< Path to TLE file referenced in ring file
};

}  // namespace ns3

#endif /* ORBITSHIELD_CONSTELLATION_H */