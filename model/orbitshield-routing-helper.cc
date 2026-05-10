/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "orbitshield-routing-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OrbitShieldRoutingHelper");

OrbitShieldRoutingHelper::OrbitShieldRoutingHelper()
{
    NS_LOG_FUNCTION(this);
}

OrbitShieldRoutingHelper::~OrbitShieldRoutingHelper()
{
    NS_LOG_FUNCTION(this);
}

void
OrbitShieldRoutingHelper::Install(Ptr<Constellation> constellation)
{
    NS_LOG_FUNCTION(this << constellation);
    // Stub: no-op in Milestone 1.2
    // Real implementation deferred to Milestone 3
}

void
OrbitShieldRoutingHelper::RecomputeRoutes(Ptr<Constellation> constellation)
{
    NS_LOG_FUNCTION(this << constellation);
    // Stub: no-op in Milestone 1.2
    // Real implementation deferred to Milestone 4
}

}  // namespace ns3
