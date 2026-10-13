#pragma once

#include "DiagnosticState.h"

class ModulationBus
{
public:
    ModulationBus() = default;

    void prepare(double sampleRate);
    void computeBlock(const DiagnosticState& state, int numSamples);
    float getOffset(ModTarget target) const;

private:
    double sampleRate = 48000.0;
    double lfo1Phase = 0, lfo2Phase = 0;
    std::array<float, static_cast<int>(ModSource::Count)> sourceValues{};
    std::array<float, static_cast<int>(ModTarget::Count)> offsets{};

    static float applyCurve(float input, CurveType curve);
    float computeLFO(double& phase, float rate, int shape, int numSamples);
};
