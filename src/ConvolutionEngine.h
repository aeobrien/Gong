#pragma once

#include <JuceHeader.h>

class ConvolutionEngine
{
public:
    ConvolutionEngine();
    ~ConvolutionEngine() = default;

    void prepare(double sampleRate, int blockSize);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

    bool loadImpulseResponse(const juce::File& file);
    bool loadImpulseResponseFromData(const void* data, size_t dataSize);

    void setWetDryMix(float wetRatio); // 0.0 = dry, 1.0 = wet
    float getWetDryMix() const { return wetDryMix; }

    void setOutputGainDb(float gainDb);
    float getOutputGainDb() const { return outputGainDb; }

    bool isLoaded() const { return irLoaded; }

    // IR buffer access for visualization
    const juce::AudioBuffer<float>* getIRBuffer() const { return &irBuffer; }
    int getIRLengthSamples() const { return irBuffer.getNumSamples(); }
    int getIRSampleRate() const { return irSampleRate; }
    float getIRLengthSeconds() const;
    juce::String getIRFileName() const { return irFileName; }

private:
    // Default convolution - works up to ~10 seconds
    // NonUniform mode tested but has same limitation
    juce::dsp::Convolution convolution;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Store loaded IR for visualization
    juce::AudioBuffer<float> irBuffer;
    int irSampleRate = 0;
    juce::String irFileName;

    double currentSampleRate = 48000.0;
    int currentBlockSize = 256;
    float wetDryMix = 0.5f;
    float outputGainDb = 0.0f;
    bool irLoaded = false;
    bool isPrepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConvolutionEngine)
};
