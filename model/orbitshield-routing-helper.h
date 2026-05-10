/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_ROUTING_HELPER_H
#define ORBITSHIELD_ROUTING_HELPER_H

#include "ns3/callback.h"
#include "ns3/ptr.h"

namespace ns3
{

class Constellation;

/**
 * \brief Helper class for installing and managing routing in an OrbitShield constellation
 *
 * This helper provides a simple interface for installing routing configuration
 * and managing route updates in response to topology changes.
 * 
 * This is a plain C++ helper class (not an ns-3 Object) designed to configure
 * routing behavior on constellation nodes.
 */
class OrbitShieldRoutingHelper
{
  public:
    /**
     * \brief Constructor
     */
    OrbitShieldRoutingHelper();

    /**
     * \brief Destructor
     */
    ~OrbitShieldRoutingHelper();

    /**
     * \brief Install routing configuration on all nodes in the constellation
     * 
     * \param constellation Pointer to the constellation to configure
     */
    void Install(Ptr<Constellation> constellation);

    /**
     * \brief Recompute and update routes for all nodes in the constellation
     * 
     * This method recalculates routing paths based on the current topology
     * and updates routing tables accordingly.
     * 
     * \param constellation Pointer to the constellation
     */
    void RecomputeRoutes(Ptr<Constellation> constellation);
};

}  // namespace ns3

#endif /* ORBITSHIELD_ROUTING_HELPER_H */
