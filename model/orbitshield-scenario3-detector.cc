/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "orbitshield-scenario3-detector.h"

#include <algorithm>

namespace ns3
{

void
OrbitShieldScenario3Detector::SetMinSamples(uint32_t minSamples)
{
    m_minSamples = std::max(1u, minSamples);
}

void
OrbitShieldScenario3Detector::SetTargetPdrThreshold(double threshold)
{
    m_targetPdrThreshold = std::max(0.0, std::min(1.0, threshold));
}

void
OrbitShieldScenario3Detector::SetScoreThreshold(double threshold)
{
    m_scoreThreshold = std::max(0.0, threshold);
}

void
OrbitShieldScenario3Detector::SetMaxFlaggedSatellites(uint32_t maxFlaggedSatellites)
{
    m_maxFlaggedSatellites = maxFlaggedSatellites;
}

void
OrbitShieldScenario3Detector::ObserveWindow(const OrbitShieldScenario3FlowSample& sample,
                                            const std::vector<std::string>& routeSatellites,
                                            bool targetFlow)
{
    if (!targetFlow || sample.sent < m_minSamples || sample.pdr >= m_targetPdrThreshold)
    {
        return;
    }

    for (const auto& satelliteName : routeSatellites)
    {
        if (m_scores.find(satelliteName) == m_scores.end())
        {
            m_observationOrder.push_back(satelliteName);
        }
        m_scores[satelliteName] += 1.0;
    }
}

void
OrbitShieldScenario3Detector::Clear()
{
    m_scores.clear();
    m_observationOrder.clear();
}

double
OrbitShieldScenario3Detector::GetScore(const std::string& satelliteName) const
{
    auto scoreIt = m_scores.find(satelliteName);
    return scoreIt == m_scores.end() ? 0.0 : scoreIt->second;
}

std::vector<std::string>
OrbitShieldScenario3Detector::GetFlaggedSatellites() const
{
    std::vector<std::string> flaggedSatellites;
    if (m_maxFlaggedSatellites == 0)
    {
        return flaggedSatellites;
    }

    for (const auto& satelliteName : m_observationOrder)
    {
        if (GetScore(satelliteName) >= m_scoreThreshold)
        {
            flaggedSatellites.push_back(satelliteName);
            if (flaggedSatellites.size() >= m_maxFlaggedSatellites)
            {
                break;
            }
        }
    }
    return flaggedSatellites;
}

} // namespace ns3