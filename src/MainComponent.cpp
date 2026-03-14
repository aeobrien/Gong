#include "MainComponent.h"
#include <chrono>

MainComponent::MainComponent()
{
    setSize(1050, 1250);

    // Resolve directories relative to source
    sourceDirectory = juce::File(__FILE__).getParentDirectory().getParentDirectory();
    irDirectory = sourceDirectory.getChildFile("IRs");
    presetsDirectory = sourceDirectory.getChildFile("presets");
    webDirectory = sourceDirectory.getChildFile("web");

    // Initialize preset manager
    presetManager.setPresetsDirectory(presetsDirectory);
    presetManager.setIRDirectory(irDirectory);

    // MIDI input selector
    addAndMakeVisible(midiInputListLabel);
    midiInputListLabel.setText("MIDI:", juce::dontSendNotification);

    addAndMakeVisible(midiInputList);
    midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs");
    midiInputList.addListener(this);

    updateMidiDeviceList();
    if (midiDevices.size() > 0)
        setMidiInput(0);

    // === EXCITATION MODE ===
    audioInputModeButton.setRadioGroupId(999);
    audioInputModeButton.setToggleState(true, juce::dontSendNotification);
    audioInputModeButton.addListener(this);
    addAndMakeVisible(audioInputModeButton);

    syntheticModeButton.setRadioGroupId(999);
    syntheticModeButton.addListener(this);
    addAndMakeVisible(syntheticModeButton);

    // === INPUT SECTION ===
    addAndMakeVisible(inputGainLabel);
    addAndMakeVisible(inputGainSlider);
    inputGainSlider.setRange(0.0, 4.0, 0.01);
    inputGainSlider.setValue(1.0);
    inputGainSlider.addListener(this);

    addAndMakeVisible(strikeThreshLabel);
    addAndMakeVisible(strikeThreshSlider);
    strikeThreshSlider.setRange(0.01, 0.5, 0.01);
    strikeThreshSlider.setValue(0.1);
    strikeThreshSlider.addListener(this);

    addAndMakeVisible(strikeHoldoffLabel);
    addAndMakeVisible(strikeHoldoffSlider);
    strikeHoldoffSlider.setRange(10.0, 200.0, 1.0);
    strikeHoldoffSlider.setValue(50.0);
    strikeHoldoffSlider.setTextValueSuffix(" ms");
    strikeHoldoffSlider.addListener(this);

    // === ENERGY SECTION ===
    addAndMakeVisible(energyDecayLabel);
    addAndMakeVisible(energyDecaySlider);
    energyDecaySlider.setRange(500.0, 5000.0, 10.0);
    energyDecaySlider.setValue(2000.0);
    energyDecaySlider.setTextValueSuffix(" ms");
    energyDecaySlider.addListener(this);

    addAndMakeVisible(energyInjectionLabel);
    addAndMakeVisible(energyInjectionSlider);
    energyInjectionSlider.setRange(0.1, 3.0, 0.01);
    energyInjectionSlider.setValue(1.0);
    energyInjectionSlider.addListener(this);

    addAndMakeVisible(energyPowerLabel);
    addAndMakeVisible(energyPowerSlider);
    energyPowerSlider.setRange(0.5, 3.0, 0.01);
    energyPowerSlider.setValue(1.5);
    energyPowerSlider.addListener(this);

    // === GLOBAL RESONATOR CONTROLS ===
    addAndMakeVisible(globalDecayLabel);
    addAndMakeVisible(globalDecaySlider);
    globalDecaySlider.setRange(0.5, 15.0, 0.1);
    globalDecaySlider.setValue(4.0);
    globalDecaySlider.setTextValueSuffix(" s");
    globalDecaySlider.addListener(this);

    addAndMakeVisible(globalBrightnessLabel);
    addAndMakeVisible(globalBrightnessSlider);
    globalBrightnessSlider.setRange(0.0, 1.0, 0.01);
    globalBrightnessSlider.setValue(0.7);
    globalBrightnessSlider.addListener(this);

    addAndMakeVisible(globalSpreadLevelLabel);
    addAndMakeVisible(globalSpreadLevelSlider);
    globalSpreadLevelSlider.setRange(0.0, 1.0, 0.01);
    globalSpreadLevelSlider.setValue(0.5);
    globalSpreadLevelSlider.addListener(this);

    addAndMakeVisible(globalSpreadPanWidthLabel);
    addAndMakeVisible(globalSpreadPanWidthSlider);
    globalSpreadPanWidthSlider.setRange(0.0, 1.0, 0.01);
    globalSpreadPanWidthSlider.setValue(1.0);
    globalSpreadPanWidthSlider.addListener(this);

    // === RESONATOR COLUMN HEADERS ===
    auto setupHeader = [](juce::Label& label) {
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(11.0f));
    };

    addAndMakeVisible(resHeaderOn); setupHeader(resHeaderOn);
    addAndMakeVisible(resHeaderMode); setupHeader(resHeaderMode);
    addAndMakeVisible(resHeaderFreq); setupHeader(resHeaderFreq);
    addAndMakeVisible(resHeaderGain); setupHeader(resHeaderGain);
    addAndMakeVisible(resHeaderBright); setupHeader(resHeaderBright);
    addAndMakeVisible(resHeaderSpread); setupHeader(resHeaderSpread);
    addAndMakeVisible(resHeaderDetune); setupHeader(resHeaderDetune);
    addAndMakeVisible(resHeaderPan); setupHeader(resHeaderPan);

    // === PER-RESONATOR CONTROLS ===
    const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    float defaultFreqs[] = { 110.0f, 220.0f, 330.0f, 440.0f };
    int defaultNotes[] = { 45, 57, 64, 69 };

    for (int i = 0; i < kNumResonators; ++i)
    {
        auto& rc = resonatorControls[i];

        rc.nameLabel.setText("R" + juce::String(i + 1), juce::dontSendNotification);
        rc.nameLabel.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
        addAndMakeVisible(rc.nameLabel);

        rc.enableButton.setToggleState(true, juce::dontSendNotification);
        rc.enableButton.addListener(this);
        addAndMakeVisible(rc.enableButton);

        // Mode toggle
        rc.freeModeButton.setRadioGroupId(100 + i);
        rc.freeModeButton.setToggleState(true, juce::dontSendNotification);
        rc.freeModeButton.addListener(this);
        addAndMakeVisible(rc.freeModeButton);

        rc.snapModeButton.setRadioGroupId(100 + i);
        rc.snapModeButton.addListener(this);
        addAndMakeVisible(rc.snapModeButton);

        // Frequency slider (Free mode)
        rc.freqSlider.setRange(20.0, 2000.0, 1.0);
        rc.freqSlider.setSkewFactorFromMidPoint(220.0);
        rc.freqSlider.setValue(defaultFreqs[i]);
        rc.freqSlider.setTextValueSuffix(" Hz");
        rc.freqSlider.addListener(this);
        rc.freqSlider.setVisible(true);
        addAndMakeVisible(rc.freqSlider);

        // MIDI note combo (Snap mode)
        for (int note = 24; note <= 96; ++note)
        {
            int octave = (note / 12) - 1;
            int noteIndex = note % 12;
            juce::String noteName = juce::String(noteNames[noteIndex]) + juce::String(octave);
            rc.noteCombo.addItem(noteName, note + 1);
        }
        rc.noteCombo.setSelectedId(defaultNotes[i] + 1, juce::dontSendNotification);
        rc.noteCombo.addListener(this);
        addChildComponent(rc.noteCombo);

        // Gain slider
        rc.gainSlider.setRange(-24.0, 12.0, 0.1);
        rc.gainSlider.setValue(0.0);
        rc.gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        rc.gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        rc.gainSlider.addListener(this);
        addAndMakeVisible(rc.gainSlider);

        // Frequency display
        rc.freqValueLabel.setText(juce::String((int)defaultFreqs[i]) + " Hz", juce::dontSendNotification);
        rc.freqValueLabel.setJustificationType(juce::Justification::centred);
        rc.freqValueLabel.setFont(juce::FontOptions(10.0f));
        addAndMakeVisible(rc.freqValueLabel);

        // Energy modulation sliders
        auto setupModSlider = [this](juce::Slider& slider, float defaultVal) {
            slider.setRange(0.0, 1.0, 0.01);
            slider.setValue(defaultVal);
            slider.setSliderStyle(juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            slider.addListener(this);
            addAndMakeVisible(slider);
        };

        setupModSlider(rc.brightnessModSlider, 0.5f);
        setupModSlider(rc.spreadModSlider, 0.3f);
        setupModSlider(rc.detuneModSlider, 0.2f);
        setupModSlider(rc.panModSlider, 0.3f);
    }

    // === CONVOLUTION SECTION ===
    addAndMakeVisible(irWaveform);

    addAndMakeVisible(loadIRButton);
    loadIRButton.addListener(this);

    addAndMakeVisible(irFileLabel);

    addAndMakeVisible(reverbMixLabel);
    addAndMakeVisible(reverbMixSlider);
    reverbMixSlider.setRange(0.0, 1.0, 0.01);
    reverbMixSlider.setValue(0.4);
    reverbMixSlider.addListener(this);

    addAndMakeVisible(convGainLabel);
    addAndMakeVisible(convGainSlider);
    convGainSlider.setRange(-24.0, 24.0, 0.1);
    convGainSlider.setValue(0.0);
    convGainSlider.setTextValueSuffix(" dB");
    convGainSlider.addListener(this);

    // === EXCITER SECTION ===
    addAndMakeVisible(exciterEnableButton);
    exciterEnableButton.setToggleState(true, juce::dontSendNotification);
    exciterEnableButton.addListener(this);

    addAndMakeVisible(exciterFreqLabel);
    addAndMakeVisible(exciterFreqSlider);
    exciterFreqSlider.setRange(200.0, 2000.0, 1.0);
    exciterFreqSlider.setValue(500.0);
    exciterFreqSlider.setTextValueSuffix(" Hz");
    exciterFreqSlider.addListener(this);

    addAndMakeVisible(exciterDriveLabel);
    addAndMakeVisible(exciterDriveSlider);
    exciterDriveSlider.setRange(1.0, 5.0, 0.1);
    exciterDriveSlider.setValue(2.0);
    exciterDriveSlider.addListener(this);

    addAndMakeVisible(exciterMixLabel);
    addAndMakeVisible(exciterMixSlider);
    exciterMixSlider.setRange(0.0, 1.0, 0.01);
    exciterMixSlider.setValue(0.3);
    exciterMixSlider.addListener(this);

    // === MULTIBAND COMPRESSOR SECTION ===
    addAndMakeVisible(compEnableButton);
    compEnableButton.setToggleState(true, juce::dontSendNotification);
    compEnableButton.addListener(this);

    addAndMakeVisible(compThreshLabel);
    addAndMakeVisible(compThreshSlider);
    compThreshSlider.setRange(-40.0, 0.0, 0.1);
    compThreshSlider.setValue(-12.0);
    compThreshSlider.setTextValueSuffix(" dB");
    compThreshSlider.addListener(this);

    addAndMakeVisible(compRatioLabel);
    addAndMakeVisible(compRatioSlider);
    compRatioSlider.setRange(1.0, 20.0, 0.1);
    compRatioSlider.setValue(4.0);
    compRatioSlider.addListener(this);

    addAndMakeVisible(compAttackLabel);
    addAndMakeVisible(compAttackSlider);
    compAttackSlider.setRange(1.0, 100.0, 1.0);
    compAttackSlider.setValue(10.0);
    compAttackSlider.setTextValueSuffix(" ms");
    compAttackSlider.addListener(this);

    addAndMakeVisible(compReleaseLabel);
    addAndMakeVisible(compReleaseSlider);
    compReleaseSlider.setRange(10.0, 500.0, 1.0);
    compReleaseSlider.setValue(100.0);
    compReleaseSlider.setTextValueSuffix(" ms");
    compReleaseSlider.addListener(this);

    // === PRESET SECTION ===
    addAndMakeVisible(presetLabel);
    addAndMakeVisible(presetComboBox);
    presetComboBox.addListener(this);
    updatePresetComboBox();

    addAndMakeVisible(presetSaveButton);
    presetSaveButton.addListener(this);

    addAndMakeVisible(presetSaveAsButton);
    presetSaveAsButton.addListener(this);

    // === NONLINEAR DYNAMICS CONTROLS ===
    addAndMakeVisible(couplingRateLabel);
    addAndMakeVisible(couplingRateSlider);
    couplingRateSlider.setRange(0.0, 0.01, 0.0001);
    couplingRateSlider.setValue(0.001);
    couplingRateSlider.addListener(this);

    addAndMakeVisible(bloomThreshLabel);
    addAndMakeVisible(bloomThreshSlider);
    bloomThreshSlider.setRange(0.1, 1.0, 0.01);
    bloomThreshSlider.setValue(0.7);
    bloomThreshSlider.addListener(this);

    addAndMakeVisible(glideDirectionLabel);
    addAndMakeVisible(glideDirectionSlider);
    glideDirectionSlider.setRange(-1.0, 1.0, 0.01);
    glideDirectionSlider.setValue(1.0);
    glideDirectionSlider.addListener(this);

    addAndMakeVisible(glideSensitivityLabel);
    addAndMakeVisible(glideSensitivitySlider);
    glideSensitivitySlider.setRange(0.0, 200.0, 1.0);
    glideSensitivitySlider.setValue(80.0);
    glideSensitivitySlider.addListener(this);

    addAndMakeVisible(postConvLowLabel);
    addAndMakeVisible(postConvLowSlider);
    postConvLowSlider.setRange(-12.0, 12.0, 0.1);
    postConvLowSlider.setValue(0.0);
    postConvLowSlider.addListener(this);

    addAndMakeVisible(postConvMidLabel);
    addAndMakeVisible(postConvMidSlider);
    postConvMidSlider.setRange(-12.0, 12.0, 0.1);
    postConvMidSlider.setValue(0.0);
    postConvMidSlider.addListener(this);

    addAndMakeVisible(postConvHighLabel);
    addAndMakeVisible(postConvHighSlider);
    postConvHighSlider.setRange(-12.0, 12.0, 0.1);
    postConvHighSlider.setValue(0.0);
    postConvHighSlider.addListener(this);

    addAndMakeVisible(modalTemplateLabel);
    addAndMakeVisible(modalTemplateCombo);
    modalTemplateCombo.addItem("Manual", 1);
    for (int i = 0; i < kNumTemplates; ++i)
        modalTemplateCombo.addItem(kAllTemplates[i]->name, i + 2);
    modalTemplateCombo.setSelectedId(1, juce::dontSendNotification);
    modalTemplateCombo.addListener(this);

    addAndMakeVisible(loadIRBButton);
    loadIRBButton.addListener(this);
    addAndMakeVisible(irBFileLabel);

    // Section enable toggles
    addAndMakeVisible(convSectionToggle);
    convSectionToggle.setToggleState(true, juce::dontSendNotification);
    convSectionToggle.addListener(this);
    convSectionToggle.setTooltip("Enable/disable convolution reverb processing");

    addAndMakeVisible(nlSectionToggle);
    nlSectionToggle.setToggleState(true, juce::dontSendNotification);
    nlSectionToggle.addListener(this);
    nlSectionToggle.setTooltip("Enable/disable nonlinear dynamics (coupling, bloom, pitch glide)");

    addAndMakeVisible(macroSectionToggle);
    macroSectionToggle.setToggleState(true, juce::dontSendNotification);
    macroSectionToggle.addListener(this);
    macroSectionToggle.setTooltip("Enable/disable macro parameter offsets");

    // Macro knobs
    static const char* macroNames[] = { "Size", "Material", "Intensity", "Space" };
    for (int i = 0; i < 4; ++i)
    {
        macroLabels[i].setText(macroNames[i], juce::dontSendNotification);
        addAndMakeVisible(macroLabels[i]);
        addAndMakeVisible(macroSliders[i]);
        macroSliders[i].setRange(0.0, 1.0, 0.01);
        macroSliders[i].setValue(0.5);
        macroSliders[i].addListener(this);
    }

    // === BOTTOM CONTROLS ===
    addAndMakeVisible(volumeLabel);
    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.8);
    volumeSlider.addListener(this);

    addAndMakeVisible(panicButton);
    panicButton.addListener(this);

    addAndMakeVisible(diagnosticsButton);
    diagnosticsButton.addListener(this);

    // Setup tooltips
    setupTooltips();

    // Request microphone permission
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

    // Request larger buffer size to avoid overloads with convolution
    {
        auto setup = deviceManager.getAudioDeviceSetup();
        if (setup.bufferSize < 2048)
        {
            setup.bufferSize = 2048;
            deviceManager.setAudioDeviceSetup(setup, true);
        }
    }

    // Start performance server
    performanceServer = std::make_unique<PerformanceServer>(*this, 8080);
    performanceServer->setWebRoot(webDirectory);
    performanceServer->startServer();

    startTimerHz(15);
}

