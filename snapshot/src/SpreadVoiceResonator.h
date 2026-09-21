#pragma once

#include <JuceHeader.h>

/**
 * Single resonator with 7 voices (1 center + 6 spread).
 * Energy modulates brightness, spread level, detune, pan width, and frequency bending.
 */
class SpreadVoiceResonator
{
public:
    static constexpr int kNumVoices = 7;
    static constexpr int kCenterVoice = 0;

    // Base panning positions for spread voices
    // Voice 0: Center (0.0)
    // Voice 1: Left 1 (-0.7), Voice 2: Right 1 (+0.7)
    // Voice 3: Left 2 (-0.4), Voice 4: Right 2 (+0.4)
    // Voice 5: Far Left (-1.0), Voice 6: Far Right (+1.0)
    static constexpr float kBasePanPositions[kNumVoices] = {
        0.0f, -0.7f, 0.7f, -0.4f, 0.4f, -1.0f, 1.0f
    };

    // Base detune offsets (cents) for spread voices
    static constexpr float kBaseDetuneOffsets[kNumVoices] = {
        0.0f, -12.0f, 12.0f, -7.0f, 7.0f, -18.0f, 18.0f
    };

    enum class FrequencyMode
    {
        Free,   // Slider value in Hz (20-2000)
        Snap    // MIDI note number -> Hz
    };

    SpreadVoiceResonator();
    ~SpreadVoiceResonator() = default;

    void prepare(double sampleRate, int blockSize);
    void process(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output);
    void reset();

    // Frequency setting
    void setFrequencyMode(FrequencyMode mode);
    FrequencyMode getFrequencyMode() const { return frequencyMode; }

    void setFrequencyHz(float hz);  // Used in Free mode
    float getFrequencyHz() const { return frequencyHz; }

    void setMidiNote(int note);     // Used in Snap mode
    int getMidiNote() const { return midiNote; }

    float getEffectiveFrequency() const;  // Returns actual Hz based on mode

    // Decay and Q
    void setDecayTime(float seconds);
    float getDecayTime() const { return decayTime; }

    // Base brightness (before energy modulation)
    void setBaseBrightness(float brightness);
    float getBaseBrightness() const { return baseBrightness; }

    // Spread voice parameters (base values before energy modulation)
    void setSpreadLevel(float level);      // 0.0 = off, 1.0 = full
    float getSpreadLevel() const { return spreadLevel; }

    void setSpreadDetune(float cents);     // Additional detune on top of base offsets
    float getSpreadDetune() const { return spreadDetune; }

    void setSpreadPanWidth(float width);   // 0.0 = collapsed, 1.0 = full
    float getSpreadPanWidth() const { return spreadPanWidth; }

    // Master gain
    void setGainDb(float db);
    float getGainDb() const { return gainDb; }

    // Energy input (call before processing each block)
    void setEnergy(float energy);          // 0.0 to 1.0
    float getEnergy() const { return currentEnergy; }

    // Energy modulation amounts (how much energy affects each parameter)
    void setBrightnessEnergyAmount(float amount);  // 0.0 to 1.0
    void setSpreadLevelEnergyAmount(float amount);
    void setSpreadDetuneEnergyAmount(float amount);
    void setPanWidthEnergyAmount(float amount);
    void setFrequencyBendEnergyAmount(float amount);

    float getBrightnessEnergyAmount() const { return brightnessEnergyAmount; }
    float getSpreadLevelEnergyAmount() const { return spreadLevelEnergyAmount; }
    float getSpreadDetuneEnergyAmount() const { return spreadDetuneEnergyAmount; }
    float getPanWidthEnergyAmount() const { return panWidthEnergyAmount; }
    float getFrequencyBendEnergyAmount() const { return frequencyBendEnergyAmount; }

    // Step 6: Nonlinear pitch glide
    void setPitchGlideDirection(float dir);   // +1.0 = hardening, -1.0 = softening
    float getPitchGlideDirection() const { return glideDirection; }
    void setPitchGlideSensitivity(float cents);
    float getPitchGlideSensitivity() const { return glideSensitivity; }
    void setPitchGlideSmoothing(float coeff);
    float getPitchGlideSmoothing() const { return glideSmoothing; }

    // Enable/disable
    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() const { return isEnabled; }

private:
    void updateFilters();
    void updateVoiceParameters();

    double sampleRate = 48000.0;
    int blockSize = 256;

    // Frequency
    FrequencyMode frequencyMode = FrequencyMode::Free;
    float frequencyHz = 220.0f;
    int midiNote = 57;  // A3

    // Decay
    float decayTime = 3.0f;

    // Brightness
    float baseBrightness = 0.7f;

    // Spread parameters
    float spreadLevel = 0.5f;
    float spreadDetune = 0.0f;
    float spreadPanWidth = 1.0f;

    // Master gain
    float gainDb = 0.0f;

    // Current energy level
    float currentEnergy = 0.0f;
    juce::SmoothedValue<float> smoothedEnergy;

    // Energy modulation amounts
    float brightnessEnergyAmount = 0.5f;   // TOP PRIORITY
    float spreadLevelEnergyAmount = 0.3f;
    float spreadDetuneEnergyAmount = 0.2f;
    float panWidthEnergyAmount = 0.3f;
    float frequencyBendEnergyAmount = 0.0f;  // Subtle

    // Step 6: Nonlinear pitch glide
    float glideDirection = 1.0f;      // +1 = hardening (pitch up), -1 = softening (pitch down)
    float glideSensitivity = 80.0f;   // Cents per unit energy
    float glideSmoothing = 0.05f;     // One-pole smoothing coefficient
    float currentBend = 0.0f;         // Current smoothed bend in cents

    // Calculated per-voice values
    std::array<float, kNumVoices> voiceGains;
    std::array<float, kNumVoices> voicePans;
    std::array<float, kNumVoices> voiceFreqs;

    // Bandpass filters for each voice (L and R)
    std::array<juce::dsp::IIR::Filter<float>, kNumVoices> filtersL;
    std::array<juce::dsp::IIR::Filter<float>, kNumVoices> filtersR;

    bool isEnabled = true;
    bool needsUpdate = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpreadVoiceResonator)
};
