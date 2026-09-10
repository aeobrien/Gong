#include "MainComponent.h"
#include <chrono>

//==============================================================================
// Construction / Destruction
//==============================================================================

MainComponent::MainComponent()
{
    setSize(1400, 900);
    setLookAndFeel(&gongLnF);

    // Resolve directories relative to source
    sourceDirectory = juce::File(__FILE__).getParentDirectory().getParentDirectory();
    irDirectory = sourceDirectory.getChildFile("IRs");
    presetsDirectory = sourceDirectory.getChildFile("presets");
    webDirectory = sourceDirectory.getChildFile("web");

    presetManager.setPresetsDirectory(presetsDirectory);
    presetManager.setIRDirectory(irDirectory);

    // === Toolbar: MIDI ===
    addAndMakeVisible(midiInputListLabel);
    addAndMakeVisible(midiInputList);
    midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs");
    midiInputList.onChange = [this] { setMidiInput(midiInputList.getSelectedItemIndex()); };
    updateMidiDeviceList();
    if (midiDevices.size() > 0)
        setMidiInput(0);

    // === Toolbar: Preset ===
    addAndMakeVisible(presetLabel);
    addAndMakeVisible(presetComboBox);
    presetComboBox.onChange = [this] {
        auto name = presetComboBox.getText();
        if (name.isNotEmpty())
        {
            if (presetManager.loadPreset(name, gongSynth, convolutionEngine,
                                          exciterProcessor, multibandCompressor, masterGain, &diagnosticState))
            {
                updateUIFromState();
                if (convolutionEngine.isLoaded())
                    convolutionContent.getIRWaveform().setIR(convolutionEngine.getIRBuffer(), convolutionEngine.getIRSampleRate());
            }
        }
    };
    updatePresetComboBox();

    addAndMakeVisible(presetSaveButton);
    presetSaveButton.onClick = [this] {
        auto name = presetManager.getCurrentPresetName();
        if (name.isEmpty()) name = "Untitled";
        presetManager.savePreset(name, gongSynth, convolutionEngine, exciterProcessor, multibandCompressor, masterGain, &diagnosticState);
        updatePresetComboBox();
    };

    addAndMakeVisible(presetSaveAsButton);
    presetSaveAsButton.onClick = [this] {
        auto* alertWindow = new juce::AlertWindow("Save Preset As", "Enter preset name:", juce::MessageBoxIconType::QuestionIcon);
        alertWindow->addTextEditor("name", presetManager.getCurrentPresetName(), "Name:");
        alertWindow->addButton("Save", 1);
        alertWindow->addButton("Cancel", 0);
        alertWindow->enterModalState(true, juce::ModalCallbackFunction::create([this, alertWindow](int result) {
            if (result == 1)
            {
                auto name = alertWindow->getTextEditorContents("name");
                if (name.isNotEmpty())
                {
                    presetManager.savePreset(name, gongSynth, convolutionEngine, exciterProcessor, multibandCompressor, masterGain, &diagnosticState);
                    updatePresetComboBox();
                }
            }
            delete alertWindow;
        }));
    };

    // === Toolbar: Macro knobs ===
    for (int i = 0; i < 4; ++i)
    {
        macroKnobs[i].getSlider().setValue(0.5, juce::dontSendNotification);
        macroKnobs[i].getSlider().onValueChange = [this, i] {
            macroParameters.setMacro(i, static_cast<float>(macroKnobs[i].getSlider().getValue()));
        };
        macroKnobs[i].setTooltipText(ModuleDescriptions::getMacroDescription(i));
        macroKnobs[i].setCenterValueDisplay(true);
        addAndMakeVisible(macroKnobs[i]);
    }

    // === Toolbar: Volume ===
    volumeKnob.getSlider().setValue(0.8, juce::dontSendNotification);
    volumeKnob.getSlider().onValueChange = [this] { masterGain = static_cast<float>(volumeKnob.getSlider().getValue()); };
    addAndMakeVisible(volumeKnob);

    addAndMakeVisible(panicButton);
    panicButton.onClick = [this] { gongSynth.panic(); };

    addAndMakeVisible(diagnosticsButton);
    diagnosticsButton.onClick = [this] {
        if (!diagnosticWindow || !diagnosticWindow->isVisible())
            diagnosticWindow = std::make_unique<DiagnosticWindow>(diagnosticState);
        else
            diagnosticWindow->setVisible(true);
    };

    // === Input extras ===
    audioInputModeButton.setRadioGroupId(999);
    audioInputModeButton.setToggleState(true, juce::dontSendNotification);
    audioInputModeButton.onClick = [this] { gongSynth.setExcitationMode(GongSynthesizer::ExcitationMode::AudioInput); };
    addAndMakeVisible(audioInputModeButton);

    syntheticModeButton.setRadioGroupId(999);
    syntheticModeButton.onClick = [this] { gongSynth.setExcitationMode(GongSynthesizer::ExcitationMode::SyntheticImpulse); };
    addAndMakeVisible(syntheticModeButton);

    auditionButton.onClick = [this] {
        // Switch to synthetic mode if not already
        gongSynth.setExcitationMode(GongSynthesizer::ExcitationMode::SyntheticImpulse);
        syntheticModeButton.setToggleState(true, juce::dontSendNotification);

        // Read current strike shaping values from the input panel knobs
        if (auto* s = inputPanel.getSlider("strikeAttack"))
            auditionDescriptor.attackSlope = static_cast<uint8_t>(s->getValue());
        if (auto* s = inputPanel.getSlider("strikeBright"))
            auditionDescriptor.spectralCentroid = static_cast<uint16_t>(s->getValue());
        if (auto* s = inputPanel.getSlider("strikeHF"))
            auditionDescriptor.hfEnergyRatio = static_cast<uint8_t>(s->getValue());
        if (auto* s = inputPanel.getSlider("strikeDecay"))
            auditionDescriptor.decayShape = static_cast<uint8_t>(s->getValue());

        gongSynth.getSyntheticImpulseGenerator().trigger(auditionDescriptor);

        // Inject energy into all bands
        float velocity = auditionDescriptor.velocityNorm();
        for (int band = 0; band < EnergyAccumulator::kNumBands; ++band)
            gongSynth.getEnergyAccumulator().injectEnergy(velocity, band);
    };
    addAndMakeVisible(auditionButton);

    // === Modal template ===
    addAndMakeVisible(modalTemplateLabel);
    addAndMakeVisible(modalTemplateCombo);
    modalTemplateCombo.addItem("Manual", 1);
    for (int i = 0; i < kNumTemplates; ++i)
        modalTemplateCombo.addItem(kAllTemplates[i]->name, i + 2);
    modalTemplateCombo.setSelectedId(1, juce::dontSendNotification);
    modalTemplateCombo.onChange = [this] {
        int id = modalTemplateCombo.getSelectedId();
        if (id > 1)
        {
            float fundamental = gongSynth.getResonatorBank().getResonator(0).getEffectiveFrequency();
            gongSynth.getResonatorBank().applyModalTemplate(id - 2, fundamental);
            resonatorGrid.updateFromState();
        }
    };

    // === Convolution content (IR waveform + load buttons) ===
    convolutionContent.getLoadIRButton().onClick = [this] { loadIRFile(); };
    convolutionContent.getLoadIRBButton().onClick = [this] { loadIRBFile(); };

    // === Build module panels ===
    buildModulePanels();
    buildParamBindings();

    // === Patch cables ===
    addAndMakeVisible(patchCables);
    buildPatchCables();

    // Custom content bindings
    energyPanel.setCustomContent(&energyRing);
    resonatorGrid.setResonatorBank(&gongSynth.getResonatorBank());
    resonatorPanel.setCustomContent(&resonatorGrid);
    convolutionPanel.setCustomContent(&convolutionContent);

    // Output meter in bottom bar
    outputMeter.setMeter(&diagnosticState.output);
    outputMeter.setColour(juce::Colour(0xff78909c));
    addAndMakeVisible(outputMeter);

    // Request audio
    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio)
        && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(
            juce::RuntimePermissions::recordAudio,
            [&](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        setAudioChannels(2, 2);
    }

    // Request larger buffer
    {
        auto setup = deviceManager.getAudioDeviceSetup();
        if (setup.bufferSize < 2048)
        {
            setup.bufferSize = 2048;
            deviceManager.setAudioDeviceSetup(setup, true);
        }
    }

    // Performance server
    performanceServer = std::make_unique<PerformanceServer>(*this, 8080);
    performanceServer->setWebRoot(webDirectory);
    performanceServer->startServer();

    startTimerHz(15);
}

