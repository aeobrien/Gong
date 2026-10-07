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

    // Step 14: Second IR for energy-based crossfade
    bool loadImpulseResponseB(const juce::File& file);
    void setIRCrossfadeEnergy(float energy);  // Call per-block with global energy

    void setWetDryMix(float wetRatio); // 0.0 = dry, 1.0 = wet
    float getWetDryMix() const { return wetDryMix; }

    void setOutputGainDb(float gainDb);
    float getOutputGainDb() const { return outputGainDb; }

    bool isLoaded() const { return irLoaded; }
    bool isLoadedB() const { return irLoadedB; }

    // IR buffer access for visualization
    const juce::AudioBuffer<float>* getIRBuffer() const { return &irBuffer; }
    int getIRLengthSamples() const { return irBuffer.getNumSamples(); }
    int getIRSampleRate() const { return irSampleRate; }
    float getIRLengthSeconds() const;
    juce::String getIRFileName() const { return irFileName; }
    juce::String getIRFileNameB() const { return irFileNameB; }

    // Step 8: Post-convolution EQ
    void setPostConvLowGainDb(float db);
    float getPostConvLowGainDb() const { return postConvLowGainDb; }
    void setPostConvMidGainDb(float db);
    float getPostConvMidGainDb() const { return postConvMidGainDb; }
    void setPostConvHighGainDb(float db);
    float getPostConvHighGainDb() const { return postConvHighGainDb; }

private:
    // NonUniform partitioned convolution — uses small FFTs for the head
    // and progressively larger FFTs for the tail, making long IRs feasible.
    juce::dsp::Convolution convolution { juce::dsp::Convolution::NonUniform { 4096 } };
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Step 14: Second convolution for multi-IR crossfade
    juce::dsp::Convolution convolutionB { juce::dsp::Convolution::NonUniform { 4096 } };

    // Pre-allocated dry buffer (avoid heap allocation on audio thread)
    juce::AudioBuffer<float> dryBuffer;

    // Step 14: Pre-allocated buffers for IR crossfade
    juce::AudioBuffer<float> convABuffer, convBBuffer;

    // Store loaded IR for visualization
    juce::AudioBuffer<float> irBuffer;
    int irSampleRate = 0;
    juce::String irFileName;
    juce::String irFileNameB;

    // Step 8: Post-convolution EQ filters
    juce::dsp::StateVariableTPTFilter<float> postLowShelfL, postLowShelfR;
    juce::dsp::StateVariableTPTFilter<float> postHighShelfL, postHighShelfR;
    juce::dsp::StateVariableTPTFilter<float> postMidPeakL, postMidPeakR;
    float postConvLowGainDb = 0.0f;
    float postConvMidGainDb = 0.0f;
    float postConvHighGainDb = 0.0f;
    bool eqNeedsUpdate = true;

    double currentSampleRate = 48000.0;
    int currentBlockSize = 256;
    float wetDryMix = 0.5f;
    float outputGainDb = 0.0f;
    bool irLoaded = false;
    bool irLoadedB = false;
    bool isPrepared = false;

    // Step 14: IR crossfade
    float irCrossfade = 0.0f;       // 0 = IR A only, 1 = IR B only
    float irCrossfadeTarget = 0.0f;
    float irCrossfadeSmoothing = 0.01f;

    // Guard against concurrent prepare() and process() calls
    std::atomic<bool> isLoadingIR { false };

    void updatePostConvEQ();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConvolutionEngine)
};
