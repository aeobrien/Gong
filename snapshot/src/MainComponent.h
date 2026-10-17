#pragma once

#include <JuceHeader.h>
#include "ConvolutionEngine.h"
#include "GongSynthesizer.h"
#include "ExciterProcessor.h"
#include "MultibandCompressor.h"
#include "PresetManager.h"
#include "PerformanceServer.h"
#include "DiagnosticState.h"
#include "TestSignalGenerator.h"
#include "ModulationBus.h"
#include "DiagnosticWindow.h"
#include "MacroParameters.h"
#include "ModalTemplate.h"
#include "StrikeDescriptor.h"
#include "ui/GongLookAndFeel.h"
#include "ui/ModulePanel.h"
#include "ui/RotaryKnob.h"
#include "ui/MeterComponent.h"
#include "ui/EnergyRingComponent.h"
#include "ui/ResonatorGridComponent.h"
#include "ui/PatchCableOverlay.h"
#include "ui/ModuleDescriptions.h"
#include "ui/ConvolutionContentComponent.h"

class MainComponent : public juce::AudioAppComponent,
                      private juce::MidiInputCallback,
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
    // MIDI
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    void setMidiInput(int index);
    void updateMidiDeviceList();

    // Audio processing
    void applyModulationOffsets();

    // UI helpers
    void buildModulePanels();
    void buildParamBindings();
    void buildPatchCables();
    void loadIRFile();
    void loadIRBFile();
    void updateUIFromState();
    void updatePresetComboBox();
    void pushMeterData();
    void pushEnergyData();
    void pushEffectiveValues();

    // Custom look and feel
    GongLookAndFeel gongLnF;

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
    MacroParameters macroParameters;

    juce::AudioBuffer<float> synthBuffer;
    juce::AudioBuffer<float> audioInputBuffer;

    // State
    int lastPlayedNote = 60;
    float masterGain = 0.8f;
    std::atomic<bool> nlDynamicsEnabled { true };
    std::atomic<bool> macrosEnabled { true };

    // MIDI
    juce::MidiMessageCollector midiCollector;
    juce::Array<juce::MidiDeviceInfo> midiDevices;
    int lastMidiInputIndex = 0;

    // === Toolbar ===
    juce::ComboBox midiInputList;
    juce::Label midiInputListLabel { {}, "MIDI:" };
    juce::ComboBox presetComboBox;
    juce::Label presetLabel { {}, "Preset:" };
    juce::TextButton presetSaveButton { "Save" };
    juce::TextButton presetSaveAsButton { "Save As..." };
    RotaryKnob volumeKnob { "Volume", {}, 0.0, 1.0, 0.01 };
    juce::TextButton panicButton { "PANIC" };
    juce::TextButton diagnosticsButton { "Diag" };

    // === Macro Sidebar ===
    RotaryKnob macroKnobs[4] {
        { "Size",      {}, 0.0, 1.0, 0.01 },
        { "Material",  {}, 0.0, 1.0, 0.01 },
        { "Intensity", {}, 0.0, 1.0, 0.01 },
        { "Space",     {}, 0.0, 1.0, 0.01 }
    };

    // === Module Panels ===
    ModulePanel inputPanel;
    ModulePanel energyPanel;
    ModulePanel resonatorPanel;
    ModulePanel comboTonePanel;
    ModulePanel crashPanel;
    ModulePanel convolutionPanel;
    ModulePanel exciterPanel;
    ModulePanel compressorPanel;

    // === Custom content for modules ===
    EnergyRingComponent energyRing;
    ResonatorGridComponent resonatorGrid;
    ConvolutionContentComponent convolutionContent;

    // Input extras
    juce::ToggleButton audioInputModeButton { "Audio In" };
    juce::ToggleButton syntheticModeButton { "Synthetic" };
    juce::TextButton auditionButton { "Audition" };

    // Synthetic impulse shaping
    StrikeDescriptor auditionDescriptor;

    // Modal template
    juce::ComboBox modalTemplateCombo;
    juce::Label modalTemplateLabel { {}, "Template:" };

    // Output meter (bottom bar instead of panel)
    MeterComponent outputMeter;

    // Patch cable overlay
    PatchCableOverlay patchCables;

    // === Parameter Binding System ===
    struct ParamBinding {
        std::function<void(float)> setter;
        std::function<float()> getter;
        std::function<float()> effectiveGetter;
    };
    std::unordered_map<juce::String, ParamBinding> paramBindings;

    // Tooltip
    juce::TooltipWindow tooltipWindow { this, 300 };

    // Directories
    juce::File sourceDirectory;
    juce::File irDirectory;
    juce::File presetsDirectory;
    juce::File webDirectory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
