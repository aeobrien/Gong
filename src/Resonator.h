#pragma once

#include <JuceHeader.h>

class Resonator
{
public:
    static constexpr int kMaxModes = 8;

    // Gong-like modal frequency ratios (relative to fundamental)
    // Based on analysis of gamelan gong tones
    static constexpr float kGongModeRatios[kMaxModes] = {
        1.0f,    // Fundamental
        2.76f,   // Second partial
        5.40f,   // Third partial
        8.93f,   // Fourth partial
        13.34f,  // Fifth partial
        18.64f,  // Sixth partial
        24.82f,  // Seventh partial
        31.87f   // Eighth partial
    };

    // Default decay multipliers for each mode (higher modes decay faster)
    static constexpr float kDefaultModeDecayMultipliers[kMaxModes] = {
        1.0f, 0.85f, 0.72f, 0.60f, 0.50f, 0.42f, 0.35f, 0.30f
    };

    // Per-mode parameter structure
    struct ModeParams
    {
        float frequencyRatio = 1.0f;    // Multiplier relative to base ratio (0.5 to 2.0)
        float decayMultiplier = 1.0f;   // Multiplier relative to global decay (0.1 to 3.0)
        float gainDb = 0.0f;            // Gain adjustment in dB (-24 to +6)
        float detuneCents = 0.0f;       // Detune in cents (-100 to +100)
        float stereoWidth = 0.0f;       // Stereo width (0 = mono, 1 = full stereo)
        bool enabled = true;            // Whether this mode is enabled
    };

    Resonator();
    ~Resonator() = default;

    void prepare(double sampleRate, int blockSize);
    void process(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output);
    void reset();

    // Global parameters
    void setFundamentalFrequency(float frequencyHz);
    float getFundamentalFrequency() const { return fundamentalFrequency; }

    void setDecayTime(float decaySeconds);
    float getDecayTime() const { return decayTime; }

    void setBrightness(float brightness); // 0.0 = dark, 1.0 = bright
    float getBrightness() const { return brightness; }

    void setNumActiveModes(int numModes);
    int getNumActiveModes() const { return numActiveModes; }

    // Per-mode parameter access
    const ModeParams& getModeParams(int mode) const;
    void setModeParams(int mode, const ModeParams& params);

    // Individual per-mode setters for convenience
    void setModeFrequencyRatio(int mode, float ratio);
    void setModeDecayMultiplier(int mode, float multiplier);
    void setModeGainDb(int mode, float gainDb);
    void setModeDetuneCents(int mode, float cents);
    void setModeStereoWidth(int mode, float width);
    void setModeEnabled(int mode, bool enabled);

    // Get base frequency ratio for a mode (before user adjustment)
    float getBaseModeRatio(int mode) const;

    // Get the effective frequency for a mode (including all adjustments)
    float getEffectiveModeFrequency(int mode) const;

    // Master output gain (in dB)
    void setMasterGainDb(float gainDb);
    float getMasterGainDb() const { return masterGainDb; }

private:
    void updateFilters();
    float centsToRatio(float cents) const;

    double sampleRate = 48000.0;
    int blockSize = 256;

    float fundamentalFrequency = 220.0f; // A3
    float decayTime = 3.0f;              // seconds
    float brightness = 0.7f;             // controls high mode damping
    int numActiveModes = 6;
    float masterGainDb = 0.0f;           // master output gain in dB

    // Per-mode parameters
    std::array<ModeParams, kMaxModes> modeParams;

    // Biquad filters for each mode (stereo - L and R can have different frequencies for width)
    std::array<juce::dsp::IIR::Filter<float>, kMaxModes> filtersL;
    std::array<juce::dsp::IIR::Filter<float>, kMaxModes> filtersR;

    // Mode gains (affected by brightness and per-mode gain)
    std::array<float, kMaxModes> modeGains;

    // Effective frequencies for L/R (for stereo width)
    std::array<float, kMaxModes> modeFreqL;
    std::array<float, kMaxModes> modeFreqR;

    bool needsUpdate = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Resonator)
};
