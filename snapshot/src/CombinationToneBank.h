#pragma once

#include <JuceHeader.h>
#include "EnergyAccumulator.h"
#include "ResonatorBank.h"

/**
 * Step 12: Combination tone resonators.
 * When two strong modes interact, sum and difference frequencies emerge.
 * This adds a lightweight bank of extra resonators at those frequencies.
 */
class CombinationToneBank
{
public:
    static constexpr int kNumTones = 8;

    CombinationToneBank();
    ~CombinationToneBank() = default;

    void prepare(double sampleRate, int blockSize);
    void reset();

    // Update combination tone frequencies based on current energy and resonator state
    void update(const EnergyAccumulator& energy, const ResonatorBank& resonators);

    // Process audio - adds combination tones to output
    void process(juce::AudioBuffer<float>& buffer);

    void setMixLevel(float level);
    float getMixLevel() const { return mixLevel; }

    void setActivationThreshold(float threshold);
    float getActivationThreshold() const { return activationThreshold; }

private:
    struct CombTone {
        float freq = 440.0f;
        float gain = 0.0f;
        juce::dsp::StateVariableTPTFilter<float> filterL, filterR;
    };

    std::array<CombTone, kNumTones> tones;
    double sampleRate = 48000.0;
    float mixLevel = 0.15f;
    float activationThreshold = 0.3f;
    bool isPrepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CombinationToneBank)
};