void MainComponent::setupTooltips()
{
    inputGainSlider.setTooltip("Amplifies incoming audio before strike detection and resonator excitation");
    strikeThreshSlider.setTooltip("Audio level threshold that triggers energy injection (lower = more sensitive)");
    strikeHoldoffSlider.setTooltip("Minimum time between strike detections to prevent retriggering");

    energyDecaySlider.setTooltip("How long accumulated energy takes to fade out (longer = sustained brightness)");
    energyInjectionSlider.setTooltip("How much energy is added per strike (higher = faster energy buildup)");
    energyPowerSlider.setTooltip("Shapes the velocity-to-energy curve (1.0=linear, >1=emphasize loud hits, <1=emphasize soft hits)");

    globalDecaySlider.setTooltip("How long resonators ring after excitation (filter Q derived from this)");
    globalBrightnessSlider.setTooltip("Base tonal brightness before energy modulation (affects filter Q)");
    globalSpreadLevelSlider.setTooltip("Level of the 6 spread voices relative to center voice");
    globalSpreadPanWidthSlider.setTooltip("Stereo width of spread voices (0=mono, 1=full stereo)");

    audioInputModeButton.setTooltip("Use microphone/line input as excitation source");
    syntheticModeButton.setTooltip("Use MIDI controller to generate shaped noise bursts");

    for (int i = 0; i < kNumResonators; ++i)
    {
        auto& rc = resonatorControls[i];
        rc.enableButton.setTooltip("Enable/disable this resonator");
        rc.freeModeButton.setTooltip("Free mode: set frequency directly in Hz");
        rc.snapModeButton.setTooltip("Snap mode: set frequency by MIDI note");
        rc.freqSlider.setTooltip("Resonator center frequency in Hz");
        rc.noteCombo.setTooltip("Resonator frequency as MIDI note");
        rc.gainSlider.setTooltip("Individual resonator output level (-24 to +12 dB)");
        rc.brightnessModSlider.setTooltip("How much energy increases brightness/Q (0=none, 1=full)");
        rc.spreadModSlider.setTooltip("How much energy increases spread voice level (0=none, 1=full)");
        rc.detuneModSlider.setTooltip("How much energy increases spread voice detuning (0=none, 1=full)");
        rc.panModSlider.setTooltip("How much energy increases stereo width (0=none, 1=full)");
    }

    loadIRButton.setTooltip("Load an impulse response WAV file for convolution reverb");
    reverbMixSlider.setTooltip("Dry/wet mix for convolution reverb (0=dry, 1=wet)");
    convGainSlider.setTooltip("Output gain for convolution reverb");

    exciterEnableButton.setTooltip("Enable harmonic exciter (adds brightness via saturation)");
    exciterFreqSlider.setTooltip("Highpass frequency - only frequencies above this are excited");
    exciterDriveSlider.setTooltip("Saturation amount (higher = more harmonics)");
    exciterMixSlider.setTooltip("Dry/wet mix for exciter (0=dry, 1=wet)");

    compEnableButton.setTooltip("Enable 3-band multiband compressor");
    compThreshSlider.setTooltip("Level above which compression begins");
    compRatioSlider.setTooltip("Compression ratio (4:1 means 4dB over threshold becomes 1dB)");
    compAttackSlider.setTooltip("How fast compressor responds to transients");
    compReleaseSlider.setTooltip("How fast compressor releases after signal drops");

    volumeSlider.setTooltip("Master output volume");
    panicButton.setTooltip("Immediately silence all audio and reset energy");
}

