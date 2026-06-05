/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_SCENARIO3_DETECTOR_H
#define ORBITSHIELD_SCENARIO3_DETECTOR_H

#include "orbitshield-scenario3-telemetry.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ns3
{

class OrbitShieldScenario3Detector
{
  public:
    void SetMinSamples(uint32_t minSamples);
    void SetTargetPdrThreshold(double threshold);
    void SetScoreThreshold(double threshold);
    void SetMaxFlaggedSatellites(uint32_t maxFlaggedSatellites);

    void ObserveWindow(const OrbitShieldScenario3FlowSample& sample,
                       const std::vector<std::string>& routeSatellites,
                       bool targetFlow);
    void Clear();

    double GetScore(const std::string& satelliteName) const;
    std::vector<std::string> GetFlaggedSatellites() const;

  private:
    uint32_t m_minSamples{1};
    double m_targetPdrThreshold{0.6};
    double m_scoreThreshold{1.0};
    uint32_t m_maxFlaggedSatellites{4};
    std::unordered_map<std::string, double> m_scores;
    std::vector<std::string> m_observationOrder;
};

} // namespace ns3

#endif /* ORBITSHIELD_SCENARIO3_DETECTOR_H */