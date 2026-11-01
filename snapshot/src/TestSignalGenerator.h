#pragma once

#include <JuceHeader.h>

class TestSignalGenerator
{
public:
    TestSignalGenerator() = default;

    void prepare(double sampleRate);
    void generateBlock(juce::AudioBuffer<float>& dest, int numSamples,
                       int signalType, float gain, float sineFreq);

private:
    double sampleRate = 48000.0;
    double phase = 0.0;
    double sweepPhase = 0.0;
    int sweepSampleCount = 0;
    int impulseSampleCount = 0;

    // Pink noise filter state (Paul Kellet's method)
    float pinkState[7] = {};

    juce::Random random;
};
