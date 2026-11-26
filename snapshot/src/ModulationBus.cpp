#include "ModulationBus.h"
#include <cmath>

void ModulationBus::prepare(double sr)
{
    sampleRate = sr;
    lfo1Phase = 0;
    lfo2Phase = 0;
    sourceValues.fill(0.0f);
    offsets.fill(0.0f);
}

float ModulationBus::computeLFO(double& phase, float rate, int shape, int numSamples)
{
    // Advance phase to block midpoint
    double advance = (rate * numSamples * 0.5) / sampleRate;
    phase += advance;
    if (phase >= 1.0) phase -= std::floor(phase);

    float p = static_cast<float>(phase);
    float value = 0.0f;

    switch (shape)
    {
        case 0: // Sine
            value = std::sin(p * 2.0f * juce::MathConstants<float>::pi);
            break;
        case 1: // Triangle
            value = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
            break;
        case 2: // Saw
            value = 2.0f * p - 1.0f;
            break;
        case 3: // Square
            value = (p < 0.5f) ? 1.0f : -1.0f;
            break;
    }

    // Advance remaining half
    phase += advance;
    if (phase >= 1.0) phase -= std::floor(phase);

    return value;
}

float ModulationBus::applyCurve(float input, CurveType curve)
{
    // Input is expected to be -1..+1, we preserve sign
    float sign = (input >= 0.0f) ? 1.0f : -1.0f;
    float abs = std::fabs(input);

    switch (curve)
    {
        case CurveType::Linear:
            return input;
        case CurveType::Exponential:
            return sign * abs * abs;
        case CurveType::SCurve:
        {
            float t = abs * abs * (3.0f - 2.0f * abs); // smoothstep
            return sign * t;
        }
        case CurveType::Logarithmic:
            return sign * std::log2(1.0f + abs);
        case CurveType::DualSigmoid:
        {
            float x = abs;
            float threshold1 = 0.3f, threshold2 = 0.7f;
            float k1 = 10.0f, k2 = 15.0f;
            float sig1 = 1.0f / (1.0f + std::exp(-k1 * (x - threshold1)));
            float sig2 = 1.0f / (1.0f + std::exp(-k2 * (x - threshold2)));
            float result = sig1 * 0.5f + sig2 * 0.5f;
            return std::copysign(result, input);
        }
    }
    return input;
}

void ModulationBus::computeBlock(const DiagnosticState& state, int numSamples)
{
    offsets.fill(0.0f);

    // Read source values from diagnostic state
    sourceValues[static_cast<int>(ModSource::InputLevel)] =
        state.input.rmsL.load(std::memory_order_relaxed);
    sourceValues[static_cast<int>(ModSource::GlobalEnergy)] =
        state.globalEnergy.load(std::memory_order_relaxed);
    sourceValues[static_cast<int>(ModSource::BandEnergy0)] =
        state.bandEnergy[0].load(std::memory_order_relaxed);
    sourceValues[static_cast<int>(ModSource::BandEnergy1)] =
        state.bandEnergy[1].load(std::memory_order_relaxed);
    sourceValues[static_cast<int>(ModSource::BandEnergy2)] =
        state.bandEnergy[2].load(std::memory_order_relaxed);
    sourceValues[static_cast<int>(ModSource::BandEnergy3)] =
        state.bandEnergy[3].load(std::memory_order_relaxed);
    sourceValues[static_cast<int>(ModSource::OutputLevel)] =
        state.output.rmsL.load(std::memory_order_relaxed);

    // Compute LFOs
    sourceValues[static_cast<int>(ModSource::LFO1)] =
        computeLFO(lfo1Phase,
                   state.lfo1Rate.load(std::memory_order_relaxed),
                   state.lfo1Shape.load(std::memory_order_relaxed),
                   numSamples);
    sourceValues[static_cast<int>(ModSource::LFO2)] =
        computeLFO(lfo2Phase,
                   state.lfo2Rate.load(std::memory_order_relaxed),
                   state.lfo2Shape.load(std::memory_order_relaxed),
                   numSamples);

    // Process routes
    int routeCount = 0;
    const ModRoute* routes = state.getActiveRoutes(routeCount);
    auto& specs = getTargetSpecs();

    for (int i = 0; i < routeCount; ++i)
    {
        const auto& route = routes[i];
        if (!route.enabled) continue;

        int srcIdx = static_cast<int>(route.source);
        int tgtIdx = static_cast<int>(route.target);

        if (srcIdx < 0 || srcIdx >= static_cast<int>(ModSource::Count)) continue;
        if (tgtIdx < 0 || tgtIdx >= static_cast<int>(ModTarget::Count)) continue;

        float srcVal = sourceValues[srcIdx];
        float curved = applyCurve(srcVal, route.curve);
        float range = specs[tgtIdx].maxVal - specs[tgtIdx].minVal;

        offsets[tgtIdx] += curved * route.amount * range;
    }
}

float ModulationBus::getOffset(ModTarget target) const
{
    int idx = static_cast<int>(target);
    if (idx >= 0 && idx < static_cast<int>(ModTarget::Count))
        return offsets[idx];
    return 0.0f;
}
