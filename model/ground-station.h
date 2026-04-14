/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_GROUND_STATION_H
#define ORBITSHIELD_GROUND_STATION_H

#include "ns3/object.h"

#include <string>

namespace ns3
{

class GroundStation : public Object
{
  public:
    static TypeId GetTypeId();

    GroundStation();
    ~GroundStation() override;

    void SetName(const std::string& name);
    const std::string& GetName() const;

    void SetLatitude(double latitude);
    double GetLatitude() const;

    void SetLongitude(double longitude);
    double GetLongitude() const;

  private:
    std::string m_name;
    double m_latitude{0.0};
    double m_longitude{0.0};
};

} // namespace ns3

#endif /* ORBITSHIELD_GROUND_STATION_H */