MainComponent::~MainComponent()
{
    stopTimer();
    diagnosticWindow.reset();
    if (performanceServer)
        performanceServer->stopServer();
    shutdownAudio();
    for (auto& device : midiDevices)
        deviceManager.setMidiInputDeviceEnabled(device.identifier, false);
    setLookAndFeel(nullptr);
}

//==============================================================================
// Module Panel Construction
//==============================================================================

void MainComponent::buildModulePanels()
{
    // --- Input / Strike Detection ---
    {
        ModuleDescriptor desc;
        desc.id = "input";
        desc.info = &ModuleDescriptions::getInputModule();
        desc.knobs = {
            { "inputGain",     "Gain",      {},       0.0, 4.0, 0.01, 1.0 },
            { "threshold",     "Threshold", {},       0.01, 0.5, 0.01, 0.1 },
            { "holdoff",       "Holdoff",   " ms",    10.0, 200.0, 1.0, 50.0 },
            { "strikeAttack",  "Attack",    {},       0.0, 255.0, 1.0, 180.0 },
            { "strikeBright",  "Bright",    " Hz",    200.0, 15000.0, 10.0, 3000.0 },
            { "strikeHF",      "HF Mix",    {},       0.0, 255.0, 1.0, 128.0 },
            { "strikeDecay",   "Tail",      {},       0.0, 255.0, 1.0, 160.0 },
        };
        desc.hasInputMeter = true;
        desc.hasOutputMeter = false;
        inputPanel.configure(desc);
        addAndMakeVisible(inputPanel);

        // Initialize audition descriptor defaults
        auditionDescriptor.velocity = 100;
        auditionDescriptor.attackSlope = 180;
        auditionDescriptor.spectralCentroid = 3000;
        auditionDescriptor.hfEnergyRatio = 128;
        auditionDescriptor.decayShape = 160;
        auditionDescriptor.padId = 0;
    }

    // --- Energy Accumulator ---
    {
        ModuleDescriptor desc;
        desc.id = "energy";
        desc.info = &ModuleDescriptions::getEnergyModule();
        desc.knobs = {
            { "energyDecay", "Decay",     " ms",  500.0, 5000.0, 10.0, 2000.0 },
            { "injection",   "Injection", {},      0.1, 3.0, 0.01, 1.0 },
            { "power",       "Power",     {},      0.5, 3.0, 0.01, 1.5 },
            { "coupling",    "Coupling",  {},      0.0, 0.01, 0.0001, 0.001 },
            { "bloom",       "Bloom",     {},      0.1, 1.0, 0.01, 0.7 },
        };
        desc.hasOutputMeter = true;
        energyPanel.configure(desc);
        addAndMakeVisible(energyPanel);
    }

    // --- Resonator Bank ---
    {
        ModuleDescriptor desc;
        desc.id = "resonator";
        desc.info = &ModuleDescriptions::getResonatorModule();
        desc.knobs = {
            { "resDecay",    "Decay",      " s",   0.5, 15.0, 0.1, 4.0 },
            { "brightness",  "Brightness", {},      0.0, 1.0, 0.01, 0.7 },
            { "spreadLevel", "Spread",     {},      0.0, 1.0, 0.01, 0.5 },
            { "panWidth",    "Pan Width",  {},      0.0, 1.0, 0.01, 1.0 },
        };
        desc.hasInputMeter = true;
        desc.hasOutputMeter = true;
        resonatorPanel.configure(desc);
        addAndMakeVisible(resonatorPanel);
    }

    // --- Combination Tones ---
    {
        ModuleDescriptor desc;
        desc.id = "combo";
        desc.info = &ModuleDescriptions::getCombinationModule();
        desc.knobs = {
            { "comboMix",       "Mix",       {},  0.0, 1.0, 0.01, 0.15 },
            { "comboThreshold", "Threshold", {},  0.0, 1.0, 0.01, 0.3 },
        };
        comboTonePanel.configure(desc);
        addAndMakeVisible(comboTonePanel);
    }

    // --- Crash Noise ---
    {
        ModuleDescriptor desc;
        desc.id = "crash";
        desc.info = &ModuleDescriptions::getCrashModule();
        desc.knobs = {
            { "crashThreshold", "Threshold", {},  0.1, 1.0, 0.01, 0.85 },
            { "crashLevel",     "Level",     {},  0.0, 1.0, 0.01, 0.3 },
        };
        crashPanel.configure(desc);
        addAndMakeVisible(crashPanel);
    }

    // --- Convolution Reverb ---
    {
        ModuleDescriptor desc;
        desc.id = "convolution";
        desc.info = &ModuleDescriptions::getConvolutionModule();
        desc.knobs = {
            { "convMix",     "Mix",      {},      0.0, 1.0, 0.01, 0.4 },
            { "convGain",    "Gain",     " dB",   -24.0, 24.0, 0.1, 0.0 },
            { "postLowEQ",   "Low EQ",   " dB",   -12.0, 12.0, 0.1, 0.0 },
            { "postMidEQ",   "Mid EQ",   " dB",   -12.0, 12.0, 0.1, 0.0 },
            { "postHighEQ",  "High EQ",  " dB",   -12.0, 12.0, 0.1, 0.0 },
        };
        desc.hasInputMeter = true;
        desc.hasOutputMeter = true;
        convolutionPanel.configure(desc);
        addAndMakeVisible(convolutionPanel);
    }

    // --- Harmonic Exciter ---
    {
        ModuleDescriptor desc;
        desc.id = "exciter";
        desc.info = &ModuleDescriptions::getExciterModule();
        desc.knobs = {
            { "exciterFreq",  "HP Freq",  " Hz",   200.0, 2000.0, 1.0, 500.0 },
            { "exciterDrive", "Drive",    {},       1.0, 5.0, 0.1, 2.0 },
            { "exciterMix",   "Mix",      {},       0.0, 1.0, 0.01, 0.3 },
        };
        desc.hasInputMeter = true;
        desc.hasOutputMeter = true;
        exciterPanel.configure(desc);
        addAndMakeVisible(exciterPanel);
    }

    // --- Multiband Compressor ---
    {
        ModuleDescriptor desc;
        desc.id = "compressor";
        desc.info = &ModuleDescriptions::getCompressorModule();
        desc.knobs = {
            { "compThresh",   "Threshold", " dB",  -40.0, 0.0, 0.1, -12.0 },
            { "compRatio",    "Ratio",     {},      1.0, 20.0, 0.1, 4.0 },
            { "compAttack",   "Attack",    " ms",   1.0, 100.0, 1.0, 10.0 },
            { "compRelease",  "Release",   " ms",   10.0, 500.0, 1.0, 100.0 },
        };
        desc.hasInputMeter = true;
        desc.hasOutputMeter = true;
        compressorPanel.configure(desc);
        addAndMakeVisible(compressorPanel);
    }

    // Output panel removed - output meter is in the bottom status bar
}

