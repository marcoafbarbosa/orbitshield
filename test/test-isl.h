/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef TEST_ISL_H
#define TEST_ISL_H

#include "ns3/test.h"
#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/address.h"
#include "ns3/ptr.h"

using namespace ns3;

/**
 * \brief Test case for ISL channel
 */
class IslChannelTestCase : public TestCase
{
  public:
    IslChannelTestCase();
    ~IslChannelTestCase() override;

  private:
    void DoRun() override;
    bool OnReceive(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address& src);

    bool m_received;
};

#endif /* TEST_ISL_H */
