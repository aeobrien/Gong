#pragma once

#include <JuceHeader.h>
#include "EnergyAccumulator.h"
#include "ResonatorBank.h"
#include "ImpulseGenerator.h"
#include "SyntheticImpulseGenerator.h"
#include "MidiControllerMock.h"
#include "CombinationToneBank.h"
#include "CrashNoiseGenerator.h"

/**
 * Energy-accumulation-based gong synthesizer.
 * Supports two excitation modes:
 * - AudioInput: existing mic input path with strike detection
 * - SyntheticImpulse: MIDI-controller-driven shaped noise bursts
 */
class GongSynthesizer
{
public:
    enum class ExcitationMode
    {
        AudioInput,         // Existing mic path
        SyntheticImpulse    // MIDI → StrikeDescriptor → shaped noise burst
    };

    GongSynthesizer();
    ~GongSynthesizer() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    // Main processing
    void process(juce::AudioBuffer<float>& outputBuffer,
                 const juce::AudioBuffer<float>& audioInput,
                 juce::MidiBuffer& midiMessages,
                 int numSamples);

    // Access to components for parameter control
    EnergyAccumulator& getEnergyAccumulator() { return energyAccumulator; }
    const EnergyAccumulator& getEnergyAccumulator() const { return energyAccumulator; }

    ResonatorBank& getResonatorBank() { return resonatorBank; }
    const ResonatorBank& getResonatorBank() const { return resonatorBank; }

    ImpulseGenerator& getImpulseGenerator() { return impulseGenerator; }
    const ImpulseGenerator& getImpulseGenerator() const { return impulseGenerator; }

    SyntheticImpulseGenerator& getSyntheticImpulseGenerator() { return syntheticImpulseGen; }
    MidiControllerMock& getMidiControllerMock() { return midiControllerMock; }

    CombinationToneBank& getCombinationToneBank() { return combinationToneBank; }
    CrashNoiseGenerator& getCrashNoiseGenerator() { return crashNoiseGenerator; }

    // Excitation mode
    void setExcitationMode(ExcitationMode mode);
    ExcitationMode getExcitationMode() const { return excitationMode; }

    // Strike detection settings
    void setStrikeThreshold(float threshold);
    float getStrikeThreshold() const { return strikeThreshold; }

    void setStrikeHoldoffMs(float ms);
    float getStrikeHoldoffMs() const { return strikeHoldoffMs; }

    // Audio input settings
    void setInputGain(float gain);
    float getInputGain() const { return inputGain; }

    // MIDI settings
    void setMidiEnabled(bool enabled);
    bool getMidiEnabled() const { return midiEnabled; }

    // Global parameters that affect all resonators
    void setGlobalDecayTime(float seconds);
    void setGlobalBrightness(float brightness);
    void setGlobalSpreadLevel(float level);
    void setGlobalSpreadPanWidth(float width);

    // Reset all processing state
    void reset();

    // Panic - immediately stop all sound
    void panic();

private:
    void processStrikeDetection(const juce::AudioBuffer<float>& audioInput, int numSamples);
    void processMidiMessages(juce::MidiBuffer& midiMessages);
    void injectStrike(float velocity, int resonatorIndex = -1);

    double sampleRate = 48000.0;
    int blockSize = 256;

    // Excitation mode
    ExcitationMode excitationMode = ExcitationMode::AudioInput;

    // Core components
    EnergyAccumulator energyAccumulator;
    ResonatorBank resonatorBank;
    ImpulseGenerator impulseGenerator;
    SyntheticImpulseGenerator syntheticImpulseGen;
    MidiControllerMock midiControllerMock;
    CombinationToneBank combinationToneBank;
    CrashNoiseGenerator crashNoiseGenerator;

    // Strike detection
    float strikeThreshold = 0.1f;
    float strikeHoldoffMs = 50.0f;
    int strikeHoldoffSamples = 2400;
    int samplesSinceLastStrike = 0;
    float previousPeak = 0.0f;

    // Input settings
    float inputGain = 1.0f;
    bool midiEnabled = true;

    // Processing buffers (pre-allocated to avoid heap allocs on audio thread)
    juce::AudioBuffer<float> impulseBuffer;
    juce::AudioBuffer<float> resonatorBuffer;
    juce::AudioBuffer<float> stereoInputBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GongSynthesizer)
};
