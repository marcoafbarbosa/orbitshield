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

/**
 * \brief Point-to-point channel connecting two SatelliteNetDevice endpoints.
 */
class SatelliteLink : public Channel
{
  public:
    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * \brief Construct an unbound link.
     */
    SatelliteLink();

    /**
     * \brief Construct and bind a link to two endpoint devices.
     * \param a First endpoint device.
     * \param b Second endpoint device.
     */
    SatelliteLink(Ptr<SatelliteNetDevice> a, Ptr<SatelliteNetDevice> b);

    /**
     * \brief Destructor.
     */
    ~SatelliteLink() override;

    /**
     * \brief Get the number of attached endpoint devices.
     * \return Number of attached devices (0, 1, or 2).
     */
    std::size_t GetNDevices() const override;

    /**
     * \brief Get an endpoint device by index.
     * \param i Endpoint index.
     * \return Endpoint device at index \p i or nullptr when out of range.
     */
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

    /**
     * \brief Set maximum allowed endpoint distance for active transmission.
     * \param range Maximum distance in meters.
     */
    void SetMaxRange(double range);

    /**
     * \brief Get configured maximum link range.
     * \return Maximum distance in meters.
     */
    double GetMaxRange() const;

    /**
     * \brief Set propagation delay model used by this channel.
     * \param delayModel Delay model instance.
     */
    void SetPropagationDelayModel(Ptr<PropagationDelayModel> delayModel);

    /**
     * \brief Get currently configured propagation delay model.
     * \return Delay model instance, or nullptr when not configured.
     */
    Ptr<PropagationDelayModel> GetPropagationDelayModel() const;

    /**
     * \brief Check whether the link is currently active.
     * \return True when both endpoints are present and within max range.
     */
    bool IsActive() const;

    /**
     * \brief Transmit a packet from one endpoint to the peer endpoint.
     * \param source Transmitting endpoint device.
     * \param packet Packet to transmit.
     * \param dest Destination L2 address.
     * \param protocolNumber Ethertype/protocol number.
     * \param sourceAddress Source L2 address.
     * \return True when transmission is accepted and scheduled.
     */
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
