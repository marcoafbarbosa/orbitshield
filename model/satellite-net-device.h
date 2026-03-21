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

namespace ns3
{

class SatelliteNetDevice : public NetDevice
{
  public:
    static TypeId GetTypeId();

    SatelliteNetDevice();
    ~SatelliteNetDevice() override;

    void SetLink(Ptr<SatelliteLink> link);
    Ptr<SatelliteLink> GetSatelliteLink() const;

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
    Ptr<SatelliteLink> m_link;
    Ptr<Node> m_node;
    Address m_address;
    uint32_t m_ifIndex;
    bool m_linkUp;
    uint16_t m_mtu;
    ReceiveCallback m_receiveCallback;
    PromiscReceiveCallback m_promiscCallback;
    Callback<void> m_linkChangeCallback;
};

} // namespace ns3

#endif /* ORBITSHIELD_SATELLITE_NET_DEVICE_H */