void MainComponent::updateResonatorFrequencyDisplay(int index)
{
    if (index < 0 || index >= kNumResonators) return;

    auto& rc = resonatorControls[index];
    auto& resonator = gongSynth.getResonatorBank().getResonator(index);

    float freq = resonator.getEffectiveFrequency();
    rc.freqValueLabel.setText(juce::String((int)freq) + " Hz", juce::dontSendNotification);
}

void MainComponent::updatePresetComboBox()
{
    presetComboBox.clear(juce::dontSendNotification);
    auto names = presetManager.getPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetComboBox.addItem(names[i], i + 1);

    // Select current preset if any
    auto currentName = presetManager.getCurrentPresetName();
    if (currentName.isNotEmpty())
    {
        int idx = names.indexOf(currentName);
        if (idx >= 0)
            presetComboBox.setSelectedId(idx + 1, juce::dontSendNotification);
    }
}

void MainComponent::updateUIFromState()
{
    // Update all UI controls from the current synth state
    inputGainSlider.setValue(gongSynth.getInputGain(), juce::dontSendNotification);
    strikeThreshSlider.setValue(gongSynth.getStrikeThreshold(), juce::dontSendNotification);
    strikeHoldoffSlider.setValue(gongSynth.getStrikeHoldoffMs(), juce::dontSendNotification);

    auto& ea = gongSynth.getEnergyAccumulator();
    energyDecaySlider.setValue(ea.getGlobalDecayMs(), juce::dontSendNotification);
    energyInjectionSlider.setValue(ea.getInjectionGain(), juce::dontSendNotification);
    energyPowerSlider.setValue(ea.getInjectionPower(), juce::dontSendNotification);

    auto& bank = gongSynth.getResonatorBank();
    globalDecaySlider.setValue(bank.getResonator(0).getDecayTime(), juce::dontSendNotification);
    globalBrightnessSlider.setValue(bank.getResonator(0).getBaseBrightness(), juce::dontSendNotification);
    globalSpreadLevelSlider.setValue(bank.getResonator(0).getSpreadLevel(), juce::dontSendNotification);
    globalSpreadPanWidthSlider.setValue(bank.getResonator(0).getSpreadPanWidth(), juce::dontSendNotification);

    for (int i = 0; i < kNumResonators; ++i)
    {
        auto& rc = resonatorControls[i];
        auto& res = bank.getResonator(i);

        rc.enableButton.setToggleState(res.getEnabled(), juce::dontSendNotification);

        bool isFree = res.getFrequencyMode() == SpreadVoiceResonator::FrequencyMode::Free;
        rc.freeModeButton.setToggleState(isFree, juce::dontSendNotification);
        rc.snapModeButton.setToggleState(!isFree, juce::dontSendNotification);
        rc.freqSlider.setVisible(isFree);
        rc.noteCombo.setVisible(!isFree);

        rc.freqSlider.setValue(res.getFrequencyHz(), juce::dontSendNotification);
        rc.noteCombo.setSelectedId(res.getMidiNote() + 1, juce::dontSendNotification);
        rc.gainSlider.setValue(res.getGainDb(), juce::dontSendNotification);

        rc.brightnessModSlider.setValue(res.getBrightnessEnergyAmount(), juce::dontSendNotification);
        rc.spreadModSlider.setValue(res.getSpreadLevelEnergyAmount(), juce::dontSendNotification);
        rc.detuneModSlider.setValue(res.getSpreadDetuneEnergyAmount(), juce::dontSendNotification);
        rc.panModSlider.setValue(res.getPanWidthEnergyAmount(), juce::dontSendNotification);

        updateResonatorFrequencyDisplay(i);
    }

    reverbMixSlider.setValue(convolutionEngine.getWetDryMix(), juce::dontSendNotification);
    convGainSlider.setValue(convolutionEngine.getOutputGainDb(), juce::dontSendNotification);

    exciterEnableButton.setToggleState(exciterProcessor.getEnabled(), juce::dontSendNotification);
    exciterFreqSlider.setValue(exciterProcessor.getHighpassFrequency(), juce::dontSendNotification);
    exciterDriveSlider.setValue(exciterProcessor.getSaturationDrive(), juce::dontSendNotification);
    exciterMixSlider.setValue(exciterProcessor.getDryWetMix(), juce::dontSendNotification);

    compEnableButton.setToggleState(multibandCompressor.getEnabled(), juce::dontSendNotification);
    auto& bs = multibandCompressor.getBandSettings(0);
    compThreshSlider.setValue(bs.threshold, juce::dontSendNotification);
    compRatioSlider.setValue(bs.ratio, juce::dontSendNotification);
    compAttackSlider.setValue(bs.attackMs, juce::dontSendNotification);
    compReleaseSlider.setValue(bs.releaseMs, juce::dontSendNotification);

    volumeSlider.setValue(masterGain, juce::dontSendNotification);

    if (convolutionEngine.isLoaded())
        irFileLabel.setText(convolutionEngine.getIRFileName(), juce::dontSendNotification);
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
}

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
    if (index < 0 || index >= midiDevices.size())
        return;

    if (lastMidiInputIndex >= 0 && lastMidiInputIndex < midiDevices.size())
    {
        auto previousDevice = midiDevices[lastMidiInputIndex];
        if (!deviceManager.isMidiInputDeviceEnabled(previousDevice.identifier))
            deviceManager.setMidiInputDeviceEnabled(previousDevice.identifier, false);
        deviceManager.removeMidiInputDeviceCallback(previousDevice.identifier, this);
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

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    // Input controls
    if (slider == &inputGainSlider)
        gongSynth.setInputGain(static_cast<float>(inputGainSlider.getValue()));
    else if (slider == &strikeThreshSlider)
        gongSynth.setStrikeThreshold(static_cast<float>(strikeThreshSlider.getValue()));
    else if (slider == &strikeHoldoffSlider)
        gongSynth.setStrikeHoldoffMs(static_cast<float>(strikeHoldoffSlider.getValue()));
    // Energy controls
    else if (slider == &energyDecaySlider)
        gongSynth.getEnergyAccumulator().setGlobalDecayMs(static_cast<float>(energyDecaySlider.getValue()));
    else if (slider == &energyInjectionSlider)
        gongSynth.getEnergyAccumulator().setInjectionGain(static_cast<float>(energyInjectionSlider.getValue()));
    else if (slider == &energyPowerSlider)
        gongSynth.getEnergyAccumulator().setInjectionPower(static_cast<float>(energyPowerSlider.getValue()));
    // Global resonator controls
    else if (slider == &globalDecaySlider)
        gongSynth.setGlobalDecayTime(static_cast<float>(globalDecaySlider.getValue()));
    else if (slider == &globalBrightnessSlider)
        gongSynth.setGlobalBrightness(static_cast<float>(globalBrightnessSlider.getValue()));
    else if (slider == &globalSpreadLevelSlider)
        gongSynth.setGlobalSpreadLevel(static_cast<float>(globalSpreadLevelSlider.getValue()));
    else if (slider == &globalSpreadPanWidthSlider)
        gongSynth.setGlobalSpreadPanWidth(static_cast<float>(globalSpreadPanWidthSlider.getValue()));
    // Convolution
    else if (slider == &reverbMixSlider)
        convolutionEngine.setWetDryMix(static_cast<float>(reverbMixSlider.getValue()));
    else if (slider == &convGainSlider)
        convolutionEngine.setOutputGainDb(static_cast<float>(convGainSlider.getValue()));
    // Exciter
    else if (slider == &exciterFreqSlider)
        exciterProcessor.setHighpassFrequency(static_cast<float>(exciterFreqSlider.getValue()));
    else if (slider == &exciterDriveSlider)
        exciterProcessor.setSaturationDrive(static_cast<float>(exciterDriveSlider.getValue()));
    else if (slider == &exciterMixSlider)
        exciterProcessor.setDryWetMix(static_cast<float>(exciterMixSlider.getValue()));
    // Compressor
    else if (slider == &compThreshSlider)
        multibandCompressor.setAllThresholds(static_cast<float>(compThreshSlider.getValue()));
    else if (slider == &compRatioSlider)
        multibandCompressor.setAllRatios(static_cast<float>(compRatioSlider.getValue()));
    else if (slider == &compAttackSlider)
        multibandCompressor.setAllAttacks(static_cast<float>(compAttackSlider.getValue()));
    else if (slider == &compReleaseSlider)
        multibandCompressor.setAllReleases(static_cast<float>(compReleaseSlider.getValue()));
    // Nonlinear dynamics
    else if (slider == &couplingRateSlider)
        gongSynth.getEnergyAccumulator().setCouplingRate(static_cast<float>(couplingRateSlider.getValue()));
    else if (slider == &bloomThreshSlider)
        gongSynth.getEnergyAccumulator().setBloomThreshold(static_cast<float>(bloomThreshSlider.getValue()));
    else if (slider == &glideDirectionSlider)
        gongSynth.getResonatorBank().setAllPitchGlideDirection(static_cast<float>(glideDirectionSlider.getValue()));
    else if (slider == &glideSensitivitySlider)
        gongSynth.getResonatorBank().setAllPitchGlideSensitivity(static_cast<float>(glideSensitivitySlider.getValue()));
    else if (slider == &postConvLowSlider)
        convolutionEngine.setPostConvLowGainDb(static_cast<float>(postConvLowSlider.getValue()));
    else if (slider == &postConvMidSlider)
        convolutionEngine.setPostConvMidGainDb(static_cast<float>(postConvMidSlider.getValue()));
    else if (slider == &postConvHighSlider)
        convolutionEngine.setPostConvHighGainDb(static_cast<float>(postConvHighSlider.getValue()));
    // Macro knobs
    else if (slider == &macroSliders[0])
        macroParameters.setMacro(0, static_cast<float>(macroSliders[0].getValue()));
    else if (slider == &macroSliders[1])
        macroParameters.setMacro(1, static_cast<float>(macroSliders[1].getValue()));
    else if (slider == &macroSliders[2])
        macroParameters.setMacro(2, static_cast<float>(macroSliders[2].getValue()));
    else if (slider == &macroSliders[3])
        macroParameters.setMacro(3, static_cast<float>(macroSliders[3].getValue()));
    // Volume
    else if (slider == &volumeSlider)
        masterGain = static_cast<float>(volumeSlider.getValue());
    else
    {
        // Per-resonator sliders
        for (int i = 0; i < kNumResonators; ++i)
        {
            auto& rc = resonatorControls[i];
            auto& resonator = gongSynth.getResonatorBank().getResonator(i);

            if (slider == &rc.freqSlider)
            {
                resonator.setFrequencyHz(static_cast<float>(rc.freqSlider.getValue()));
                updateResonatorFrequencyDisplay(i);
                return;
            }
            else if (slider == &rc.gainSlider)
            {
                resonator.setGainDb(static_cast<float>(rc.gainSlider.getValue()));
                return;
            }
            else if (slider == &rc.brightnessModSlider)
            {
                resonator.setBrightnessEnergyAmount(static_cast<float>(rc.brightnessModSlider.getValue()));
                return;
            }
            else if (slider == &rc.spreadModSlider)
            {
                resonator.setSpreadLevelEnergyAmount(static_cast<float>(rc.spreadModSlider.getValue()));
                return;
            }
            else if (slider == &rc.detuneModSlider)
            {
                resonator.setSpreadDetuneEnergyAmount(static_cast<float>(rc.detuneModSlider.getValue()));
                return;
            }
            else if (slider == &rc.panModSlider)
            {
                resonator.setPanWidthEnergyAmount(static_cast<float>(rc.panModSlider.getValue()));
                return;
            }
        }
    }
}

