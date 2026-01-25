#pragma once

#include <JuceHeader.h>
#include "ResonatorVoice.h"

class ResonatorSynth : public juce::Synthesiser
{
public:
    static constexpr int kDefaultNumVoices = 8;

    ResonatorSynth();
    ~ResonatorSynth() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);

    // Global parameter setters (applies to all voices and audio input resonator)
    void setDecayTime(float decaySeconds);
    void setBrightness(float brightness);
    void setNumModes(int numModes);

    // Excitation source (applies to all voices)
    void setExcitationSource(ImpulseGenerator::ExcitationSource source);
    ImpulseGenerator::ExcitationSource getExcitationSource() const;

    // Impulse filter controls (applies to all voices and audio input)
    void setFilterEnabled(bool enabled);
    void setFilterCutoff(float frequencyHz);
    void setFilterResonance(float resonance);
    void setFilterType(ImpulseGenerator::FilterType type);
    void setVelocityCurve(float curve);

    // Per-mode resonator controls (applies to all voices and audio input resonator)
    void setModeFrequencyRatio(int mode, float ratio);
    void setModeDecayMultiplier(int mode, float multiplier);
    void setModeGainDb(int mode, float gainDb);
    void setModeDetuneCents(int mode, float cents);
    void setModeStereoWidth(int mode, float width);
    void setModeEnabled(int mode, bool enabled);

    // Master resonator gain
    void setMasterGainDb(float gainDb);

    // Audio input fundamental frequency (used when in audio input mode without MIDI)
    void setAudioInputFundamental(float frequencyHz);
    float getAudioInputFundamental() const { return audioInputFundamental; }

    // Audio input processing (for audio excitation mode)
    void processWithAudioInput(juce::AudioBuffer<float>& outputBuffer,
                               const juce::AudioBuffer<float>& audioInput,
                               juce::MidiBuffer& midiMessages,
                               int numSamples,
                               float inputGain = 1.0f);

    // Panic - silence all voices
    void panic();

    // Get resonator parameters from first voice (for UI display)
    const Resonator* getFirstVoiceResonator() const;

private:
    int numVoices = kDefaultNumVoices;
    ImpulseGenerator::ExcitationSource currentExcitationSource = ImpulseGenerator::ExcitationSource::InternalNoise;

    // Dedicated resonator for audio input mode (always active, no MIDI needed)
    Resonator audioInputResonator;
    ImpulseGenerator audioInputImpulseGen;
    juce::AudioBuffer<float> audioInputImpulseBuffer;
    juce::AudioBuffer<float> audioInputResonatorBuffer;
    float audioInputFundamental = 220.0f;
    float currentDecayTime = 4.0f;
    float currentBrightness = 0.6f;
    float currentMasterGainDb = 0.0f;
    int currentNumModes = 6;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResonatorSynth)
};