void MainComponent::buildParamBindings()
{
    auto& ea = gongSynth.getEnergyAccumulator();
    auto& rb = gongSynth.getResonatorBank();
    auto& combo = gongSynth.getCombinationToneBank();
    auto& crash = gongSynth.getCrashNoiseGenerator();

    // Helper to bind a knob's onValueChange
    auto bind = [this](const juce::String& panelId, const juce::String& knobId,
                       std::function<void(float)> setter,
                       std::function<float()> getter,
                       std::function<float()> effectiveGetter = nullptr) {
        paramBindings[knobId] = { setter, getter, effectiveGetter };

        // Find the right panel
        ModulePanel* panel = nullptr;
        if (panelId == "input") panel = &inputPanel;
        else if (panelId == "energy") panel = &energyPanel;
        else if (panelId == "resonator") panel = &resonatorPanel;
        else if (panelId == "combo") panel = &comboTonePanel;
        else if (panelId == "crash") panel = &crashPanel;
        else if (panelId == "convolution") panel = &convolutionPanel;
        else if (panelId == "exciter") panel = &exciterPanel;
        else if (panelId == "compressor") panel = &compressorPanel;

        if (panel != nullptr)
        {
            auto* slider = panel->getSlider(knobId);
            if (slider != nullptr)
            {
                slider->onValueChange = [setter, slider] {
                    setter(static_cast<float>(slider->getValue()));
                };
            }
        }
    };

    // Input
    bind("input", "inputGain",
         [this](float v) { gongSynth.setInputGain(v); },
         [this]() { return gongSynth.getInputGain(); });
    bind("input", "threshold",
         [this](float v) { gongSynth.setStrikeThreshold(v); },
         [this]() { return gongSynth.getStrikeThreshold(); });
    bind("input", "holdoff",
         [this](float v) { gongSynth.setStrikeHoldoffMs(v); },
         [this]() { return gongSynth.getStrikeHoldoffMs(); });

    // Synthetic impulse shaping (these update auditionDescriptor for the audition button,
    // and also affect MIDI-triggered strikes via the MidiControllerMock defaults)
    bind("input", "strikeAttack",
         [this](float v) { auditionDescriptor.attackSlope = static_cast<uint8_t>(v); },
         [this]() { return static_cast<float>(auditionDescriptor.attackSlope); });
    bind("input", "strikeBright",
         [this](float v) { auditionDescriptor.spectralCentroid = static_cast<uint16_t>(v); },
         [this]() { return static_cast<float>(auditionDescriptor.spectralCentroid); });
    bind("input", "strikeHF",
         [this](float v) { auditionDescriptor.hfEnergyRatio = static_cast<uint8_t>(v); },
         [this]() { return static_cast<float>(auditionDescriptor.hfEnergyRatio); });
    bind("input", "strikeDecay",
         [this](float v) { auditionDescriptor.decayShape = static_cast<uint8_t>(v); },
         [this]() { return static_cast<float>(auditionDescriptor.decayShape); });

    // Energy
    bind("energy", "energyDecay",
         [&ea](float v) { ea.setGlobalDecayMs(v); },
         [&ea]() { return ea.getGlobalDecayMs(); });
    bind("energy", "injection",
         [&ea](float v) { ea.setInjectionGain(v); },
         [&ea]() { return ea.getInjectionGain(); });
    bind("energy", "power",
         [&ea](float v) { ea.setInjectionPower(v); },
         [&ea]() { return ea.getInjectionPower(); });
    bind("energy", "coupling",
         [&ea](float v) { ea.setCouplingRate(v); },
         [&ea]() { return ea.getCouplingRate(); });
    bind("energy", "bloom",
         [&ea](float v) { ea.setBloomThreshold(v); },
         [&ea]() { return ea.getBloomThreshold(); });

    // Resonator globals
    bind("resonator", "resDecay",
         [this](float v) { gongSynth.setGlobalDecayTime(v); },
         [&rb]() { return rb.getResonator(0).getDecayTime(); });
    bind("resonator", "brightness",
         [this](float v) { gongSynth.setGlobalBrightness(v); },
         [&rb]() { return rb.getResonator(0).getBaseBrightness(); });
    bind("resonator", "spreadLevel",
         [this](float v) { gongSynth.setGlobalSpreadLevel(v); },
         [&rb]() { return rb.getResonator(0).getSpreadLevel(); });
    bind("resonator", "panWidth",
         [this](float v) { gongSynth.setGlobalSpreadPanWidth(v); },
         [&rb]() { return rb.getResonator(0).getSpreadPanWidth(); });

    // Combination tones
    bind("combo", "comboMix",
         [&combo](float v) { combo.setMixLevel(v); },
         [&combo]() { return combo.getMixLevel(); });
    bind("combo", "comboThreshold",
         [&combo](float v) { combo.setActivationThreshold(v); },
         [&combo]() { return combo.getActivationThreshold(); });

    // Crash
    bind("crash", "crashThreshold",
         [&crash](float v) { crash.setThreshold(v); },
         [&crash]() { return crash.getThreshold(); });
    bind("crash", "crashLevel",
         [&crash](float v) { crash.setMixLevel(v); },
         [&crash]() { return crash.getMixLevel(); });

    // Convolution
    bind("convolution", "convMix",
         [this](float v) { convolutionEngine.setWetDryMix(v); },
         [this]() { return convolutionEngine.getWetDryMix(); });
    bind("convolution", "convGain",
         [this](float v) { convolutionEngine.setOutputGainDb(v); },
         [this]() { return convolutionEngine.getOutputGainDb(); });
    bind("convolution", "postLowEQ",
         [this](float v) { convolutionEngine.setPostConvLowGainDb(v); },
         [this]() { return convolutionEngine.getPostConvLowGainDb(); });
    bind("convolution", "postMidEQ",
         [this](float v) { convolutionEngine.setPostConvMidGainDb(v); },
         [this]() { return convolutionEngine.getPostConvMidGainDb(); });
    bind("convolution", "postHighEQ",
         [this](float v) { convolutionEngine.setPostConvHighGainDb(v); },
         [this]() { return convolutionEngine.getPostConvHighGainDb(); });

    // Exciter
    bind("exciter", "exciterFreq",
         [this](float v) { exciterProcessor.setHighpassFrequency(v); },
         [this]() { return exciterProcessor.getHighpassFrequency(); });
    bind("exciter", "exciterDrive",
         [this](float v) { exciterProcessor.setSaturationDrive(v); },
         [this]() { return exciterProcessor.getSaturationDrive(); });
    bind("exciter", "exciterMix",
         [this](float v) { exciterProcessor.setDryWetMix(v); },
         [this]() { return exciterProcessor.getDryWetMix(); });

    // Compressor
    bind("compressor", "compThresh",
         [this](float v) { multibandCompressor.setAllThresholds(v); },
         [this]() { return multibandCompressor.getBandSettings(0).threshold; });
    bind("compressor", "compRatio",
         [this](float v) { multibandCompressor.setAllRatios(v); },
         [this]() { return multibandCompressor.getBandSettings(0).ratio; });
    bind("compressor", "compAttack",
         [this](float v) { multibandCompressor.setAllAttacks(v); },
         [this]() { return multibandCompressor.getBandSettings(0).attackMs; });
    bind("compressor", "compRelease",
         [this](float v) { multibandCompressor.setAllReleases(v); },
         [this]() { return multibandCompressor.getBandSettings(0).releaseMs; });

    // Nonlinear: glide direction and sensitivity (bound to resonator knobs inline)
    // These are handled via the modal template combo and directly in applyModulationOffsets
}

