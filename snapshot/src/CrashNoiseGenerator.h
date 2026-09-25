#pragma once

#include <JuceHeader.h>

/**
 * Step 13: Crash noise generator.
 * At extreme energy levels, gongs produce broadband noise from chaotic vibration.
 * Activates above a threshold and adds bandpass-filtered noise.
 */
class CrashNoiseGenerator
{
public:
    CrashNoiseGenerator();
    ~CrashNoiseGenerator() = default;

    void prepare(double sampleRate, int blockSize);
    void reset();

    // Process - adds crash noise based on global energy level
    void process(float globalEnergy, juce::AudioBuffer<float>& buffer);

    void setThreshold(float threshold);
    float getThreshold() const { return threshold; }

    void setMixLevel(float level);
    float getMixLevel() const { return mixLevel; }

private:
    float threshold = 0.85f;
    float mixLevel = 0.3f;
    float currentNoiseLevel = 0.0f;  // Smoothed for click-free onset/offset
    float smoothingCoeff = 0.01f;

    juce::dsp::StateVariableTPTFilter<float> bandpassL, bandpassR;
    juce::Random random;
    bool isPrepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrashNoiseGenerator)
};
