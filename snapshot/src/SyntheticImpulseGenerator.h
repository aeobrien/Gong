#pragma once

#include <JuceHeader.h>
#include "StrikeDescriptor.h"

/**
 * Generates shaped noise bursts from StrikeDescriptor data.
 * Follows the same state-machine pattern as ImpulseGenerator
 * (samplesRemaining counter, fills output buffer across multiple process() calls).
 *
 * Descriptor → impulse mapping:
 *   velocity       → burst amplitude (power curve)
 *   attackSlope    → attack phase length (1 sample at 255, ~3ms at 0)
 *   spectralCentroid → lowpass filter cutoff on noise (200-15000 Hz)
 *   hfEnergyRatio  → filtered vs unfiltered noise mix + filter Q
 *   decayShape     → decay phase length (~0.5ms at 0, ~5ms at 255)
 *   padId          → routing only (not used by generator)
 */
class SyntheticImpulseGenerator
{
public:
    SyntheticImpulseGenerator();
    ~SyntheticImpulseGenerator() = default;

    void prepare(double sampleRate, int blockSize);
    void trigger(const StrikeDescriptor& descriptor);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

    bool isActive() const { return samplesRemaining > 0; }

private:
    double sampleRate = 48000.0;
    int blockSize = 256;

    // Current impulse state
    int samplesRemaining = 0;
    int totalSamples = 0;
    int attackSamples = 0;
    int decaySamples = 0;
    float amplitude = 0.0f;
    float hfMix = 0.5f;    // 0=LP only, 1=white noise

    // Filter for spectral shaping
    juce::dsp::StateVariableTPTFilter<float> noiseFilter;
    bool filterPrepared = false;

    juce::Random random;

    // Pending trigger (thread-safe)
    std::atomic<bool> triggerPending { false };
    StrikeDescriptor pendingDescriptor;
    juce::SpinLock descriptorLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SyntheticImpulseGenerator)
};