void MainComponent::buildPatchCables()
{
    patchCables.clearCables();

    // Row 1: Input -> Energy -> Resonator -> Combo/Crash
    patchCables.addCable(&inputPanel, &energyPanel);        // cable 0
    patchCables.addCable(&energyPanel, &resonatorPanel);    // cable 1
    patchCables.addCable(&resonatorPanel, &comboTonePanel); // cable 2
    patchCables.addCable(&resonatorPanel, &crashPanel);     // cable 3

    // Cross-row: Combo+Crash -> Convolution (row 1 to row 2)
    patchCables.addCable(&comboTonePanel, &convolutionPanel); // cable 4
    patchCables.addCable(&crashPanel, &convolutionPanel);     // cable 5

    // Row 2: Convolution -> Exciter -> Compressor
    patchCables.addCable(&convolutionPanel, &exciterPanel);   // cable 6
    patchCables.addCable(&exciterPanel, &compressorPanel);    // cable 7
}

//==============================================================================
// UI State
//==============================================================================

void MainComponent::updateUIFromState()
{
    // Update all knobs from processor state via bindings
    for (auto& [id, binding] : paramBindings)
    {
        if (binding.getter)
        {
            float value = binding.getter();
            // Find which panel has this knob
            ModulePanel* panels[] = { &inputPanel, &energyPanel, &resonatorPanel, &comboTonePanel,
                                       &crashPanel, &convolutionPanel, &exciterPanel, &compressorPanel };
            for (auto* panel : panels)
            {
                auto* slider = panel->getSlider(id);
                if (slider != nullptr)
                {
                    slider->setValue(value, juce::dontSendNotification);
                    break;
                }
            }
        }
    }

    // Resonator grid
    resonatorGrid.updateFromState();

    // Volume
    volumeKnob.getSlider().setValue(masterGain, juce::dontSendNotification);

    // Macros
    for (int i = 0; i < 4; ++i)
        macroKnobs[i].getSlider().setValue(macroParameters.getMacro(i), juce::dontSendNotification);

    // IR labels
    if (convolutionEngine.isLoaded())
        convolutionContent.getIRFileLabel().setText(convolutionEngine.getIRFileName(), juce::dontSendNotification);

    // Excitation mode
    bool isAudio = gongSynth.getExcitationMode() == GongSynthesizer::ExcitationMode::AudioInput;
    audioInputModeButton.setToggleState(isAudio, juce::dontSendNotification);
    syntheticModeButton.setToggleState(!isAudio, juce::dontSendNotification);
}

void MainComponent::updatePresetComboBox()
{
    presetComboBox.clear(juce::dontSendNotification);
    auto names = presetManager.getPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetComboBox.addItem(names[i], i + 1);

    auto currentName = presetManager.getCurrentPresetName();
    if (currentName.isNotEmpty())
    {
        int idx = names.indexOf(currentName);
        if (idx >= 0)
            presetComboBox.setSelectedId(idx + 1, juce::dontSendNotification);
    }
}

void MainComponent::pushMeterData()
{
    inputPanel.getInputMeter().setMeter(&diagnosticState.input);
    inputPanel.getInputMeter().refresh();

    energyPanel.getOutputMeter().setMeter(&diagnosticState.afterSynth);
    energyPanel.getOutputMeter().refresh();

    resonatorPanel.getInputMeter().setMeter(&diagnosticState.afterSynth);
    resonatorPanel.getInputMeter().refresh();
    resonatorPanel.getOutputMeter().setMeter(&diagnosticState.afterSynth);
    resonatorPanel.getOutputMeter().refresh();

    convolutionPanel.getInputMeter().setMeter(&diagnosticState.afterSynth);
    convolutionPanel.getInputMeter().refresh();
    convolutionPanel.getOutputMeter().setMeter(&diagnosticState.afterConv);
    convolutionPanel.getOutputMeter().refresh();

    exciterPanel.getInputMeter().setMeter(&diagnosticState.afterConv);
    exciterPanel.getInputMeter().refresh();
    exciterPanel.getOutputMeter().setMeter(&diagnosticState.afterExciter);
    exciterPanel.getOutputMeter().refresh();

    compressorPanel.getInputMeter().setMeter(&diagnosticState.afterExciter);
    compressorPanel.getInputMeter().refresh();
    compressorPanel.getOutputMeter().setMeter(&diagnosticState.afterComp);
    compressorPanel.getOutputMeter().refresh();

    outputMeter.refresh();
}

void MainComponent::pushEnergyData()
{
    auto& ea = gongSynth.getEnergyAccumulator();
    energyRing.setGlobalEnergy(ea.getNormalizedGlobalEnergy());
    for (int i = 0; i < 4; ++i)
        energyRing.setBandEnergy(i, ea.getNormalizedBandEnergy(i));

    // Update patch cable signal levels
    auto rmsFromMeter = [](const StageMeter& m) {
        return (m.rmsL.load(std::memory_order_relaxed) + m.rmsR.load(std::memory_order_relaxed)) * 0.5f;
    };

    float inputLevel = rmsFromMeter(diagnosticState.input) * 5.0f;
    float synthLevel = rmsFromMeter(diagnosticState.afterSynth) * 5.0f;
    float convLevel = rmsFromMeter(diagnosticState.afterConv) * 5.0f;
    float exciterLevel = rmsFromMeter(diagnosticState.afterExciter) * 5.0f;
    float compLevel = rmsFromMeter(diagnosticState.afterComp) * 5.0f;
    patchCables.setSignalLevel(0, juce::jlimit(0.0f, 1.0f, inputLevel));   // input->energy
    patchCables.setSignalLevel(1, juce::jlimit(0.0f, 1.0f, synthLevel));  // energy->resonator
    patchCables.setSignalLevel(2, juce::jlimit(0.0f, 1.0f, synthLevel));  // resonator->combo
    patchCables.setSignalLevel(3, juce::jlimit(0.0f, 1.0f, synthLevel));  // resonator->crash
    patchCables.setSignalLevel(4, juce::jlimit(0.0f, 1.0f, synthLevel));  // combo->conv
    patchCables.setSignalLevel(5, juce::jlimit(0.0f, 1.0f, synthLevel));  // crash->conv
    patchCables.setSignalLevel(6, juce::jlimit(0.0f, 1.0f, convLevel));   // conv->exciter
    patchCables.setSignalLevel(7, juce::jlimit(0.0f, 1.0f, exciterLevel));// exciter->comp
}