void MainComponent::buttonClicked(juce::Button* button)
{
    if (button == &panicButton)
        gongSynth.panic();
    else if (button == &loadIRButton)
        loadIRFile();
    else if (button == &exciterEnableButton)
        exciterProcessor.setEnabled(exciterEnableButton.getToggleState());
    else if (button == &compEnableButton)
        multibandCompressor.setEnabled(compEnableButton.getToggleState());
    else if (button == &audioInputModeButton)
    {
        gongSynth.setExcitationMode(GongSynthesizer::ExcitationMode::AudioInput);
    }
    else if (button == &syntheticModeButton)
    {
        gongSynth.setExcitationMode(GongSynthesizer::ExcitationMode::SyntheticImpulse);
    }
    else if (button == &convSectionToggle)
    {
        bool on = convSectionToggle.getToggleState();
        diagnosticState.convState.store(on ? 0 : 2, std::memory_order_relaxed);
    }
    else if (button == &nlSectionToggle)
    {
        nlDynamicsEnabled.store(nlSectionToggle.getToggleState(), std::memory_order_relaxed);
    }
    else if (button == &macroSectionToggle)
    {
        macrosEnabled.store(macroSectionToggle.getToggleState(), std::memory_order_relaxed);
    }
    else if (button == &loadIRBButton)
    {
        auto* chooser = new juce::FileChooser("Select IR B file...", irDirectory, "*.wav;*.aif;*.aiff;*.flac");
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                convolutionEngine.loadImpulseResponseB(file);
                irBFileLabel.setText(file.getFileName(), juce::dontSendNotification);
            }
            delete chooser;
        });
    }
    else if (button == &diagnosticsButton)
    {
        if (!diagnosticWindow || !diagnosticWindow->isVisible())
            diagnosticWindow = std::make_unique<DiagnosticWindow>(diagnosticState);
        else
            diagnosticWindow->setVisible(true);
    }
    else if (button == &presetSaveButton)
    {
        auto name = presetManager.getCurrentPresetName();
        if (name.isEmpty())
            name = "Untitled";
        presetManager.savePreset(name, gongSynth, convolutionEngine, exciterProcessor, multibandCompressor, masterGain, &diagnosticState);
        updatePresetComboBox();
    }
    else if (button == &presetSaveAsButton)
    {
        auto callback = [this](const juce::String& name) {
            if (name.isNotEmpty())
            {
                presetManager.savePreset(name, gongSynth, convolutionEngine, exciterProcessor, multibandCompressor, masterGain, &diagnosticState);
                updatePresetComboBox();
            }
        };

        auto* alertWindow = new juce::AlertWindow("Save Preset As", "Enter preset name:", juce::MessageBoxIconType::QuestionIcon);
        alertWindow->addTextEditor("name", presetManager.getCurrentPresetName(), "Name:");
        alertWindow->addButton("Save", 1);
        alertWindow->addButton("Cancel", 0);

        alertWindow->enterModalState(true, juce::ModalCallbackFunction::create([alertWindow, callback](int result) {
            if (result == 1)
            {
                auto name = alertWindow->getTextEditorContents("name");
                callback(name);
            }
            delete alertWindow;
        }));
    }
    else
    {
        for (int i = 0; i < kNumResonators; ++i)
        {
            auto& rc = resonatorControls[i];
            auto& resonator = gongSynth.getResonatorBank().getResonator(i);

            if (button == &rc.enableButton)
            {
                resonator.setEnabled(rc.enableButton.getToggleState());
                return;
            }
            else if (button == &rc.freeModeButton)
            {
                resonator.setFrequencyMode(SpreadVoiceResonator::FrequencyMode::Free);
                rc.freqSlider.setVisible(true);
                rc.noteCombo.setVisible(false);
                updateResonatorFrequencyDisplay(i);
                return;
            }
            else if (button == &rc.snapModeButton)
            {
                resonator.setFrequencyMode(SpreadVoiceResonator::FrequencyMode::Snap);
                rc.freqSlider.setVisible(false);
                rc.noteCombo.setVisible(true);
                int note = rc.noteCombo.getSelectedId() - 1;
                resonator.setMidiNote(note);
                updateResonatorFrequencyDisplay(i);
                return;
            }
        }
    }
}

void MainComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &midiInputList)
    {
        setMidiInput(midiInputList.getSelectedItemIndex());
    }
    else if (comboBox == &modalTemplateCombo)
    {
        int id = modalTemplateCombo.getSelectedId();
        if (id == 1)
        {
            // Manual mode - no template
        }
        else
        {
            int templateIdx = id - 2;
            float fundamental = gongSynth.getResonatorBank().getResonator(0).getEffectiveFrequency();
            gongSynth.getResonatorBank().applyModalTemplate(templateIdx, fundamental);
            updateUIFromState();
        }
    }
    else if (comboBox == &presetComboBox)
    {
        auto selectedName = presetComboBox.getText();
        if (selectedName.isNotEmpty())
        {
            if (presetManager.loadPreset(selectedName, gongSynth, convolutionEngine,
                                          exciterProcessor, multibandCompressor, masterGain, &diagnosticState))
            {
                updateUIFromState();
                if (convolutionEngine.isLoaded())
                    irWaveform.setIR(convolutionEngine.getIRBuffer(), convolutionEngine.getIRSampleRate());
            }
        }
    }
    else
    {
        for (int i = 0; i < kNumResonators; ++i)
        {
            auto& rc = resonatorControls[i];
            if (comboBox == &rc.noteCombo)
            {
                int note = rc.noteCombo.getSelectedId() - 1;
                gongSynth.getResonatorBank().getResonator(i).setMidiNote(note);
                updateResonatorFrequencyDisplay(i);
                return;
            }
        }
    }
}

void MainComponent::loadIRFile()
{
    auto fileChooser = std::make_unique<juce::FileChooser>(
        "Select Impulse Response",
        irDirectory.exists() ? irDirectory : juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.aiff;*.aif;*.flac"
    );

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            if (convolutionEngine.loadImpulseResponse(file))
            {
                irFileLabel.setText(file.getFileName(), juce::dontSendNotification);
                irWaveform.setIR(convolutionEngine.getIRBuffer(), convolutionEngine.getIRSampleRate());
            }
            else
            {
                irFileLabel.setText("Load failed", juce::dontSendNotification);
            }
        }
    });

    static std::unique_ptr<juce::FileChooser> currentChooser;
    currentChooser = std::move(fileChooser);
}

