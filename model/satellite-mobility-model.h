/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_SATELLITE_MOBILITY_MODEL_H
#define ORBITSHIELD_SATELLITE_MOBILITY_MODEL_H

#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "satellite.h"

namespace ns3
{

class SatelliteMobilityModel : public MobilityModel
{
  public:
    static TypeId GetTypeId();

    SatelliteMobilityModel();
    ~SatelliteMobilityModel() override;

    void SetSatellite(Ptr<Satellite> satellite);
    Ptr<Satellite> GetSatellite() const;

    Ptr<MobilityModel> Copy() const override;

  private:
    Vector DoGetPosition() const override;
    void DoSetPosition(const Vector &position) override;
    Vector DoGetVelocity() const override;
    int64_t DoAssignStreams(int64_t stream) override;

    Ptr<Satellite> m_satellite;
};

} // namespace ns3

#endif /* ORBITSHIELD_SATELLITE_MOBILITY_MODEL_H */
