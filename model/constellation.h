/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_CONSTELLATION_H
#define ORBITSHIELD_CONSTELLATION_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ground-station.h"
#include "satellite.h"

#include <perturb/perturb.hpp>

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class ConstellationTestCase;

namespace ns3
{

class SatelliteLink;
class SatelliteNetDevice;

/**
 * \brief A constellation of satellites for orbital simulations
 *
 * This class represents a collection of satellites forming a constellation,
 * providing methods to load satellites from TLE data and manage constellation-level operations.
 */
class Constellation : public Object
{
    friend class ::ConstellationTestCase;

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
     * \brief Create satellite-ground links based only on distance.
     * \param maxRange maximum distance (meters) for active links.
     * \return vector of created links.
     */
    std::vector<Ptr<SatelliteLink>> CreateGroundLinks(double maxRange);

    /**
     * \brief Export ISL topology as a Graphviz DOT format string.
     * \param links Vector of satellite links to export.
     * \param activeOnly If true, only export active links; otherwise export all links.
     * \return A string containing the DOT graph representation.
     */
    std::string ExportIslAsDot(const std::vector<Ptr<SatelliteLink>>& links, bool activeOnly = true) const;

    // YAML constellation description metadata
    void LoadFromRingFile(const std::string& filename);
    void LoadFromRingFile(std::istream& file, const std::string& basePath = "");

    uint32_t GetRingCount() const;
    std::string GetConstellationName() const;
    const std::vector<Ptr<GroundStation>>& GetGroundStations() const;
    std::optional<uint32_t> GetRingOfSatellite(const std::string& satName) const;
    const std::vector<Ptr<Satellite>>& GetSatellitesInRing(uint32_t ringId) const;
    const std::vector<Ptr<Satellite>>& GetPreviousRingSatellites(uint32_t ringId) const;
    const std::vector<Ptr<Satellite>>& GetNextRingSatellites(uint32_t ringId) const;

    /**
     * \brief Get the cached ISL topology at the current time.
     * \return Vector of ISL links (updated when topology is refreshed)
     */
    const std::vector<Ptr<SatelliteLink>>& GetCurrentIsls() const;

    /**
     * \brief Get the cached ground-link topology at the current time.
     * \return Vector of satellite-ground links (updated when topology is refreshed)
     */
    const std::vector<Ptr<SatelliteLink>>& GetCurrentGroundLinks() const;

    /**
     * \brief Set the interval at which ISL topology is automatically refreshed.
     * \param interval Time between topology refreshes (default: 10 seconds)
     */
    void SetIslRefreshInterval(Time interval);

    /**
     * \brief Get the configured ISL refresh interval.
     * \return Current refresh interval
     */
    Time GetIslRefreshInterval() const;

    /**
     * \brief Force an immediate refresh of the ISL topology.
     * Updates the cached ISL list based on current satellite positions.
     */
    void RefreshIslTopology();

  private:
    std::vector<Ptr<Satellite>> m_satellites; //!< Collection of satellites in the constellation
    perturb::JulianDate m_simulationStartJD;  //!< Global simulation start time

    uint32_t m_ringCount = 0;
    std::map<uint32_t, std::vector<Ptr<Satellite>>> m_rings; //!< ring-id -> satellites
    std::unordered_map<std::string, uint32_t> m_satelliteRingMap; //!< satellite name -> ring-id
    std::string m_constellationName;
    std::string m_tleFile; //!< Path to TLE file referenced in ring file
    std::vector<Ptr<GroundStation>> m_groundStations;

    // Time-aware ISL topology management (driven by ns-3 Simulator via scheduled events)
    EventId m_refreshEvent;                   //!< Pending topology refresh event
    Time m_islRefreshInterval{Seconds(10)};   //!< Interval between refreshes
    std::vector<Ptr<SatelliteLink>> m_currentIsls; //!< Cached ISL topology
    std::vector<Ptr<SatelliteLink>> m_currentGroundLinks; //!< Cached satellite-ground topology
    double m_islMaxRange{0.0};                 //!< Max range for ISL creation (cached)
    double m_groundMaxRange{0.0};              //!< Max range for ground-link creation (cached)

    // Helper methods for ISL creation
    double CalculateSatelliteDistance(Ptr<Satellite> satA, Ptr<Satellite> satB);
    double CalculateSatelliteGroundDistance(Ptr<Satellite> sat, Ptr<GroundStation> station);
    Ptr<Satellite> FindClosestSatellite(Ptr<Satellite> reference, const std::vector<Ptr<Satellite>>& candidates);
    Ptr<SatelliteNetDevice> GetOrCreateSatelliteNetDevice(Ptr<Satellite> satellite);
    Ptr<SatelliteNetDevice> GetOrCreateGroundStationNetDevice(Ptr<GroundStation> station);
    bool CreateIslLink(Ptr<Satellite> satA, Ptr<Satellite> satB, double maxRange);
    bool CreateGroundLink(Ptr<Satellite> sat, Ptr<GroundStation> station, double maxRange);
    void ScheduleTopologyRefresh(); //!< Schedule next automatic topology refresh
};

}  // namespace ns3

#endif /* ORBITSHIELD_CONSTELLATION_H */