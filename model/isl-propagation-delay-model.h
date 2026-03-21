/*
 * Copyright (c) 2026 Marco A. F. Barbosa
 */

#ifndef ORBITSHIELD_ISL_PROPAGATION_DELAY_MODEL_H
#define ORBITSHIELD_ISL_PROPAGATION_DELAY_MODEL_H

#include "ns3/propagation-delay-model.h"

namespace ns3
{

class IslPropagationDelayModel : public PropagationDelayModel
{
  public:
    static TypeId GetTypeId();

    IslPropagationDelayModel();
    ~IslPropagationDelayModel() override;

    Time GetDelay(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

  private:
    int64_t DoAssignStreams(int64_t stream) override;
};

} // namespace ns3

#endif /* ORBITSHIELD_ISL_PROPAGATION_DELAY_MODEL_H */
