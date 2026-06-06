/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#include "targeted-flow-grayhole-detector.h"

#include <algorithm>

namespace ns3
{

void
OrbitShieldTargetedFlowGrayholeDetector::SetMinSamples(uint32_t minSamples)
{
    m_minSamples = std::max(1u, minSamples);
}

void
OrbitShieldTargetedFlowGrayholeDetector::SetTargetPdrThreshold(double threshold)
{
    m_targetPdrThreshold = std::max(0.0, std::min(1.0, threshold));
}

void
OrbitShieldTargetedFlowGrayholeDetector::SetScoreThreshold(double threshold)
{
    m_scoreThreshold = std::max(0.0, threshold);
}

void
OrbitShieldTargetedFlowGrayholeDetector::SetMaxFlaggedSatellites(uint32_t maxFlaggedSatellites)
{
    m_maxFlaggedSatellites = maxFlaggedSatellites;
}

void
OrbitShieldTargetedFlowGrayholeDetector::ObserveWindow(const OrbitShieldTargetedFlowGrayholeFlowSample& sample,
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
OrbitShieldTargetedFlowGrayholeDetector::Clear()
{
    m_scores.clear();
    m_observationOrder.clear();
}

double
OrbitShieldTargetedFlowGrayholeDetector::GetScore(const std::string& satelliteName) const
{
    auto scoreIt = m_scores.find(satelliteName);
    return scoreIt == m_scores.end() ? 0.0 : scoreIt->second;
}

std::vector<std::string>
OrbitShieldTargetedFlowGrayholeDetector::GetFlaggedSatellites() const
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