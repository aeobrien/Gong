#pragma once

#include <JuceHeader.h>
#include "ConvolutionEngine.h"
#include "ResonatorSynth.h"
#include "EnvelopeFollower.h"
#include "IRWaveformComponent.h"

class MainComponent : public juce::AudioAppComponent,
                      private juce::MidiInputCallback,
                      private juce::Slider::Listener,
                      private juce::Button::Listener,
                      private juce::ComboBox::Listener,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    // Callbacks
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;

    void setMidiInput(int index);
    void updateMidiDeviceList();
    void loadIRFile();
    void updateEnvelopeFollowerTarget();

    double currentSampleRate = 48000.0;
    int currentBlockSize = 256;

    // Core audio
    ResonatorSynth synth;
    ConvolutionEngine convolutionEngine;
    EnvelopeFollower envelopeFollower;

    juce::AudioBuffer<float> synthBuffer;
    juce::AudioBuffer<float> audioInputBuffer;

    // Current note for display
    int lastPlayedNote = 60;
    float masterGain = 0.8f;

    // MIDI
    juce::MidiMessageCollector midiCollector;
    juce::ComboBox midiInputList;
    juce::Label midiInputListLabel;
    int lastMidiInputIndex = 0;
    juce::Array<juce::MidiDeviceInfo> midiDevices;

    // Excitation Section
    juce::ToggleButton midiImpulseButton { "MIDI Impulse" };
    juce::ToggleButton audioInputButton { "Audio Input" };

    // Audio input frequency control (visible when audio input mode is selected)
    juce::Label audioInputFreqLabel { {}, "Frequency" };
    juce::Slider audioInputFreqSlider;

    juce::Label filterCutoffLabel { {}, "Cutoff" };
    juce::Slider filterCutoffSlider;
    juce::Label filterResoLabel { {}, "Reso" };
    juce::Slider filterResoSlider;
    juce::ComboBox filterTypeCombo;
    juce::Label filterTypeLabel { {}, "Type" };
    juce::Label velocityCurveLabel { {}, "Vel Curve" };
    juce::Slider velocityCurveSlider;

    // Envelope Follower Section
    juce::ToggleButton envFollowerEnableButton { "Enable" };
    juce::Label envAttackLabel { {}, "Attack" };
    juce::Slider envAttackSlider;
    juce::Label envReleaseLabel { {}, "Release" };
    juce::Slider envReleaseSlider;
    juce::ComboBox envTargetCombo;
    juce::Label envTargetLabel { {}, "Target" };
    juce::Slider envAmountSlider;
    juce::Label envAmountLabel { {}, "Amount" };
    float envFollowerAmount = 0.5f;
    int envFollowerTarget = 0; // 0=Velocity, 1=Filter Cutoff, 2=Decay, 3=Brightness, 4=Reverb

    // Resonator Global
    juce::Slider decaySlider;
    juce::Label decayLabel { {}, "Decay" };
    juce::Slider brightnessSlider;
    juce::Label brightnessLabel { {}, "Brightness" };
    juce::Slider resonatorGainSlider;
    juce::Label resonatorGainLabel { {}, "Res Gain" };

    // Per-Mode Controls
    static constexpr int kNumModes = 8;

    // Column header labels for mode controls
    juce::Label modeHeaderLabel { {}, "Mode" };
    juce::Label freqHeaderLabel { {}, "Freq" };
    juce::Label decayHeaderLabel { {}, "Decay" };
    juce::Label gainHeaderLabel { {}, "Gain" };
    juce::Label detuneHeaderLabel { {}, "Detune" };
    juce::Label widthHeaderLabel { {}, "Width" };
    juce::Label enableHeaderLabel { {}, "On" };
    juce::Label ratioHeaderLabel { {}, "Ratio" };

    struct ModeControls
    {
        juce::Label nameLabel;
        juce::Slider freqSlider;
        juce::Slider decaySlider;
        juce::Slider gainSlider;
        juce::Slider detuneSlider;
        juce::Slider widthSlider;
        juce::ToggleButton enableButton;
        juce::Label ratioLabel;
    };
    std::array<ModeControls, kNumModes> modeControls;

    // Convolution Section
    IRWaveformComponent irWaveform;
    juce::TextButton loadIRButton { "Load IR..." };
    juce::Label irFileLabel { {}, "No IR loaded" };
    juce::Slider reverbMixSlider;
    juce::Label reverbMixLabel { {}, "Mix" };
    juce::Slider convGainSlider;
    juce::Label convGainLabel { {}, "Conv Gain" };

    // Bottom Controls
    juce::Slider volumeSlider;
    juce::Label volumeLabel { {}, "Volume" };
    juce::TextButton panicButton { "PANIC" };

    // Input level meter
    float currentInputLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
