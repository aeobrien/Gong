#pragma once

#include <JuceHeader.h>

/**
 * 3-band compressor using Linkwitz-Riley crossovers.
 * Signal chain: Input -> Split (200Hz, 2kHz) -> [Low Comp] + [Mid Comp] + [High Comp] -> Sum -> Output
 */
class MultibandCompressor
{
public:
    static constexpr int kNumBands = 3;

    // Per-band settings
    struct BandSettings
    {
        float threshold = -12.0f;   // dB
        float ratio = 4.0f;         // compression ratio
        float attackMs = 10.0f;
        float releaseMs = 100.0f;
        float makeupGainDb = 0.0f;
        bool enabled = true;
    };

    MultibandCompressor();
    ~MultibandCompressor() = default;

    void prepare(double sampleRate, int blockSize);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

    // Enable/disable entire processor
    void setEnabled(bool enabled);
    bool getEnabled() const { return isEnabled; }

    // Crossover frequencies
    void setLowMidCrossover(float hz);
    float getLowMidCrossover() const { return lowMidCrossover; }

    void setMidHighCrossover(float hz);
    float getMidHighCrossover() const { return midHighCrossover; }

    // Per-band settings
    void setBandSettings(int band, const BandSettings& settings);
    const BandSettings& getBandSettings(int band) const;

    // Convenience setters for all bands
    void setAllThresholds(float db);
    void setAllRatios(float ratio);
    void setAllAttacks(float ms);
    void setAllReleases(float ms);

    // Output gain
    void setOutputGainDb(float db);
    float getOutputGainDb() const { return outputGainDb; }

private:
    void updateCrossovers();

    double sampleRate = 48000.0;
    bool isEnabled = true;

    float lowMidCrossover = 200.0f;
    float midHighCrossover = 2000.0f;
    float outputGainDb = 0.0f;

    std::array<BandSettings, kNumBands> bandSettings;

    // Linkwitz-Riley crossover filters (4th order = two 2nd order cascaded)
    // Low/Mid crossover
    juce::dsp::LinkwitzRileyFilter<float> lowMidLowpassL;
    juce::dsp::LinkwitzRileyFilter<float> lowMidLowpassR;
    juce::dsp::LinkwitzRileyFilter<float> lowMidHighpassL;
    juce::dsp::LinkwitzRileyFilter<float> lowMidHighpassR;

    // Mid/High crossover
    juce::dsp::LinkwitzRileyFilter<float> midHighLowpassL;
    juce::dsp::LinkwitzRileyFilter<float> midHighLowpassR;
    juce::dsp::LinkwitzRileyFilter<float> midHighHighpassL;
    juce::dsp::LinkwitzRileyFilter<float> midHighHighpassR;

    // Per-band compressors
    std::array<juce::dsp::Compressor<float>, kNumBands> compressorsL;
    std::array<juce::dsp::Compressor<float>, kNumBands> compressorsR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultibandCompressor)
};