void MainComponent::pushEffectiveValues()
{
    auto& ea = gongSynth.getEnergyAccumulator();
    float energy = ea.getNormalizedGlobalEnergy();
    auto& res0 = gongSynth.getResonatorBank().getResonator(0);

    // Helper: show effective value on a knob (orange arc = energy/macro modulated value)
    auto showEffective = [](ModulePanel& panel, const juce::String& id, float effectiveVal) {
        if (auto* knob = panel.getKnob(id))
            knob->setEffectiveValue(effectiveVal);
    };

    // --- Energy modulation on resonator parameters ---
    float baseBright = res0.getBaseBrightness();
    float effBright = juce::jlimit(0.0f, 1.0f, baseBright + energy * res0.getBrightnessEnergyAmount());
    showEffective(resonatorPanel, "brightness", effBright);

    float baseSpread = res0.getSpreadLevel();
    float effSpread = juce::jlimit(0.0f, 1.0f, baseSpread + energy * res0.getSpreadLevelEnergyAmount());
    showEffective(resonatorPanel, "spreadLevel", effSpread);

    float basePan = res0.getSpreadPanWidth();
    float effPan = juce::jlimit(0.0f, 1.0f, basePan + energy * res0.getPanWidthEnergyAmount());
    showEffective(resonatorPanel, "panWidth", effPan);

    // --- Macro influences (show where macros are pushing values) ---
    if (macrosEnabled.load(std::memory_order_relaxed))
    {
        // Size macro -> decay time
        float baseDecay = res0.getDecayTime();
        float effDecay = baseDecay * macroParameters.getDecayTimeScale();
        showEffective(resonatorPanel, "resDecay", juce::jlimit(0.5f, 15.0f, effDecay));

        // Size macro -> coupling
        float baseCoupling = ea.getCouplingRate();
        showEffective(energyPanel, "coupling",
                       juce::jlimit(0.0f, 0.01f, baseCoupling + macroParameters.getCouplingRateOffset()));

        // Material macro -> brightness (combined with energy)
        showEffective(resonatorPanel, "brightness",
                       juce::jlimit(0.0f, 1.0f, effBright + macroParameters.getBrightnessOffset()));

        // Intensity macro -> injection gain
        float baseInj = ea.getInjectionGain();
        showEffective(energyPanel, "injection",
                       juce::jlimit(0.1f, 3.0f, baseInj + macroParameters.getInjectionGainOffset()));

        // Intensity macro -> bloom threshold
        float baseBloom = ea.getBloomThreshold();
        showEffective(energyPanel, "bloom",
                       juce::jlimit(0.1f, 1.0f, baseBloom + macroParameters.getBloomThresholdOffset()));

        // Space macro -> conv mix
        float baseConvMix = convolutionEngine.getWetDryMix();
        showEffective(convolutionPanel, "convMix",
                       juce::jlimit(0.0f, 1.0f, baseConvMix + macroParameters.getConvMixOffset()));

        // Space macro -> pan width (combined with energy)
        showEffective(resonatorPanel, "panWidth",
                       juce::jlimit(0.0f, 1.0f, effPan + macroParameters.getSpreadWidthOffset()));
    }
}

//==============================================================================
// IR Loading
//==============================================================================

void MainComponent::loadIRFile()
{
    auto chooser = std::make_unique<juce::FileChooser>(
        "Select Impulse Response",
        irDirectory.exists() ? irDirectory : juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.aiff;*.aif;*.flac");
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            if (convolutionEngine.loadImpulseResponse(file))
            {
                convolutionContent.getIRFileLabel().setText(file.getFileName(), juce::dontSendNotification);
                convolutionContent.getIRWaveform().setIR(convolutionEngine.getIRBuffer(), convolutionEngine.getIRSampleRate());
            }
        }
    });
    static std::unique_ptr<juce::FileChooser> currentChooser;
    currentChooser = std::move(chooser);
}

void MainComponent::loadIRBFile()
{
    auto* chooser = new juce::FileChooser("Select IR B file...", irDirectory, "*.wav;*.aif;*.aiff;*.flac");
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            convolutionEngine.loadImpulseResponseB(file);
            convolutionContent.getIRBFileLabel().setText(file.getFileName(), juce::dontSendNotification);
        }
        delete chooser;
    });
}

//==============================================================================
// MIDI
//==============================================================================

void MainComponent::updateMidiDeviceList()
{
    midiDevices = juce::MidiInput::getAvailableDevices();
    midiInputList.clear();
    juce::StringArray midiInputNames;
    for (auto& device : midiDevices)
        midiInputNames.add(device.name);
    midiInputList.addItemList(midiInputNames, 1);
}

void MainComponent::setMidiInput(int index)
{
    if (index < 0 || index >= midiDevices.size()) return;

    if (lastMidiInputIndex >= 0 && lastMidiInputIndex < midiDevices.size())
    {
        auto prev = midiDevices[lastMidiInputIndex];
        if (!deviceManager.isMidiInputDeviceEnabled(prev.identifier))
            deviceManager.setMidiInputDeviceEnabled(prev.identifier, false);
        deviceManager.removeMidiInputDeviceCallback(prev.identifier, this);
    }

    auto newDevice = midiDevices[index];
    if (!deviceManager.isMidiInputDeviceEnabled(newDevice.identifier))
        deviceManager.setMidiInputDeviceEnabled(newDevice.identifier, true);

    deviceManager.addMidiInputDeviceCallback(newDevice.identifier, this);
    midiInputList.setSelectedId(index + 1, juce::dontSendNotification);
    lastMidiInputIndex = index;
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
    midiCollector.addMessageToQueue(message);
    if (message.isNoteOn())
        lastPlayedNote = message.getNoteNumber();
}

//==============================================================================
// Timer
//==============================================================================

void MainComponent::timerCallback()
{
    pushMeterData();
    pushEnergyData();
    pushEffectiveValues();
    resonatorGrid.refresh();

    if (diagnosticWindow)
        diagnosticWindow->refresh();

    repaint();
}

