/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_ROUTING_HELPER_H
#define ORBITSHIELD_ROUTING_HELPER_H

#include "ns3/callback.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/node.h"
#include "ns3/ptr.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

    void SetExcludedSatellites(const std::vector<std::string>& satelliteNames);
    void AddExcludedSatellite(const std::string& satelliteName);
    void ClearExcludedSatellites();
    std::vector<std::string> GetExcludedSatellites() const;

    /**
     * \brief Get the last recomputed node path for a source/destination route.
     *
     * The returned path includes the source node and destination node. An empty
     * vector means no route was available during the last recomputation.
     *
     * \param source Source node.
     * \param destination Destination IPv4 host address.
     * \return Ordered node path from source to destination.
     */
    std::vector<Ptr<Node>> GetRoutePath(Ptr<Node> source, Ipv4Address destination) const;

    /**
     * \brief Get the hop count for the last recomputed source/destination route.
     * \param source Source node.
     * \param destination Destination IPv4 host address.
     * \return Number of next-hop transitions, or 0 when no path is known.
     */
    uint32_t GetRouteHopCount(Ptr<Node> source, Ipv4Address destination) const;

    /**
     * \brief Get satellite names used as transit nodes on the last recomputed route.
     * \param source Source node.
     * \param destination Destination IPv4 host address.
     * \return Ordered satellite names, excluding ground-station endpoints.
     */
    std::vector<std::string> GetTransitSatelliteNames(Ptr<Node> source,
                              Ipv4Address destination) const;

    private:
    struct RoutePathKey
    {
      uint32_t sourceNodeId{0};
      uint32_t destinationAddress{0};

      bool operator==(const RoutePathKey& other) const;
    };

    struct RoutePathKeyHash
    {
      std::size_t operator()(const RoutePathKey& key) const;
    };

    bool IsExcludedSatellite(Ptr<Node> node) const;

    std::unordered_map<RoutePathKey, std::vector<Ptr<Node>>, RoutePathKeyHash> m_lastRoutePaths;
    std::unordered_set<std::string> m_excludedSatellites;
};

}  // namespace ns3

#endif /* ORBITSHIELD_ROUTING_HELPER_H */
