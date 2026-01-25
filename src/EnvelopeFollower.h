#pragma once

#include <JuceHeader.h>

/**
 * Envelope follower that tracks amplitude of incoming audio for parameter modulation.
 * Uses peak detection with configurable attack and release times.
 */
class EnvelopeFollower
{
public:
    EnvelopeFollower();
    ~EnvelopeFollower() = default;

    void prepare(double sampleRate);
    void reset();

    // Process a block and return the smoothed level (0.0 to 1.0)
    float process(const juce::AudioBuffer<float>& input);

    // Process a single sample (for sample-by-sample operation)
    float processSample(float inputSample);

    // Settings
    void setAttackMs(float ms);
    float getAttackMs() const { return attackMs; }

    void setReleaseMs(float ms);
    float getReleaseMs() const { return releaseMs; }

    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() const { return isEnabled; }

    // Current level (can be read from UI thread)
    float getCurrentLevel() const { return currentLevel.load(); }

private:
    void updateCoefficients();

    double sampleRate = 48000.0;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    std::atomic<float> currentLevel { 0.0f };

    bool isEnabled = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeFollower)
};