//==============================================================================
// Audio Processing (unchanged logic)
//==============================================================================

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;

    midiCollector.reset(sampleRate);
    gongSynth.prepareToPlay(sampleRate, samplesPerBlockExpected);
    convolutionEngine.prepare(sampleRate, samplesPerBlockExpected);
    exciterProcessor.prepare(sampleRate, samplesPerBlockExpected);
    multibandCompressor.prepare(sampleRate, samplesPerBlockExpected);

    synthBuffer.setSize(2, samplesPerBlockExpected);
    audioInputBuffer.setSize(2, samplesPerBlockExpected);
    testBuffer.setSize(2, samplesPerBlockExpected);

    testSignalGenerator.prepare(sampleRate);
    modulationBus.prepare(sampleRate);
    diagnosticState.budgetUs.store(static_cast<float>(1000000.0 * samplesPerBlockExpected / sampleRate),
                                    std::memory_order_relaxed);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    juce::MidiBuffer midiBuffer;
    midiCollector.removeNextBlockOfMessages(midiBuffer, bufferToFill.numSamples);

    auto numOutputChannels = bufferToFill.buffer->getNumChannels();
    auto numSamples = bufferToFill.numSamples;

    auto activeInputChannels = device->getActiveInputChannels();
    int numInputChannels = activeInputChannels.countNumberOfSetBits();
    if (numInputChannels == 0) numInputChannels = 1;

    if (synthBuffer.getNumChannels() != numOutputChannels || synthBuffer.getNumSamples() < numSamples)
        synthBuffer.setSize(numOutputChannels, numSamples, false, false, true);
    if (audioInputBuffer.getNumChannels() < 2 || audioInputBuffer.getNumSamples() < numSamples)
        audioInputBuffer.setSize(2, numSamples, false, false, true);
    if (testBuffer.getNumChannels() < 2 || testBuffer.getNumSamples() < numSamples)
        testBuffer.setSize(2, numSamples, false, false, true);

    auto* inputChannelData = bufferToFill.buffer->getArrayOfReadPointers();
    audioInputBuffer.copyFrom(0, 0, inputChannelData[0] + bufferToFill.startSample, numSamples);

    if (numInputChannels == 1 || numOutputChannels == 1)
        audioInputBuffer.copyFrom(1, 0, audioInputBuffer, 0, 0, numSamples);
    else
        audioInputBuffer.copyFrom(1, 0, inputChannelData[1] + bufferToFill.startSample, numSamples);

    // 1. Meter input
    diagnosticState.input.update(audioInputBuffer, 0, numSamples);

    // 2. Modulation
    modulationBus.computeBlock(diagnosticState, numSamples);
    applyModulationOffsets();

    // 3. Test signal
    int testType = diagnosticState.testSignalType.load(std::memory_order_relaxed);
    int testPoint = diagnosticState.testInjectionPoint.load(std::memory_order_relaxed);
    float testGain = diagnosticState.testSignalGain.load(std::memory_order_relaxed);
    float testFreq = diagnosticState.testSineFreq.load(std::memory_order_relaxed);

    auto injectTestSignal = [&](juce::AudioBuffer<float>& buf, int atPoint) {
        if (testType > 0 && testPoint == atPoint)
        {
            testBuffer.clear(0, numSamples);
            testSignalGenerator.generateBlock(testBuffer, numSamples, testType, testGain, testFreq);
            for (int ch = 0; ch < buf.getNumChannels() && ch < 2; ++ch)
                buf.addFrom(ch, 0, testBuffer, ch, 0, numSamples);
        }
    };

    injectTestSignal(audioInputBuffer, 0);

    // Bypass/Solo/Mute logic
    bool anySoloed = diagnosticState.isAnySoloed();
    int synthEff = diagnosticState.getEffectiveState(diagnosticState.synthState.load(std::memory_order_relaxed), anySoloed);
    int convEff = diagnosticState.getEffectiveState(diagnosticState.convState.load(std::memory_order_relaxed), anySoloed);
    int exciterEff = diagnosticState.getEffectiveState(diagnosticState.exciterState.load(std::memory_order_relaxed), anySoloed);
    int compEff = diagnosticState.getEffectiveState(diagnosticState.compState.load(std::memory_order_relaxed), anySoloed);

    synthBuffer.clear();

    // 4. Synth
    auto t0 = std::chrono::steady_clock::now();
    if (synthEff != 2) gongSynth.process(synthBuffer, audioInputBuffer, midiBuffer, numSamples);
    if (synthEff == 1) synthBuffer.clear(0, numSamples);
    auto t1 = std::chrono::steady_clock::now();
    diagnosticState.synthTime.record(static_cast<float>(std::chrono::duration<double, std::micro>(t1 - t0).count()));
    diagnosticState.afterSynth.update(synthBuffer, 0, numSamples);

    // Copy synth to output
    for (int ch = 0; ch < static_cast<int>(numOutputChannels); ++ch)
        bufferToFill.buffer->copyFrom(ch, bufferToFill.startSample, synthBuffer, ch, 0, numSamples);

    juce::AudioBuffer<float> processBuffer(bufferToFill.buffer->getArrayOfWritePointers(),
                                           static_cast<int>(numOutputChannels),
                                           bufferToFill.startSample, numSamples);

    injectTestSignal(processBuffer, 1);

    // 5. Convolution
    auto t2s = std::chrono::steady_clock::now();
    if (convEff != 2) convolutionEngine.process(processBuffer);
    if (convEff == 1) processBuffer.clear(0, numSamples);
    auto t2 = std::chrono::steady_clock::now();
    diagnosticState.convTime.record(static_cast<float>(std::chrono::duration<double, std::micro>(t2 - t2s).count()));
    diagnosticState.afterConv.update(processBuffer, 0, numSamples);

    injectTestSignal(processBuffer, 2);

    // 6. Exciter
    auto t3s = std::chrono::steady_clock::now();
    if (exciterEff != 2) exciterProcessor.process(processBuffer);
    if (exciterEff == 1) processBuffer.clear(0, numSamples);
    auto t3 = std::chrono::steady_clock::now();
    diagnosticState.exciterTime.record(static_cast<float>(std::chrono::duration<double, std::micro>(t3 - t3s).count()));
    diagnosticState.afterExciter.update(processBuffer, 0, numSamples);

    injectTestSignal(processBuffer, 3);

    // 7. Compressor
    auto t4s = std::chrono::steady_clock::now();
    if (compEff != 2) multibandCompressor.process(processBuffer);
    if (compEff == 1) processBuffer.clear(0, numSamples);
    auto t4 = std::chrono::steady_clock::now();
    diagnosticState.compTime.record(static_cast<float>(std::chrono::duration<double, std::micro>(t4 - t4s).count()));
    diagnosticState.afterComp.update(processBuffer, 0, numSamples);

    // 8. Master gain
    bufferToFill.buffer->applyGain(bufferToFill.startSample, numSamples, masterGain);

    // 9. Output metering
    diagnosticState.output.update(*bufferToFill.buffer, bufferToFill.startSample, numSamples);

    // 10. Total timing
    diagnosticState.totalTimeUs.store(
        static_cast<float>(std::chrono::duration<double, std::micro>(t4 - t0).count()),
        std::memory_order_relaxed);

    // 11. Energy values
    auto& ea = gongSynth.getEnergyAccumulator();
    diagnosticState.globalEnergy.store(ea.getNormalizedGlobalEnergy(), std::memory_order_relaxed);
    for (int i = 0; i < 4; ++i)
        diagnosticState.bandEnergy[i].store(ea.getNormalizedBandEnergy(i), std::memory_order_relaxed);
}

