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

/**
 * \brief Test case for ISL channel
 */
class IslChannelTestCase : public ns3::TestCase
{
  public:
    IslChannelTestCase();
    ~IslChannelTestCase() override;

  private:
    void DoRun() override;
    bool OnReceive(ns3::Ptr<ns3::NetDevice> device,
                   ns3::Ptr<const ns3::Packet> packet,
                   uint16_t protocol,
                   const ns3::Address& src);

    bool m_received;
};

#endif /* TEST_ISL_H */
