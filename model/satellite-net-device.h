/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_SATELLITE_NET_DEVICE_H
#define ORBITSHIELD_SATELLITE_NET_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/chunk.h"
#include "ns3/callback.h"
#include "ns3/mac48-address.h"
#include "satellite-link.h"

#include <vector>

namespace ns3
{

class SatelliteNetDevice : public NetDevice
{
  public:
    static TypeId GetTypeId();

    SatelliteNetDevice();
    ~SatelliteNetDevice() override;

    /**
     * \brief Add a satellite link to this device
     * 
     * This device can manage multiple concurrent links, each with independent
     * propagation delay models and packet delivery.
     * 
     * \param link The link to add
     */
    void AddLink(Ptr<SatelliteLink> link);

    /**
     * \brief Set a single link (deprecated; use AddLink for new code)
     * 
     * This method exists for backward compatibility. It clears existing links
     * and adds the provided link as the sole link.
     * 
     * \param link The link to set
     */
    void SetLink(Ptr<SatelliteLink> link);

    /**
     * \brief Get the first satellite link (deprecated; use GetLinks for new code)
     * 
     * \return The first link if any exist, nullptr otherwise
     */
    Ptr<SatelliteLink> GetSatelliteLink() const;

    /**
     * \brief Get all satellite links attached to this device
     * 
     * \return Vector of links
     */
    const std::vector<Ptr<SatelliteLink>>& GetLinks() const;

    bool ReceiveFromChannel(Ptr<Packet> packet,
                            const Address& source,
                            const Address& dest,
                            uint16_t protocolNumber);

    // NetDevice methods
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    void SetAddress(Address address) override;
    Address GetAddress() const override;
    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;
    bool IsLinkUp() const override;
    void AddLinkChangeCallback(Callback<void> callback) override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    Address GetMulticast(Ipv6Address addr) const override;
    bool IsBridge() const override;
    bool IsPointToPoint() const override;
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    void SetReceiveCallback(ReceiveCallback cb) override;
    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;

  private:
    std::vector<Ptr<SatelliteLink>> m_links; //!< Collection of links (point-to-point satellite channels)
    Ptr<Node> m_node;                          //!< The node this device is attached to
    Address m_address;                          //!< MAC address of this device
    uint32_t m_ifIndex;                         //!< Interface index assigned by the node
    bool m_linkUp;                              //!< Whether the link is considered up
    uint16_t m_mtu;                             //!< Maximum transmission unit in bytes
    ReceiveCallback m_receiveCallback;          //!< Callback invoked on packet reception
    PromiscReceiveCallback m_promiscCallback;   //!< Callback for promiscuous mode reception
    Callback<void> m_linkChangeCallback;        //!< Callback registered for link-state changes
};

} // namespace ns3

#endif /* ORBITSHIELD_SATELLITE_NET_DEVICE_H */