void MainComponent::applyModulationOffsets()
{
    auto get = [this](ModTarget t) { return modulationBus.getOffset(t); };
    auto& specs = getTargetSpecs();

    auto clamp = [&](ModTarget t, float base) {
        float offset = get(t);
        int idx = static_cast<int>(t);
        return juce::jlimit(specs[idx].minVal, specs[idx].maxVal, base + offset);
    };

    // Read base values from knobs via bindings
    auto getKnobVal = [](ModulePanel& panel, const juce::String& id) -> float {
        auto* slider = panel.getSlider(id);
        return slider ? static_cast<float>(slider->getValue()) : 0.0f;
    };

    // Convolution
    convolutionEngine.setWetDryMix(clamp(ModTarget::ConvMix, getKnobVal(convolutionPanel, "convMix")));
    convolutionEngine.setOutputGainDb(clamp(ModTarget::ConvGain, getKnobVal(convolutionPanel, "convGain")));

    // Exciter
    exciterProcessor.setHighpassFrequency(clamp(ModTarget::ExciterFreq, getKnobVal(exciterPanel, "exciterFreq")));
    exciterProcessor.setSaturationDrive(clamp(ModTarget::ExciterDrive, getKnobVal(exciterPanel, "exciterDrive")));
    exciterProcessor.setDryWetMix(clamp(ModTarget::ExciterMix, getKnobVal(exciterPanel, "exciterMix")));
    exciterProcessor.setOutputGainDb(clamp(ModTarget::ExciterOutputGain, exciterProcessor.getOutputGainDb()));

    // Compressor
    multibandCompressor.setAllThresholds(clamp(ModTarget::CompThreshold, getKnobVal(compressorPanel, "compThresh")));
    multibandCompressor.setAllRatios(clamp(ModTarget::CompRatio, getKnobVal(compressorPanel, "compRatio")));
    multibandCompressor.setAllAttacks(clamp(ModTarget::CompAttack, getKnobVal(compressorPanel, "compAttack")));
    multibandCompressor.setAllReleases(clamp(ModTarget::CompRelease, getKnobVal(compressorPanel, "compRelease")));

    // Synth
    gongSynth.setGlobalDecayTime(clamp(ModTarget::SynthDecay, getKnobVal(resonatorPanel, "resDecay")));
    gongSynth.setGlobalBrightness(clamp(ModTarget::SynthBrightness, getKnobVal(resonatorPanel, "brightness")));
    gongSynth.setGlobalSpreadLevel(clamp(ModTarget::SynthSpreadLevel, getKnobVal(resonatorPanel, "spreadLevel")));
    gongSynth.setGlobalSpreadPanWidth(clamp(ModTarget::SynthSpreadPanWidth, getKnobVal(resonatorPanel, "panWidth")));

    // Energy
    auto& ea = gongSynth.getEnergyAccumulator();
    ea.setGlobalDecayMs(clamp(ModTarget::EnergyDecayMs, getKnobVal(energyPanel, "energyDecay")));
    ea.setInjectionGain(clamp(ModTarget::EnergyInjectionGain, getKnobVal(energyPanel, "injection")));
    ea.setInjectionPower(clamp(ModTarget::EnergyInjectionPower, getKnobVal(energyPanel, "power")));

    // Roll prime
    ea.setRollPrimeLevel(clamp(ModTarget::RollPrimeLevel, ea.getRollPrimeLevel()));

    // Nonlinear dynamics
    if (nlDynamicsEnabled.load(std::memory_order_relaxed))
    {
        float glideDir = get(ModTarget::PitchGlideDirection);
        float glideSens = get(ModTarget::PitchGlideSensitivity);
        gongSynth.getResonatorBank().setAllPitchGlideDirection(juce::jlimit(-1.0f, 1.0f, 1.0f + glideDir));
        gongSynth.getResonatorBank().setAllPitchGlideSensitivity(juce::jlimit(0.0f, 200.0f, 80.0f + glideSens));
    }
    else
    {
        gongSynth.getResonatorBank().setAllPitchGlideSensitivity(0.0f);
        ea.setCouplingRate(0.0f);
        ea.setBloomThreshold(1.0f);
    }

    // Post-conv EQ
    convolutionEngine.setPostConvLowGainDb(juce::jlimit(-12.0f, 12.0f,
        getKnobVal(convolutionPanel, "postLowEQ") + get(ModTarget::PostConvLowGain)));
    convolutionEngine.setPostConvMidGainDb(juce::jlimit(-12.0f, 12.0f,
        getKnobVal(convolutionPanel, "postMidEQ") + get(ModTarget::PostConvMidGain)));
    convolutionEngine.setPostConvHighGainDb(juce::jlimit(-12.0f, 12.0f,
        getKnobVal(convolutionPanel, "postHighEQ") + get(ModTarget::PostConvHighGain)));

    // IR crossfade
    convolutionEngine.setIRCrossfadeEnergy(ea.getNormalizedGlobalEnergy());

    // Macros - apply ALL macro offsets to their target parameters
    if (macrosEnabled.load(std::memory_order_relaxed))
    {
        // Macro 1: Size -> decay time scale, coupling rate, fundamental scale
        float decayScale = macroParameters.getDecayTimeScale();
        float baseDecay = getKnobVal(resonatorPanel, "resDecay");
        gongSynth.setGlobalDecayTime(juce::jlimit(0.5f, 15.0f, baseDecay * decayScale));

        float baseCoupling = getKnobVal(energyPanel, "coupling");
        ea.setCouplingRate(juce::jlimit(0.0f, 0.01f, baseCoupling + macroParameters.getCouplingRateOffset()));

        // Macro 2: Material -> brightness, glide direction
        float baseBright = getKnobVal(resonatorPanel, "brightness");
        gongSynth.setGlobalBrightness(juce::jlimit(0.0f, 1.0f, baseBright + macroParameters.getBrightnessOffset()));

        if (nlDynamicsEnabled.load(std::memory_order_relaxed))
        {
            gongSynth.getResonatorBank().setAllPitchGlideDirection(
                juce::jlimit(-1.0f, 1.0f, 1.0f + get(ModTarget::PitchGlideDirection) + macroParameters.getGlideDirectionOffset()));
        }

        // Macro 3: Intensity -> injection gain, bloom threshold, crash threshold
        ea.setInjectionGain(juce::jlimit(0.1f, 5.0f, ea.getInjectionGain() + macroParameters.getInjectionGainOffset()));
        ea.setBloomThreshold(juce::jlimit(0.1f, 1.0f, ea.getBloomThreshold() + macroParameters.getBloomThresholdOffset()));
        gongSynth.getCrashNoiseGenerator().setThreshold(
            juce::jlimit(0.1f, 1.0f, gongSynth.getCrashNoiseGenerator().getThreshold() + macroParameters.getCrashThresholdOffset()));

        // Macro 4: Space -> conv mix, spread width
        float baseConvMix = getKnobVal(convolutionPanel, "convMix");
        convolutionEngine.setWetDryMix(juce::jlimit(0.0f, 1.0f, baseConvMix + macroParameters.getConvMixOffset()));

        float baseSpreadW = getKnobVal(resonatorPanel, "panWidth");
        gongSynth.setGlobalSpreadPanWidth(juce::jlimit(0.0f, 1.0f, baseSpreadW + macroParameters.getSpreadWidthOffset()));
    }

    // Master gain
    float mgBase = static_cast<float>(volumeKnob.getSlider().getValue());
    masterGain = clamp(ModTarget::MasterGain, mgBase);
}

void MainComponent::releaseResources()
{
    convolutionEngine.reset();
    exciterProcessor.reset();
    multibandCompressor.reset();
    gongSynth.releaseResources();
}

//==============================================================================
// PerformanceServer::Listener
//==============================================================================

juce::StringArray MainComponent::getPresetNames()
{
    return presetManager.getPresetNames();
}

bool MainComponent::activatePreset(const juce::String& name)
{
    if (presetManager.loadPreset(name, gongSynth, convolutionEngine,
                                  exciterProcessor, multibandCompressor, masterGain, &diagnosticState))
    {
        updateUIFromState();
        updatePresetComboBox();
        if (convolutionEngine.isLoaded())
            convolutionContent.getIRWaveform().setIR(convolutionEngine.getIRBuffer(), convolutionEngine.getIRSampleRate());
        return true;
    }
    return false;
}

juce::StringArray MainComponent::getIRNames()
{
    juce::StringArray names;
    if (irDirectory.exists())
    {
        auto files = irDirectory.findChildFiles(juce::File::findFiles, false, "*.wav;*.aiff;*.aif;*.flac");
        files.sort();
        for (auto& f : files)
            names.add(f.getFileName());
    }
    return names;
}

bool MainComponent::activateIR(const juce::String& name)
{
    if (!irDirectory.exists()) return false;
    auto irFile = irDirectory.getChildFile(name);
    if (!irFile.existsAsFile()) return false;

    if (convolutionEngine.loadImpulseResponse(irFile))
    {
        convolutionContent.getIRFileLabel().setText(irFile.getFileName(), juce::dontSendNotification);
        convolutionContent.getIRWaveform().setIR(convolutionEngine.getIRBuffer(), convolutionEngine.getIRSampleRate());
        return true;
    }
    return false;
}

juce::String MainComponent::getCurrentPresetName()
{
    return presetManager.getCurrentPresetName();
}

juce::String MainComponent::getCurrentIRName()
{
    return convolutionEngine.isLoaded() ? convolutionEngine.getIRFileName() : juce::String();
}

