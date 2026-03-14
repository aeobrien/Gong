#pragma once

#include <JuceHeader.h>
#include "ConvolutionEngine.h"
#include "GongSynthesizer.h"
#include "ExciterProcessor.h"
#include "MultibandCompressor.h"
#include "IRWaveformComponent.h"
#include "PresetManager.h"
#include "PerformanceServer.h"
#include "DiagnosticState.h"
#include "TestSignalGenerator.h"
#include "ModulationBus.h"
#include "DiagnosticWindow.h"
#include "MacroParameters.h"
#include "ModalTemplate.h"

class MainComponent : public juce::AudioAppComponent,
                      private juce::MidiInputCallback,
                      private juce::Slider::Listener,
                      private juce::Button::Listener,
                      private juce::ComboBox::Listener,
                      private juce::Timer,
                      public PerformanceServer::Listener
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

    // PerformanceServer::Listener
    juce::StringArray getPresetNames() override;
    bool activatePreset(const juce::String& name) override;
    juce::StringArray getIRNames() override;
    bool activateIR(const juce::String& name) override;
    juce::String getCurrentPresetName() override;
    juce::String getCurrentIRName() override;

private:
    // Callbacks
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;

    void setMidiInput(int index);
    void updateMidiDeviceList();
    void loadIRFile();
    void setupTooltips();
    void updateResonatorFrequencyDisplay(int index);
    void updateUIFromState();
    void updatePresetComboBox();

    double currentSampleRate = 48000.0;
    int currentBlockSize = 256;

    // Core audio processing
    GongSynthesizer gongSynth;
    ConvolutionEngine convolutionEngine;
    ExciterProcessor exciterProcessor;
    MultibandCompressor multibandCompressor;
    PresetManager presetManager;
    std::unique_ptr<PerformanceServer> performanceServer;

    // Diagnostics & modulation
    DiagnosticState diagnosticState;
    TestSignalGenerator testSignalGenerator;
    ModulationBus modulationBus;
    std::unique_ptr<DiagnosticWindow> diagnosticWindow;
    juce::AudioBuffer<float> testBuffer;

    void applyModulationOffsets();

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

    // Excitation mode toggle
    juce::ToggleButton audioInputModeButton { "Audio In" };
    juce::ToggleButton syntheticModeButton { "Synthetic" };

    // Input Section
    juce::Label inputGainLabel { {}, "Input Gain" };
    juce::Slider inputGainSlider;
    juce::Label strikeThreshLabel { {}, "Strike Thresh" };
    juce::Slider strikeThreshSlider;
    juce::Label strikeHoldoffLabel { {}, "Holdoff" };
    juce::Slider strikeHoldoffSlider;

    // Energy Section
    juce::Label energyDecayLabel { {}, "Decay" };
    juce::Slider energyDecaySlider;
    juce::Label energyInjectionLabel { {}, "Injection" };
    juce::Slider energyInjectionSlider;
    juce::Label energyPowerLabel { {}, "Power" };
    juce::Slider energyPowerSlider;

    // Global Resonator Controls
    juce::Slider globalDecaySlider;
    juce::Label globalDecayLabel { {}, "Decay" };
    juce::Slider globalBrightnessSlider;
    juce::Label globalBrightnessLabel { {}, "Brightness" };
    juce::Slider globalSpreadLevelSlider;
    juce::Label globalSpreadLevelLabel { {}, "Spread Lvl" };
    juce::Slider globalSpreadPanWidthSlider;
    juce::Label globalSpreadPanWidthLabel { {}, "Pan Width" };

    // Per-Resonator Controls (4 resonators)
    static constexpr int kNumResonators = 4;

    // Column headers for resonator grid
    juce::Label resHeaderOn { {}, "On" };
    juce::Label resHeaderMode { {}, "Mode" };
    juce::Label resHeaderFreq { {}, "Frequency" };
    juce::Label resHeaderGain { {}, "Gain" };
    juce::Label resHeaderBright { {}, "Bright" };
    juce::Label resHeaderSpread { {}, "Spread" };
    juce::Label resHeaderDetune { {}, "Detune" };
    juce::Label resHeaderPan { {}, "Pan" };

    struct ResonatorControls
    {
        juce::Label nameLabel;
        juce::ToggleButton enableButton;
        juce::ToggleButton freeModeButton { "Free" };
        juce::ToggleButton snapModeButton { "Snap" };
        juce::Slider freqSlider;
        juce::ComboBox noteCombo;
        juce::Slider gainSlider;
        juce::Label freqValueLabel;
        juce::Slider brightnessModSlider;
        juce::Slider spreadModSlider;
        juce::Slider detuneModSlider;
        juce::Slider panModSlider;
    };
    std::array<ResonatorControls, kNumResonators> resonatorControls;

    // Convolution Section
    IRWaveformComponent irWaveform;
    juce::TextButton loadIRButton { "Load IR..." };
    juce::Label irFileLabel { {}, "No IR loaded" };
    juce::Slider reverbMixSlider;
    juce::Label reverbMixLabel { {}, "Mix" };
    juce::Slider convGainSlider;
    juce::Label convGainLabel { {}, "Gain" };

    // Exciter Section
    juce::ToggleButton exciterEnableButton { "Exciter" };
    juce::Label exciterFreqLabel { {}, "HP Freq" };
    juce::Slider exciterFreqSlider;
    juce::Label exciterDriveLabel { {}, "Drive" };
    juce::Slider exciterDriveSlider;
    juce::Label exciterMixLabel { {}, "Mix" };
    juce::Slider exciterMixSlider;

    // Multiband Compressor Section
    juce::ToggleButton compEnableButton { "Compressor" };
    juce::Label compThreshLabel { {}, "Thresh" };
    juce::Slider compThreshSlider;
    juce::Label compRatioLabel { {}, "Ratio" };
    juce::Slider compRatioSlider;
    juce::Label compAttackLabel { {}, "Atk" };
    juce::Slider compAttackSlider;
    juce::Label compReleaseLabel { {}, "Rel" };
    juce::Slider compReleaseSlider;

    // Preset Section
    juce::ComboBox presetComboBox;
    juce::Label presetLabel { {}, "Preset" };
    juce::TextButton presetSaveButton { "Save" };
    juce::TextButton presetSaveAsButton { "Save As..." };

    // Nonlinear Dynamics Controls (Phase 1-4)
    juce::Slider couplingRateSlider;
    juce::Label couplingRateLabel { {}, "Coupling" };
    juce::Slider bloomThreshSlider;
    juce::Label bloomThreshLabel { {}, "Bloom" };
    juce::Slider glideDirectionSlider;
    juce::Label glideDirectionLabel { {}, "Glide Dir" };
    juce::Slider glideSensitivitySlider;
    juce::Label glideSensitivityLabel { {}, "Glide Sens" };
    juce::Slider postConvLowSlider;
    juce::Label postConvLowLabel { {}, "Low EQ" };
    juce::Slider postConvMidSlider;
    juce::Label postConvMidLabel { {}, "Mid EQ" };
    juce::Slider postConvHighSlider;
    juce::Label postConvHighLabel { {}, "High EQ" };
    juce::ComboBox modalTemplateCombo;
    juce::Label modalTemplateLabel { {}, "Template" };
    juce::TextButton loadIRBButton { "Load IR B..." };
    juce::Label irBFileLabel { {}, "No IR B" };

    // Macro knobs (Step 15)
    MacroParameters macroParameters;
    juce::Slider macroSliders[4];
    juce::Label macroLabels[4];

    // Bottom Controls
    juce::Slider volumeSlider;
    juce::Label volumeLabel { {}, "Volume" };
    juce::TextButton panicButton { "PANIC" };
    juce::TextButton diagnosticsButton { "Diagnostics" };

    // Display values
    float currentInputLevel = 0.0f;
    float currentGlobalEnergy = 0.0f;

    // Tooltip component
    juce::TooltipWindow tooltipWindow { this, 300 };

    // Section enable toggles
    juce::ToggleButton convSectionToggle { "On" };
    juce::ToggleButton nlSectionToggle { "On" };
    juce::ToggleButton macroSectionToggle { "On" };

    // Section bounds (computed in resized, used in paint)
    juce::Rectangle<int> inputSectionBounds, energySectionBounds, resonatorSectionBounds;
    juce::Rectangle<int> convSectionBounds, nlSectionBounds, outputSectionBounds;
    juce::Rectangle<int> macroSectionBounds, presetSectionBounds;

    // Nonlinear dynamics / macro bypass flags
    std::atomic<bool> nlDynamicsEnabled { true };
    std::atomic<bool> macrosEnabled { true };

    // Directories
    juce::File sourceDirectory;
    juce::File irDirectory;
    juce::File presetsDirectory;
    juce::File webDirectory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
