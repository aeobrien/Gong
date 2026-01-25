#pragma once

#include <JuceHeader.h>
#include "ImpulseGenerator.h"
#include "Resonator.h"

// Sound description for the ResonatorVoice
class ResonatorSound : public juce::SynthesiserSound
{
public:
    ResonatorSound() = default;
    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};

// Voice that combines impulse generator and resonator
class ResonatorVoice : public juce::SynthesiserVoice
{
public:
    ResonatorVoice();
    ~ResonatorVoice() override = default;

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void prepareToPlay(double sampleRate, int samplesPerBlock);

    // Global parameter access
    void setDecayTime(float decaySeconds);
    void setBrightness(float brightness);
    void setNumModes(int numModes);

    // Excitation source
    void setExcitationSource(ImpulseGenerator::ExcitationSource source);
    ImpulseGenerator::ExcitationSource getExcitationSource() const;

    // Impulse filter controls
    void setFilterEnabled(bool enabled);
    void setFilterCutoff(float frequencyHz);
    void setFilterResonance(float resonance);
    void setFilterType(ImpulseGenerator::FilterType type);
    void setVelocityCurve(float curve);

    // Per-mode resonator controls
    void setModeFrequencyRatio(int mode, float ratio);
    void setModeDecayMultiplier(int mode, float multiplier);
    void setModeGainDb(int mode, float gainDb);
    void setModeDetuneCents(int mode, float cents);
    void setModeStereoWidth(int mode, float width);
    void setModeEnabled(int mode, bool enabled);

    // Audio input processing (for audio excitation mode)
    void processWithAudioInput(juce::AudioBuffer<float>& outputBuffer,
                                const juce::AudioBuffer<float>& audioInput,
                                int startSample, int numSamples,
                                float inputGain = 1.0f);

    // Check if voice is ready for audio input excitation
    bool isReadyForAudioInput() const { return currentNote >= 0; }

    // Access to resonator for reading/writing parameters
    const Resonator& getResonator() const { return resonator; }
    Resonator& getResonatorMutable() { return resonator; }

private:
    ImpulseGenerator impulseGenerator;
    Resonator resonator;

    juce::AudioBuffer<float> impulseBuffer;
    juce::AudioBuffer<float> resonatorBuffer;

    int currentNote = -1;
    bool noteIsOn = false;

    // Time tracking for natural decay
    int samplesSinceNoteOn = 0;
    int decaySamples = 0; // How long voice stays active after note-off

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResonatorVoice)
};
