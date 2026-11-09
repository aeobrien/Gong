#pragma once

#include <JuceHeader.h>

/**
 * Harmonic exciter using JUCE DSP modules.
 * Signal chain: Input -> Highpass -> Saturation -> Dry/Wet Mix -> Output
 */
class ExciterProcessor
{
public:
    ExciterProcessor();
    ~ExciterProcessor() = default;

    void prepare(double sampleRate, int blockSize);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

    // Parameters
    void setEnabled(bool enabled);
    bool getEnabled() const { return isEnabled; }

    void setHighpassFrequency(float hz);
    float getHighpassFrequency() const { return highpassFreq; }

    void setSaturationDrive(float drive);  // 1.0 = unity, higher = more saturation
    float getSaturationDrive() const { return saturationDrive; }

    void setDryWetMix(float wet);  // 0.0 = all dry, 1.0 = all wet
    float getDryWetMix() const { return dryWetMix; }

    void setOutputGainDb(float db);
    float getOutputGainDb() const { return outputGainDb; }

private:
    void updateHighpass();

    double sampleRate = 48000.0;

    bool isEnabled = true;

    float highpassFreq = 500.0f;
    float saturationDrive = 2.0f;
    float dryWetMix = 0.3f;
    float outputGainDb = 0.0f;

    // JUCE DSP components
    juce::dsp::StateVariableTPTFilter<float> highpassL;
    juce::dsp::StateVariableTPTFilter<float> highpassR;
    juce::dsp::DryWetMixer<float> mixer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExciterProcessor)
};
