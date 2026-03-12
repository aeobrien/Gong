#pragma once

#include <JuceHeader.h>
#include "EnergyAccumulator.h"
#include "ResonatorBank.h"
#include "ImpulseGenerator.h"

/**
 * Energy-accumulation-based gong synthesizer.
 * Replaces the JUCE Synthesiser-based ResonatorSynth with a simpler architecture:
 * - No polyphony management (single continuous resonator bank)
 * - Energy-based modulation instead of envelope followers
 * - Strike detection from audio input
 */
class GongSynthesizer
{
public:
    GongSynthesizer();
    ~GongSynthesizer() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    // Main processing
    // audioInput: incoming audio for excitation and strike detection
    // outputBuffer: where to write processed audio
    // midiMessages: MIDI input for triggering strikes
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

    // Strike detection settings
    void setStrikeThreshold(float threshold);  // Amplitude threshold for audio strike detection
    float getStrikeThreshold() const { return strikeThreshold; }

    void setStrikeHoldoffMs(float ms);  // Minimum time between strikes
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

    // Core components
    EnergyAccumulator energyAccumulator;
    ResonatorBank resonatorBank;
    ImpulseGenerator impulseGenerator;

    // Strike detection
    float strikeThreshold = 0.1f;
    float strikeHoldoffMs = 50.0f;
    int strikeHoldoffSamples = 2400;
    int samplesSinceLastStrike = 0;
    float previousPeak = 0.0f;

    // Input settings
    float inputGain = 1.0f;
    bool midiEnabled = true;

    // Processing buffers
    juce::AudioBuffer<float> impulseBuffer;
    juce::AudioBuffer<float> resonatorBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GongSynthesizer)
};
