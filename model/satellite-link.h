/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_SATELLITE_LINK_H
#define ORBITSHIELD_SATELLITE_LINK_H

#include "ns3/channel.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/net-device.h"
#include "ns3/mac48-address.h"

#include <vector>

namespace ns3
{

// Forward declaration to avoid circular dependency
class SatelliteNetDevice;

class SatelliteLink : public Channel
{
  public:
    static TypeId GetTypeId();

    SatelliteLink();
    SatelliteLink(Ptr<SatelliteNetDevice> a, Ptr<SatelliteNetDevice> b);
    ~SatelliteLink() override;

    std::size_t GetNDevices() const override;
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

    void SetMaxRange(double range);
    double GetMaxRange() const;

    void SetPropagationDelayModel(Ptr<PropagationDelayModel> delayModel);
    Ptr<PropagationDelayModel> GetPropagationDelayModel() const;

    bool Send(Ptr<NetDevice> source,
              Ptr<Packet> packet,
              const Address& dest,
              uint16_t protocolNumber,
              const Address& sourceAddress);

  private:
    Ptr<SatelliteNetDevice> m_deviceA;
    Ptr<SatelliteNetDevice> m_deviceB;
    double m_maxRange;
    Ptr<PropagationDelayModel> m_delayModel;
};


} // namespace ns3

#endif /* ORBITSHIELD_SATELLITE_LINK_H */
