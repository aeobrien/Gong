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
    // NonUniform partitioned convolution — uses small FFTs for the head
    // and progressively larger FFTs for the tail, making long IRs feasible.
    juce::dsp::Convolution convolution { juce::dsp::Convolution::NonUniform { 4096 } };
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Pre-allocated dry buffer (avoid heap allocation on audio thread)
    juce::AudioBuffer<float> dryBuffer;

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

    // Guard against concurrent prepare() and process() calls
    std::atomic<bool> isLoadingIR { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConvolutionEngine)
};
