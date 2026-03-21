/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "satellite-mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SatelliteMobilityModel");

NS_OBJECT_ENSURE_REGISTERED(SatelliteMobilityModel);

TypeId
SatelliteMobilityModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatelliteMobilityModel")
            .SetParent<MobilityModel>()
            .SetGroupName("OrbitShield")
            .AddConstructor<SatelliteMobilityModel>();
    return tid;
}

SatelliteMobilityModel::SatelliteMobilityModel()
    : m_satellite(nullptr)
{
    NS_LOG_FUNCTION(this);
}

SatelliteMobilityModel::~SatelliteMobilityModel()
{
    NS_LOG_FUNCTION(this);
}

void
SatelliteMobilityModel::SetSatellite(Ptr<Satellite> satellite)
{
    NS_LOG_FUNCTION(this << satellite);
    m_satellite = satellite;
}

Ptr<Satellite>
SatelliteMobilityModel::GetSatellite() const
{
    return m_satellite;
}

Ptr<MobilityModel>
SatelliteMobilityModel::Copy() const
{
    Ptr<SatelliteMobilityModel> copy = CreateObject<SatelliteMobilityModel>();
    copy->m_satellite = m_satellite;
    return copy;
}

Vector
SatelliteMobilityModel::DoGetPosition() const
{
    NS_LOG_FUNCTION(this);
    if (m_satellite)
    {
        return m_satellite->GetPosition(Simulator::Now());
    }
    return Vector(0.0, 0.0, 0.0);
}

void
SatelliteMobilityModel::DoSetPosition(const Vector &position)
{
    NS_LOG_FUNCTION(this << position);
    NS_ASSERT_MSG(false, "SatelliteMobilityModel position is driven by orbital propagator and cannot be set manually");
}

Vector
SatelliteMobilityModel::DoGetVelocity() const
{
    NS_LOG_FUNCTION(this);
    if (m_satellite)
    {
        return m_satellite->GetVelocity(Simulator::Now());
    }
    return Vector(0.0, 0.0, 0.0);
}

int64_t
SatelliteMobilityModel::DoAssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    return 0;
}

} // namespace ns3