//==============================================================================
// Paint & Layout
//==============================================================================

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(GongLookAndFeel::kBgDark));

    // Title
    g.setFont(juce::FontOptions(15.0f).withStyle("Bold"));
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawText("GONG ENERGY SYNTHESIZER", getLocalBounds().removeFromTop(38), juce::Justification::centred);

    // Status bar at bottom
    auto statusArea = getLocalBounds().removeFromBottom(20);
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(juce::Colours::white.withAlpha(0.4f));

    auto& ea = gongSynth.getEnergyAccumulator();
    juce::String statusText = "Energy: " + juce::String(static_cast<int>(ea.getNormalizedGlobalEnergy() * 100)) + "%"
        + "  |  IR: " + (convolutionEngine.isLoaded() ? convolutionEngine.getIRFileName() : "None")
        + "  |  Mode: " + (gongSynth.getExcitationMode() == GongSynthesizer::ExcitationMode::AudioInput ? "Audio" : "Synthetic")
        + "  |  Preset: " + (presetManager.getCurrentPresetName().isEmpty() ? "None" : presetManager.getCurrentPresetName());
    g.drawText(statusText, statusArea, juce::Justification::centred);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // === Title area (top 20px) - just painted text, no components ===
    area.removeFromTop(20);

    // === Toolbar (36px tall, with generous spacing) ===
    auto toolbar = area.removeFromTop(36);
    // Center the toolbar items with margins
    int toolbarContentW = 900; // approximate total width of toolbar items
    int toolbarMargin = (toolbar.getWidth() - toolbarContentW) / 2;
    toolbar = toolbar.reduced(juce::jmax(20, toolbarMargin), 4);

    midiInputListLabel.setBounds(toolbar.removeFromLeft(35));
    midiInputList.setBounds(toolbar.removeFromLeft(130));
    toolbar.removeFromLeft(16);

    audioInputModeButton.setBounds(toolbar.removeFromLeft(60));
    toolbar.removeFromLeft(4);
    syntheticModeButton.setBounds(toolbar.removeFromLeft(60));
    toolbar.removeFromLeft(4);
    auditionButton.setBounds(toolbar.removeFromLeft(55));
    toolbar.removeFromLeft(20);

    presetLabel.setBounds(toolbar.removeFromLeft(40));
    presetComboBox.setBounds(toolbar.removeFromLeft(120));
    toolbar.removeFromLeft(6);
    presetSaveButton.setBounds(toolbar.removeFromLeft(36));
    toolbar.removeFromLeft(6);
    presetSaveAsButton.setBounds(toolbar.removeFromLeft(55));
    toolbar.removeFromLeft(20);

    volumeKnob.setBounds(toolbar.removeFromLeft(55).withHeight(28));
    toolbar.removeFromLeft(12);
    panicButton.setBounds(toolbar.removeFromLeft(48));
    toolbar.removeFromLeft(8);
    diagnosticsButton.setBounds(toolbar.removeFromLeft(36));

    area.removeFromTop(6); // breathing room below toolbar

    // === Status bar (bottom 20px) ===
    area.removeFromBottom(20);

    // === Main content area with generous margins ===
    int sideMargin = 24;  // breathing room on left and right
    auto content = area.reduced(sideMargin, 4);

    // Right sidebar for macros
    int macroSidebarW = 82;
    int macroGap = 16; // gap between modules and macros
    auto macroArea = content.removeFromRight(macroSidebarW);
    content.removeFromRight(macroGap);

    int macroKnobH = macroArea.getHeight() / 4;
    for (int i = 0; i < 4; ++i)
        macroKnobs[i].setBounds(macroArea.removeFromTop(macroKnobH));

    // === Two-row module layout ===
    // Row 1 (58%): Input -> Energy -> Resonator -> Combo+Crash
    // Row 2 (42%): Convolution -> Exciter -> Compressor
    // Row 2 left edge aligns with row 1 left edge
    // Row 2 right edge aligns with row 1 right edge (combo/crash)

    int rowGap = 44;  // generous gap for cross-row cable routing
    int colGap = 20;  // spacing between modules in same row

    int totalH = content.getHeight();
    int row1H = (totalH - rowGap) * 58 / 100;
    int row2H = totalH - row1H - rowGap;

    auto row1 = content.removeFromTop(row1H);
    content.removeFromTop(rowGap);
    auto row2 = content.withHeight(row2H);

    // === Row 1: Input | Energy | Resonator | Combo+Crash ===
    int inputW = 250;
    int energyW = 220;
    int comboCrashW = 155;
    int availForRes = row1.getWidth() - inputW - energyW - comboCrashW - 3 * colGap;
    int resonatorW = juce::jmin(availForRes, 390);

    // Distribute extra space as padding between modules (not centering)
    int row1UsedW = inputW + energyW + resonatorW + comboCrashW + 3 * colGap;
    int row1Extra = row1.getWidth() - row1UsedW;
    int extraPerGap = row1Extra / 4; // spread across 3 gaps + margins

    row1.removeFromLeft(extraPerGap / 2);

    inputPanel.setBounds(row1.removeFromLeft(inputW));
    row1.removeFromLeft(colGap + extraPerGap);
    energyPanel.setBounds(row1.removeFromLeft(energyW));
    row1.removeFromLeft(colGap + extraPerGap);
    resonatorPanel.setBounds(row1.removeFromLeft(resonatorW));
    row1.removeFromLeft(colGap + extraPerGap);

    auto comboCrashArea = row1.withWidth(comboCrashW);
    int comboH = comboCrashArea.getHeight() / 2 - 2;
    comboTonePanel.setBounds(comboCrashArea.removeFromTop(comboH));
    comboCrashArea.removeFromTop(4);
    crashPanel.setBounds(comboCrashArea);

    // Record row 1 extents for aligning row 2
    int row1Left = inputPanel.getX();
    int row1Right = comboTonePanel.getRight();

    // === Row 2: Convolution | Exciter | Compressor ===
    // Aligned: left edge = row1Left, right edge = row1Right
    int row2TotalW = row1Right - row1Left;
    int convW = row2TotalW * 40 / 100;  // ~40% for convolution (wider)
    int exciterW = row2TotalW * 28 / 100;
    int compW = row2TotalW - convW - exciterW - 2 * colGap;

    auto row2Aligned = row2.withX(row1Left).withWidth(row2TotalW);

    convolutionPanel.setBounds(row2Aligned.removeFromLeft(convW));
    row2Aligned.removeFromLeft(colGap);
    exciterPanel.setBounds(row2Aligned.removeFromLeft(exciterW));
    row2Aligned.removeFromLeft(colGap);
    compressorPanel.setBounds(row2Aligned);

    // Modal template in resonator area
    auto resBounds = resonatorPanel.getBounds();
    modalTemplateLabel.setBounds(resBounds.getX() + 4, resBounds.getBottom() - 22, 55, 18);
    modalTemplateCombo.setBounds(resBounds.getX() + 58, resBounds.getBottom() - 22, 110, 18);

    // Output meter in bottom status bar
    auto statusArea = getLocalBounds().removeFromBottom(20);
    outputMeter.setBounds(statusArea.removeFromRight(120).reduced(2, 2));

    // Patch cable overlay - set cross-row route Y to middle of gap between rows
    float gapTop = static_cast<float>(inputPanel.getBottom());
    float gapBottom = static_cast<float>(convolutionPanel.getY());
    patchCables.setCrossRowY(gapTop + (gapBottom - gapTop) * 0.5f);
    patchCables.setBounds(getLocalBounds());
    patchCables.toFront(false);
}
