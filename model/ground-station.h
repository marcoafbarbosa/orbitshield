/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_GROUND_STATION_H
#define ORBITSHIELD_GROUND_STATION_H

#include "ns3/node.h"

#include <string>

namespace ns3
{

/**
 * \brief Fixed ground-station node used for satellite-ground topology and routing.
 */
class GroundStation : public Node
{
  public:
    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * \brief Constructor.
     */
    GroundStation();

    /**
     * \brief Destructor.
     */
    ~GroundStation() override;

    /**
     * \brief Set the ground-station display name.
     * \param name Human-readable station name.
     */
    void SetName(const std::string& name);

    /**
     * \brief Get the ground-station display name.
     * \return Station name.
     */
    const std::string& GetName() const;

    /**
     * \brief Set geodetic latitude in degrees.
     * \param latitude Latitude in range [-90, 90].
     */
    void SetLatitude(double latitude);

    /**
     * \brief Get geodetic latitude in degrees.
     * \return Latitude in degrees.
     */
    double GetLatitude() const;

    /**
     * \brief Set geodetic longitude in degrees.
     * \param longitude Longitude in range [-180, 180].
     */
    void SetLongitude(double longitude);

    /**
     * \brief Get geodetic longitude in degrees.
     * \return Longitude in degrees.
     */
    double GetLongitude() const;

  private:
    std::string m_name;
    double m_latitude{0.0};
    double m_longitude{0.0};
};

} // namespace ns3

#endif /* ORBITSHIELD_GROUND_STATION_H */