void MainComponent::timerCallback()
{
    currentGlobalEnergy = gongSynth.getEnergyAccumulator().getNormalizedGlobalEnergy();

    if (diagnosticWindow)
        diagnosticWindow->refresh();

    repaint();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;

    midiCollector.reset(sampleRate);

    gongSynth.prepareToPlay(sampleRate, samplesPerBlockExpected);
    convolutionEngine.prepare(sampleRate, samplesPerBlockExpected);
    exciterProcessor.prepare(sampleRate, samplesPerBlockExpected);
    multibandCompressor.prepare(sampleRate, samplesPerBlockExpected);

    // Apply initial settings
    gongSynth.setGlobalDecayTime(static_cast<float>(globalDecaySlider.getValue()));
    gongSynth.setGlobalBrightness(static_cast<float>(globalBrightnessSlider.getValue()));
    gongSynth.setGlobalSpreadLevel(static_cast<float>(globalSpreadLevelSlider.getValue()));
    gongSynth.setGlobalSpreadPanWidth(static_cast<float>(globalSpreadPanWidthSlider.getValue()));

    convolutionEngine.setWetDryMix(static_cast<float>(reverbMixSlider.getValue()));

    exciterProcessor.setHighpassFrequency(static_cast<float>(exciterFreqSlider.getValue()));
    exciterProcessor.setSaturationDrive(static_cast<float>(exciterDriveSlider.getValue()));
    exciterProcessor.setDryWetMix(static_cast<float>(exciterMixSlider.getValue()));

    multibandCompressor.setAllThresholds(static_cast<float>(compThreshSlider.getValue()));
    multibandCompressor.setAllRatios(static_cast<float>(compRatioSlider.getValue()));
    multibandCompressor.setAllAttacks(static_cast<float>(compAttackSlider.getValue()));
    multibandCompressor.setAllReleases(static_cast<float>(compReleaseSlider.getValue()));

    synthBuffer.setSize(2, samplesPerBlockExpected);
    audioInputBuffer.setSize(2, samplesPerBlockExpected);
    testBuffer.setSize(2, samplesPerBlockExpected);

    testSignalGenerator.prepare(sampleRate);
    modulationBus.prepare(sampleRate);
    diagnosticState.budgetUs.store(static_cast<float>(1000000.0 * samplesPerBlockExpected / sampleRate),
                                    std::memory_order_relaxed);

    double budgetMs = 1000.0 * samplesPerBlockExpected / sampleRate;
    juce::Logger::writeToLog("AUDIO SETUP: sampleRate=" + juce::String(sampleRate)
        + " blockSize=" + juce::String(samplesPerBlockExpected)
        + " budget=" + juce::String(budgetMs, 2) + "ms");
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
    if (numInputChannels == 0)
        numInputChannels = 1;

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

    currentInputLevel = audioInputBuffer.getMagnitude(0, numSamples);

    // 1. Meter input
    diagnosticState.input.update(audioInputBuffer, 0, numSamples);

    // 2. Modulation
    modulationBus.computeBlock(diagnosticState, numSamples);
    applyModulationOffsets();

    // 3. Test signal injection helper
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

    // Inject at input point
    injectTestSignal(audioInputBuffer, 0);

    // Bypass/Solo/Mute logic
    bool anySoloed = diagnosticState.isAnySoloed();
    int synthEff = diagnosticState.getEffectiveState(
        diagnosticState.synthState.load(std::memory_order_relaxed), anySoloed);
    int convEff = diagnosticState.getEffectiveState(
        diagnosticState.convState.load(std::memory_order_relaxed), anySoloed);
    int exciterEff = diagnosticState.getEffectiveState(
        diagnosticState.exciterState.load(std::memory_order_relaxed), anySoloed);
    int compEff = diagnosticState.getEffectiveState(
        diagnosticState.compState.load(std::memory_order_relaxed), anySoloed);

    synthBuffer.clear();

    // 4. Synth
    auto t0 = std::chrono::steady_clock::now();

    if (synthEff != 2) // not bypassed
        gongSynth.process(synthBuffer, audioInputBuffer, midiBuffer, numSamples);
    if (synthEff == 1) // muted
        synthBuffer.clear(0, numSamples);

    auto t1 = std::chrono::steady_clock::now();
    auto synthUs = static_cast<float>(std::chrono::duration<double, std::micro>(t1 - t0).count());
    diagnosticState.synthTime.record(synthUs);
    diagnosticState.afterSynth.update(synthBuffer, 0, numSamples);

    // Copy synth output to main buffer
    for (int channel = 0; channel < static_cast<int>(numOutputChannels); ++channel)
        bufferToFill.buffer->copyFrom(channel, bufferToFill.startSample, synthBuffer, channel, 0, numSamples);

    juce::AudioBuffer<float> processBuffer(bufferToFill.buffer->getArrayOfWritePointers(),
                                           static_cast<int>(numOutputChannels),
                                           bufferToFill.startSample,
                                           numSamples);

    // Inject after synth
    injectTestSignal(processBuffer, 1);

    // 5. Convolution
    auto t2start = std::chrono::steady_clock::now();

    if (convEff != 2)
        convolutionEngine.process(processBuffer);
    if (convEff == 1)
        processBuffer.clear(0, numSamples);

    auto t2 = std::chrono::steady_clock::now();
    auto convUs = static_cast<float>(std::chrono::duration<double, std::micro>(t2 - t2start).count());
    diagnosticState.convTime.record(convUs);
    diagnosticState.afterConv.update(processBuffer, 0, numSamples);

    // Inject after conv
    injectTestSignal(processBuffer, 2);

    // 6. Exciter
    auto t3start = std::chrono::steady_clock::now();

    if (exciterEff != 2)
        exciterProcessor.process(processBuffer);
    if (exciterEff == 1)
        processBuffer.clear(0, numSamples);

    auto t3 = std::chrono::steady_clock::now();
    auto exciterUs = static_cast<float>(std::chrono::duration<double, std::micro>(t3 - t3start).count());
    diagnosticState.exciterTime.record(exciterUs);
    diagnosticState.afterExciter.update(processBuffer, 0, numSamples);

    // Inject after exciter
    injectTestSignal(processBuffer, 3);

    // 7. Compressor
    auto t4start = std::chrono::steady_clock::now();

    if (compEff != 2)
        multibandCompressor.process(processBuffer);
    if (compEff == 1)
        processBuffer.clear(0, numSamples);

    auto t4 = std::chrono::steady_clock::now();
    auto compUs = static_cast<float>(std::chrono::duration<double, std::micro>(t4 - t4start).count());
    diagnosticState.compTime.record(compUs);
    diagnosticState.afterComp.update(processBuffer, 0, numSamples);

    // 8. Master gain
    bufferToFill.buffer->applyGain(bufferToFill.startSample, numSamples, masterGain);

    // 9. Output metering
    diagnosticState.output.update(*bufferToFill.buffer, bufferToFill.startSample, numSamples);

    // 10. Total timing
    auto totalUs = static_cast<float>(std::chrono::duration<double, std::micro>(t4 - t0).count());
    diagnosticState.totalTimeUs.store(totalUs, std::memory_order_relaxed);

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

    // Convolution
    float convMixBase = static_cast<float>(reverbMixSlider.getValue());
    float convGainBase = static_cast<float>(convGainSlider.getValue());
    convolutionEngine.setWetDryMix(clamp(ModTarget::ConvMix, convMixBase));
    convolutionEngine.setOutputGainDb(clamp(ModTarget::ConvGain, convGainBase));

    // Exciter
    float excFreqBase = static_cast<float>(exciterFreqSlider.getValue());
    float excDriveBase = static_cast<float>(exciterDriveSlider.getValue());
    float excMixBase = static_cast<float>(exciterMixSlider.getValue());
    exciterProcessor.setHighpassFrequency(clamp(ModTarget::ExciterFreq, excFreqBase));
    exciterProcessor.setSaturationDrive(clamp(ModTarget::ExciterDrive, excDriveBase));
    exciterProcessor.setDryWetMix(clamp(ModTarget::ExciterMix, excMixBase));
    exciterProcessor.setOutputGainDb(clamp(ModTarget::ExciterOutputGain, exciterProcessor.getOutputGainDb()));

    // Compressor
    float compThreshBase = static_cast<float>(compThreshSlider.getValue());
    float compRatioBase = static_cast<float>(compRatioSlider.getValue());
    float compAttackBase = static_cast<float>(compAttackSlider.getValue());
    float compReleaseBase = static_cast<float>(compReleaseSlider.getValue());
    multibandCompressor.setAllThresholds(clamp(ModTarget::CompThreshold, compThreshBase));
    multibandCompressor.setAllRatios(clamp(ModTarget::CompRatio, compRatioBase));
    multibandCompressor.setAllAttacks(clamp(ModTarget::CompAttack, compAttackBase));
    multibandCompressor.setAllReleases(clamp(ModTarget::CompRelease, compReleaseBase));

    // Synth
    float decayBase = static_cast<float>(globalDecaySlider.getValue());
    float brightBase = static_cast<float>(globalBrightnessSlider.getValue());
    float spreadBase = static_cast<float>(globalSpreadLevelSlider.getValue());
    float panBase = static_cast<float>(globalSpreadPanWidthSlider.getValue());
    gongSynth.setGlobalDecayTime(clamp(ModTarget::SynthDecay, decayBase));
    gongSynth.setGlobalBrightness(clamp(ModTarget::SynthBrightness, brightBase));
    gongSynth.setGlobalSpreadLevel(clamp(ModTarget::SynthSpreadLevel, spreadBase));
    gongSynth.setGlobalSpreadPanWidth(clamp(ModTarget::SynthSpreadPanWidth, panBase));

    // Energy
    float eDecayBase = static_cast<float>(energyDecaySlider.getValue());
    float eInjBase = static_cast<float>(energyInjectionSlider.getValue());
    float ePowBase = static_cast<float>(energyPowerSlider.getValue());
    auto& ea = gongSynth.getEnergyAccumulator();
    ea.setGlobalDecayMs(clamp(ModTarget::EnergyDecayMs, eDecayBase));
    ea.setInjectionGain(clamp(ModTarget::EnergyInjectionGain, eInjBase));
    ea.setInjectionPower(clamp(ModTarget::EnergyInjectionPower, ePowBase));

    // Roll prime level (Step 4)
    float rollBase = ea.getRollPrimeLevel();
    ea.setRollPrimeLevel(clamp(ModTarget::RollPrimeLevel, rollBase));

    // Nonlinear dynamics (only when enabled)
    if (nlDynamicsEnabled.load(std::memory_order_relaxed))
    {
        // Pitch glide (Step 6) - modulation offsets
        float glideDir = static_cast<float>(glideDirectionSlider.getValue()) + get(ModTarget::PitchGlideDirection);
        float glideSens = static_cast<float>(glideSensitivitySlider.getValue()) + get(ModTarget::PitchGlideSensitivity);
        gongSynth.getResonatorBank().setAllPitchGlideDirection(juce::jlimit(-1.0f, 1.0f, glideDir));
        gongSynth.getResonatorBank().setAllPitchGlideSensitivity(juce::jlimit(0.0f, 200.0f, glideSens));
    }
    else
    {
        // Bypass: zero out nonlinear effects
        gongSynth.getResonatorBank().setAllPitchGlideSensitivity(0.0f);
        ea.setCouplingRate(0.0f);
        ea.setBloomThreshold(1.0f);
    }

    // Post-conv EQ (Step 8) - modulation offsets
    float postLow = static_cast<float>(postConvLowSlider.getValue()) + get(ModTarget::PostConvLowGain);
    float postMid = static_cast<float>(postConvMidSlider.getValue()) + get(ModTarget::PostConvMidGain);
    float postHigh = static_cast<float>(postConvHighSlider.getValue()) + get(ModTarget::PostConvHighGain);
    convolutionEngine.setPostConvLowGainDb(juce::jlimit(-12.0f, 12.0f, postLow));
    convolutionEngine.setPostConvMidGainDb(juce::jlimit(-12.0f, 12.0f, postMid));
    convolutionEngine.setPostConvHighGainDb(juce::jlimit(-12.0f, 12.0f, postHigh));

    // Step 14: IR crossfade driven by energy
    float globalEnergy = ea.getNormalizedGlobalEnergy();
    convolutionEngine.setIRCrossfadeEnergy(globalEnergy);

    // Step 15: Apply macro offsets (only when enabled)
    if (macrosEnabled.load(std::memory_order_relaxed))
    {
        ea.setInjectionGain(juce::jlimit(0.1f, 5.0f, ea.getInjectionGain() + macroParameters.getInjectionGainOffset()));
        ea.setBloomThreshold(juce::jlimit(0.1f, 1.0f, ea.getBloomThreshold() + macroParameters.getBloomThresholdOffset()));
        gongSynth.getCrashNoiseGenerator().setThreshold(
            juce::jlimit(0.1f, 1.0f, gongSynth.getCrashNoiseGenerator().getThreshold() + macroParameters.getCrashThresholdOffset()));
    }

    // Master gain
    float mgBase = static_cast<float>(volumeSlider.getValue());
    masterGain = clamp(ModTarget::MasterGain, mgBase);
}

void MainComponent::releaseResources()
{
    convolutionEngine.reset();
    exciterProcessor.reset();
    multibandCompressor.reset();
    gongSynth.releaseResources();
}

