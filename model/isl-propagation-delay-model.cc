/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "isl-propagation-delay-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("IslPropagationDelayModel");

NS_OBJECT_ENSURE_REGISTERED(IslPropagationDelayModel);

TypeId
IslPropagationDelayModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::IslPropagationDelayModel")
            .SetParent<PropagationDelayModel>()
            .SetGroupName("OrbitShield")
            .AddConstructor<IslPropagationDelayModel>();
    return tid;
}

IslPropagationDelayModel::IslPropagationDelayModel()
{
    NS_LOG_FUNCTION(this);
}

IslPropagationDelayModel::~IslPropagationDelayModel()
{
    NS_LOG_FUNCTION(this);
}

Time
IslPropagationDelayModel::GetDelay(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this << a << b);
    if (a == nullptr || b == nullptr)
    {
        return Seconds(0.0);
    }

    double distance = a->GetDistanceFrom(b); // meters
    static const double c = 299792458.0;     // speed of light m/s
    return Seconds(distance / c);
}

int64_t
IslPropagationDelayModel::DoAssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    return 0;
}

} // namespace ns3
