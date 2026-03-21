/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "ns3/orbitshield-module.h"
#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

using namespace ns3;

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

IslChannelTestCase::IslChannelTestCase()
    : TestCase("Test ISL channel distance threshold and delay"), m_received(false)
{
}

IslChannelTestCase::~IslChannelTestCase()
{
}

bool
IslChannelTestCase::OnReceive(Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address&)
{
    m_received = true;
    return true;
}

void
IslChannelTestCase::DoRun()
{
    std::string name1 = "IRIDIUM 7";
    std::string tle1_line1 = "1 24793U 97020B   26071.51251582  .00000492  00000-0  16199-3 0  9992";
    std::string tle1_line2 = "2 24793  86.3908  64.4117 0001975  80.9987 279.1433 14.36187486510404";

    std::string name2 = "IRIDIUM 5";
    std::string tle2_line1 = "1 24795U 97020D   26071.62611496  .00005035  00000-0  38599-3 0  9996";
    std::string tle2_line2 = "2 24795  86.3893 319.6474 0099647 252.3257 106.7076 14.97664148525594";

    Ptr<Satellite> sat1 = CreateObject<Satellite>(name1, tle1_line1, tle1_line2);
    Ptr<Satellite> sat2 = CreateObject<Satellite>(name2, tle2_line1, tle2_line2);

    Ptr<SatelliteNetDevice> dev1 = CreateObject<SatelliteNetDevice>();
    Ptr<SatelliteNetDevice> dev2 = CreateObject<SatelliteNetDevice>();

    dev1->SetNode(sat1);
    dev2->SetNode(sat2);

    sat1->AddDevice(dev1);
    sat2->AddDevice(dev2);

    Ptr<SatelliteLink> link = CreateObject<SatelliteLink>(dev1, dev2);
    link->SetMaxRange(10000000.0); // 10,000 km
    link->SetPropagationDelayModel(CreateObject<IslPropagationDelayModel>());

    Ptr<SatelliteMobilityModel> mob1 = CreateObject<SatelliteMobilityModel>();
    mob1->SetSatellite(sat1);
    sat1->AggregateObject(mob1);

    Ptr<SatelliteMobilityModel> mob2 = CreateObject<SatelliteMobilityModel>();
    mob2->SetSatellite(sat2);
    sat2->AggregateObject(mob2);

    m_received = false;
    dev2->SetReceiveCallback(MakeCallback(&IslChannelTestCase::OnReceive, this));

    Ptr<Packet> packet = Create<Packet>(100);

    bool sent = dev1->Send(packet, dev2->GetAddress(), 0x0800);
    NS_TEST_EXPECT_MSG_EQ(sent, true, "dev1 should send packet to dev2");

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_EXPECT_MSG_EQ(m_received, true, "packet should be received when satellites are in range");

    // now set very small range and ensure no reception
    link->SetMaxRange(1.0);

    m_received = false;
    packet = Create<Packet>(100);
    sent = dev1->Send(packet, dev2->GetAddress(), 0x0800);
    NS_TEST_EXPECT_MSG_EQ(sent, false, "dev1 send should fail when destination is not within max range");

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_EXPECT_MSG_EQ(m_received, false, "packet should not be received when distance > maxRange");
}

class OrbitShieldIslTestSuite : public TestSuite
{
  public:
    OrbitShieldIslTestSuite();
};

OrbitShieldIslTestSuite::OrbitShieldIslTestSuite()
    : TestSuite("orbitshield-isl", Type::UNIT)
{
    AddTestCase(new IslChannelTestCase(), Duration::QUICK);
}

static OrbitShieldIslTestSuite orbitshieldIslTestSuite;