// === PerformanceServer::Listener Implementation ===

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
            irWaveform.setIR(convolutionEngine.getIRBuffer(), convolutionEngine.getIRSampleRate());
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
    if (!irDirectory.exists())
        return false;

    auto irFile = irDirectory.getChildFile(name);
    if (!irFile.existsAsFile())
        return false;

    if (convolutionEngine.loadImpulseResponse(irFile))
    {
        irFileLabel.setText(irFile.getFileName(), juce::dontSendNotification);
        irWaveform.setIR(convolutionEngine.getIRBuffer(), convolutionEngine.getIRSampleRate());
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

// === Paint & Layout ===

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Title
    g.setFont(juce::FontOptions(20.0f).withStyle("Bold"));
    g.setColour(juce::Colours::white);
    g.drawText("GONG ENERGY SYNTHESIZER", getLocalBounds().removeFromTop(35), juce::Justification::centred);

    // Section definitions: bounds, title, description, enabled
    struct SectionDraw {
        juce::Rectangle<int> bounds;
        const char* title;
        const char* description;
        bool enabled;
    };

    SectionDraw sections[] = {
        { inputSectionBounds,     "INPUT / STRIKE DETECTION",
          "Mic/line input -> strike detection -> energy injection. Controls how audio triggers the energy system.", true },
        { energySectionBounds,    "ENERGY ACCUMULATOR",
          "Leaky integrator: strikes inject energy that decays over time and modulates resonator behavior.", true },
        { resonatorSectionBounds, "RESONATOR BANK (4 x 7 voices)",
          "4 modal resonators with 7 spread voices each. Energy modulates brightness, spread, detuning, and stereo width.", true },
        { convSectionBounds,      "CONVOLUTION REVERB",
          "IR-based reverb with dual-IR energy crossfade and post-convolution 3-band EQ for tonal shaping.",
          convSectionToggle.getToggleState() },
        { nlSectionBounds,        "NONLINEAR DYNAMICS",
          "Inter-band energy coupling, power-law bloom cascade, pitch glide under stress -- nonlinear gong physics.",
          nlSectionToggle.getToggleState() },
        { outputSectionBounds,    "OUTPUT PROCESSING",
          "Harmonic exciter (HP filter + saturation) and 3-band multiband compressor for final shaping.", true },
        { macroSectionBounds,     "MACRO CONTROLS",
          "4 high-level knobs (Size, Material, Intensity, Space) controlling multiple parameters simultaneously.",
          macroSectionToggle.getToggleState() },
        { presetSectionBounds,    "PRESETS", "", true },
    };

    for (auto& s : sections)
    {
        if (s.bounds.isEmpty()) continue;

        float alpha = s.enabled ? 1.0f : 0.4f;

        // Background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(s.bounds.toFloat(), 5.0f);
        g.setColour(s.enabled ? juce::Colour(0xff444444) : juce::Colour(0xff333333));
        g.drawRoundedRectangle(s.bounds.toFloat().reduced(0.5f), 5.0f, 1.0f);

        // Title
        auto titleRect = s.bounds.withHeight(22).reduced(8, 2);
        g.setColour(juce::Colours::white.withAlpha(0.9f * alpha));
        g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
        g.drawText(s.title, titleRect, juce::Justification::centredLeft);

        // Description
        if (s.description[0] != '\0')
        {
            auto descRect = s.bounds.reduced(8, 0);
            descRect.removeFromTop(20);
            g.setColour(juce::Colours::white.withAlpha(0.35f * alpha));
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(s.description, descRect.removeFromTop(14), juce::Justification::centredLeft);
        }
    }

    // Energy meter (top-right of energy section)
    if (!energySectionBounds.isEmpty())
    {
        auto meterRect = juce::Rectangle<int>(energySectionBounds.getRight() - 130,
                                               energySectionBounds.getY() + 4, 110, 14);
        g.setColour(juce::Colours::black);
        g.fillRect(meterRect);
        g.setColour(juce::Colour(0xffff8800));
        g.fillRect(meterRect.withWidth(static_cast<int>(currentGlobalEnergy * meterRect.getWidth())));
        g.setColour(juce::Colours::grey);
        g.drawRect(meterRect);
    }

    // Status lines at bottom
    auto statusArea = getLocalBounds().removeFromBottom(42);
    g.setFont(juce::FontOptions(10.0f));

    auto debugLine = statusArea.removeFromTop(18);
    juce::String debugText = "LEVELS - In: " + juce::String(diagnosticState.input.rmsL.load(std::memory_order_relaxed), 3)
        + " | Synth: " + juce::String(diagnosticState.afterSynth.rmsL.load(std::memory_order_relaxed), 3)
        + " | Conv: " + juce::String(diagnosticState.afterConv.rmsL.load(std::memory_order_relaxed), 3)
        + " | Exc: " + juce::String(diagnosticState.afterExciter.rmsL.load(std::memory_order_relaxed), 3)
        + " | Comp: " + juce::String(diagnosticState.afterComp.rmsL.load(std::memory_order_relaxed), 3)
        + " | Out: " + juce::String(diagnosticState.output.rmsL.load(std::memory_order_relaxed), 3);
    g.setColour(juce::Colours::yellow);
    g.drawText(debugText, debugLine, juce::Justification::centred, true);

    g.setColour(juce::Colours::lightgrey);
    juce::String statusText = "Energy: " + juce::String(static_cast<int>(currentGlobalEnergy * 100)) + "%"
        + "  |  IR: " + (convolutionEngine.isLoaded() ? convolutionEngine.getIRFileName() : juce::String("None"))
        + "  |  Mode: " + (gongSynth.getExcitationMode() == GongSynthesizer::ExcitationMode::AudioInput ? "Audio" : "Synthetic")
        + "  |  Preset: " + (presetManager.getCurrentPresetName().isEmpty() ? "None" : presetManager.getCurrentPresetName());

    g.drawText(statusText, statusArea, juce::Justification::centred, true);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(35); // Title

    // MIDI row + excitation mode
    auto midiRow = area.removeFromTop(28);
    midiInputListLabel.setBounds(midiRow.removeFromLeft(40));
    midiInputList.setBounds(midiRow.removeFromLeft(200));
    midiRow.removeFromLeft(30);
    audioInputModeButton.setBounds(midiRow.removeFromLeft(90));
    syntheticModeButton.setBounds(midiRow.removeFromLeft(90));

    area.removeFromTop(6);

    // Helper: title(20) + description(14) + gap(4) = 38px reserved at top of each section
    constexpr int sectionHeaderH = 38;

    // === INPUT SECTION === (38 header + 28 controls = 66 + padding)
    inputSectionBounds = area.removeFromTop(70);
    {
        auto s = inputSectionBounds.reduced(8, 0);
        s.removeFromTop(sectionHeaderH);
        auto row = s.removeFromTop(28);

        inputGainLabel.setBounds(row.removeFromLeft(65));
        inputGainSlider.setBounds(row.removeFromLeft(120));
        row.removeFromLeft(15);
        strikeThreshLabel.setBounds(row.removeFromLeft(80));
        strikeThreshSlider.setBounds(row.removeFromLeft(120));
        row.removeFromLeft(15);
        strikeHoldoffLabel.setBounds(row.removeFromLeft(55));
        strikeHoldoffSlider.setBounds(row.removeFromLeft(120));
    }

    area.removeFromTop(6);

    // === ENERGY SECTION ===
    energySectionBounds = area.removeFromTop(70);
    {
        auto s = energySectionBounds.reduced(8, 0);
        s.removeFromTop(sectionHeaderH);
        auto row = s.removeFromTop(28);

        energyDecayLabel.setBounds(row.removeFromLeft(45));
        energyDecaySlider.setBounds(row.removeFromLeft(120));
        row.removeFromLeft(15);
        energyInjectionLabel.setBounds(row.removeFromLeft(60));
        energyInjectionSlider.setBounds(row.removeFromLeft(100));
        row.removeFromLeft(15);
        energyPowerLabel.setBounds(row.removeFromLeft(45));
        energyPowerSlider.setBounds(row.removeFromLeft(100));
    }

    area.removeFromTop(6);

    // === RESONATOR SECTION ===
    resonatorSectionBounds = area.removeFromTop(310);
    {
        auto s = resonatorSectionBounds.reduced(8, 0);
        s.removeFromTop(sectionHeaderH);

        auto globalRow = s.removeFromTop(26);
        globalDecayLabel.setBounds(globalRow.removeFromLeft(45));
        globalDecaySlider.setBounds(globalRow.removeFromLeft(100));
        globalRow.removeFromLeft(15);
        globalBrightnessLabel.setBounds(globalRow.removeFromLeft(65));
        globalBrightnessSlider.setBounds(globalRow.removeFromLeft(100));
        globalRow.removeFromLeft(15);
        globalSpreadLevelLabel.setBounds(globalRow.removeFromLeft(65));
        globalSpreadLevelSlider.setBounds(globalRow.removeFromLeft(100));
        globalRow.removeFromLeft(15);
        globalSpreadPanWidthLabel.setBounds(globalRow.removeFromLeft(65));
        globalSpreadPanWidthSlider.setBounds(globalRow.removeFromLeft(100));

        s.removeFromTop(6);

        auto headerRow = s.removeFromTop(18);
        int col1 = 25, col2 = 25, col3 = 75, col4 = 130, col5 = 55, col6 = 55, col7 = 55, col8 = 55, col9 = 55;
        int colGap = 8;

        resHeaderOn.setBounds(headerRow.removeFromLeft(col1 + col2));
        headerRow.removeFromLeft(colGap);
        resHeaderMode.setBounds(headerRow.removeFromLeft(col3));
        headerRow.removeFromLeft(colGap);
        resHeaderFreq.setBounds(headerRow.removeFromLeft(col4));
        headerRow.removeFromLeft(colGap);
        resHeaderGain.setBounds(headerRow.removeFromLeft(col5));
        headerRow.removeFromLeft(colGap);
        resHeaderBright.setBounds(headerRow.removeFromLeft(col6));
        headerRow.removeFromLeft(colGap);
        resHeaderSpread.setBounds(headerRow.removeFromLeft(col7));
        headerRow.removeFromLeft(colGap);
        resHeaderDetune.setBounds(headerRow.removeFromLeft(col8));
        headerRow.removeFromLeft(colGap);
        resHeaderPan.setBounds(headerRow.removeFromLeft(col9));

        s.removeFromTop(4);

        for (int i = 0; i < kNumResonators; ++i)
        {
            auto& rc = resonatorControls[i];
            auto resRow = s.removeFromTop(50);

            rc.nameLabel.setBounds(resRow.removeFromLeft(col1));
            rc.enableButton.setBounds(resRow.removeFromLeft(col2).reduced(2));
            resRow.removeFromLeft(colGap);

            auto modeArea = resRow.removeFromLeft(col3);
            rc.freeModeButton.setBounds(modeArea.removeFromTop(24));
            rc.snapModeButton.setBounds(modeArea.removeFromTop(24));
            resRow.removeFromLeft(colGap);

            auto freqArea = resRow.removeFromLeft(col4);
            auto freqControlArea = freqArea.removeFromTop(28);
            rc.freqSlider.setBounds(freqControlArea);
            rc.noteCombo.setBounds(freqControlArea);
            rc.freqValueLabel.setBounds(freqArea.removeFromTop(18));
            resRow.removeFromLeft(colGap);

            rc.gainSlider.setBounds(resRow.removeFromLeft(col5).withHeight(22));
            resRow.removeFromLeft(colGap);
            rc.brightnessModSlider.setBounds(resRow.removeFromLeft(col6).withHeight(22));
            resRow.removeFromLeft(colGap);
            rc.spreadModSlider.setBounds(resRow.removeFromLeft(col7).withHeight(22));
            resRow.removeFromLeft(colGap);
            rc.detuneModSlider.setBounds(resRow.removeFromLeft(col8).withHeight(22));
            resRow.removeFromLeft(colGap);
            rc.panModSlider.setBounds(resRow.removeFromLeft(col9).withHeight(22));
        }
    }

    area.removeFromTop(6);

    // === CONVOLUTION SECTION === (includes post-conv EQ and IR B)
    convSectionBounds = area.removeFromTop(170);
    {
        auto s = convSectionBounds.reduced(8, 0);
        // Toggle in title bar
        convSectionToggle.setBounds(convSectionBounds.getRight() - 50, convSectionBounds.getY() + 2, 40, 18);

        s.removeFromTop(sectionHeaderH);

        irWaveform.setBounds(s.removeFromTop(52).reduced(0, 2));
        s.removeFromTop(2);

        auto controlRow = s.removeFromTop(28);
        loadIRButton.setBounds(controlRow.removeFromLeft(80));
        controlRow.removeFromLeft(10);
        irFileLabel.setBounds(controlRow.removeFromLeft(160));
        controlRow.removeFromLeft(15);
        reverbMixLabel.setBounds(controlRow.removeFromLeft(30));
        reverbMixSlider.setBounds(controlRow.removeFromLeft(100));
        controlRow.removeFromLeft(15);
        convGainLabel.setBounds(controlRow.removeFromLeft(35));
        convGainSlider.setBounds(controlRow.removeFromLeft(100));

        s.removeFromTop(4);

        auto eqRow = s.removeFromTop(28);
        postConvLowLabel.setBounds(eqRow.removeFromLeft(45));
        postConvLowSlider.setBounds(eqRow.removeFromLeft(80));
        eqRow.removeFromLeft(8);
        postConvMidLabel.setBounds(eqRow.removeFromLeft(45));
        postConvMidSlider.setBounds(eqRow.removeFromLeft(80));
        eqRow.removeFromLeft(8);
        postConvHighLabel.setBounds(eqRow.removeFromLeft(50));
        postConvHighSlider.setBounds(eqRow.removeFromLeft(80));
        eqRow.removeFromLeft(15);
        loadIRBButton.setBounds(eqRow.removeFromLeft(80));
        eqRow.removeFromLeft(5);
        irBFileLabel.setBounds(eqRow.removeFromLeft(120));
    }

    area.removeFromTop(6);

    // === NONLINEAR DYNAMICS SECTION ===
    nlSectionBounds = area.removeFromTop(105);
    {
        auto s = nlSectionBounds.reduced(8, 0);
        // Toggle in title bar
        nlSectionToggle.setBounds(nlSectionBounds.getRight() - 50, nlSectionBounds.getY() + 2, 40, 18);

        s.removeFromTop(sectionHeaderH);

        auto row1 = s.removeFromTop(28);
        couplingRateLabel.setBounds(row1.removeFromLeft(55));
        couplingRateSlider.setBounds(row1.removeFromLeft(100));
        row1.removeFromLeft(15);
        bloomThreshLabel.setBounds(row1.removeFromLeft(45));
        bloomThreshSlider.setBounds(row1.removeFromLeft(100));
        row1.removeFromLeft(15);
        glideDirectionLabel.setBounds(row1.removeFromLeft(60));
        glideDirectionSlider.setBounds(row1.removeFromLeft(100));
        row1.removeFromLeft(15);
        glideSensitivityLabel.setBounds(row1.removeFromLeft(65));
        glideSensitivitySlider.setBounds(row1.removeFromLeft(100));

        s.removeFromTop(4);

        auto row2 = s.removeFromTop(28);
        modalTemplateLabel.setBounds(row2.removeFromLeft(60));
        modalTemplateCombo.setBounds(row2.removeFromLeft(150));
    }

    area.removeFromTop(6);

    // === OUTPUT PROCESSING SECTION ===
    outputSectionBounds = area.removeFromTop(105);
    {
        auto s = outputSectionBounds.reduced(8, 0);
        s.removeFromTop(sectionHeaderH);

        auto exciterRow = s.removeFromTop(28);
        exciterEnableButton.setBounds(exciterRow.removeFromLeft(70));
        exciterRow.removeFromLeft(15);
        exciterFreqLabel.setBounds(exciterRow.removeFromLeft(50));
        exciterFreqSlider.setBounds(exciterRow.removeFromLeft(100));
        exciterRow.removeFromLeft(15);
        exciterDriveLabel.setBounds(exciterRow.removeFromLeft(40));
        exciterDriveSlider.setBounds(exciterRow.removeFromLeft(80));
        exciterRow.removeFromLeft(15);
        exciterMixLabel.setBounds(exciterRow.removeFromLeft(30));
        exciterMixSlider.setBounds(exciterRow.removeFromLeft(80));

        s.removeFromTop(4);

        auto compRow = s.removeFromTop(28);
        compEnableButton.setBounds(compRow.removeFromLeft(90));
        compRow.removeFromLeft(15);
        compThreshLabel.setBounds(compRow.removeFromLeft(45));
        compThreshSlider.setBounds(compRow.removeFromLeft(80));
        compRow.removeFromLeft(10);
        compRatioLabel.setBounds(compRow.removeFromLeft(40));
        compRatioSlider.setBounds(compRow.removeFromLeft(65));
        compRow.removeFromLeft(10);
        compAttackLabel.setBounds(compRow.removeFromLeft(30));
        compAttackSlider.setBounds(compRow.removeFromLeft(65));
        compRow.removeFromLeft(10);
        compReleaseLabel.setBounds(compRow.removeFromLeft(30));
        compReleaseSlider.setBounds(compRow.removeFromLeft(65));
    }

    area.removeFromTop(6);

    // === MACRO SECTION ===
    macroSectionBounds = area.removeFromTop(70);
    {
        auto s = macroSectionBounds.reduced(8, 0);
        // Toggle in title bar
        macroSectionToggle.setBounds(macroSectionBounds.getRight() - 50, macroSectionBounds.getY() + 2, 40, 18);

        s.removeFromTop(sectionHeaderH);

        auto row = s.removeFromTop(28);
        for (int i = 0; i < 4; ++i)
        {
            macroLabels[i].setBounds(row.removeFromLeft(55));
            macroSliders[i].setBounds(row.removeFromLeft(100));
            row.removeFromLeft(15);
        }
    }

    area.removeFromTop(6);

    // === PRESET SECTION ===
    presetSectionBounds = area.removeFromTop(42);
    {
        auto s = presetSectionBounds.reduced(8, 0);
        s.removeFromTop(10);
        auto row = s.removeFromTop(26);
        presetLabel.setBounds(row.removeFromLeft(50));
        presetComboBox.setBounds(row.removeFromLeft(200));
        row.removeFromLeft(10);
        presetSaveButton.setBounds(row.removeFromLeft(60).withHeight(24));
        row.removeFromLeft(5);
        presetSaveAsButton.setBounds(row.removeFromLeft(80).withHeight(24));
    }

    area.removeFromTop(6);

    // === BOTTOM CONTROLS ===
    auto bottomRow = area.removeFromTop(35);
    volumeLabel.setBounds(bottomRow.removeFromLeft(50));
    volumeSlider.setBounds(bottomRow.removeFromLeft(200));
    bottomRow.removeFromLeft(20);
    panicButton.setBounds(bottomRow.removeFromRight(80).withHeight(28));
    bottomRow.removeFromRight(10);
    diagnosticsButton.setBounds(bottomRow.removeFromRight(100).withHeight(28));
}
