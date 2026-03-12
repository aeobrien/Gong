#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(1000, 950);

    // MIDI input selector
    addAndMakeVisible(midiInputListLabel);
    midiInputListLabel.setText("MIDI:", juce::dontSendNotification);

    addAndMakeVisible(midiInputList);
    midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs");
    midiInputList.addListener(this);

    updateMidiDeviceList();
    if (midiDevices.size() > 0)
        setMidiInput(0);

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

        // Frequency slider (Free mode) - visible by default
        rc.freqSlider.setRange(20.0, 2000.0, 1.0);
        rc.freqSlider.setSkewFactorFromMidPoint(220.0);
        rc.freqSlider.setValue(defaultFreqs[i]);
        rc.freqSlider.setTextValueSuffix(" Hz");
        rc.freqSlider.addListener(this);
        rc.freqSlider.setVisible(true);  // Explicitly visible
        addAndMakeVisible(rc.freqSlider);

        // MIDI note combo (Snap mode) - hidden by default
        for (int note = 24; note <= 96; ++note)
        {
            int octave = (note / 12) - 1;
            int noteIndex = note % 12;
            juce::String noteName = juce::String(noteNames[noteIndex]) + juce::String(octave);
            rc.noteCombo.addItem(noteName, note + 1);
        }
        rc.noteCombo.setSelectedId(defaultNotes[i] + 1, juce::dontSendNotification);
        rc.noteCombo.addListener(this);
        addChildComponent(rc.noteCombo);  // Use addChildComponent - doesn't make visible

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

    // === BOTTOM CONTROLS ===
    addAndMakeVisible(volumeLabel);
    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.8);
    volumeSlider.addListener(this);

    addAndMakeVisible(panicButton);
    panicButton.addListener(this);

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

    startTimerHz(30);
}

void MainComponent::setupTooltips()
{
    // Input Section
    inputGainSlider.setTooltip("Amplifies incoming audio before strike detection and resonator excitation");
    strikeThreshSlider.setTooltip("Audio level threshold that triggers energy injection (lower = more sensitive)");
    strikeHoldoffSlider.setTooltip("Minimum time between strike detections to prevent retriggering");

    // Energy Section
    energyDecaySlider.setTooltip("How long accumulated energy takes to fade out (longer = sustained brightness)");
    energyInjectionSlider.setTooltip("How much energy is added per strike (higher = faster energy buildup)");
    energyPowerSlider.setTooltip("Shapes the velocity-to-energy curve (1.0=linear, >1=emphasize loud hits, <1=emphasize soft hits)");

    // Global Resonator Controls
    globalDecaySlider.setTooltip("How long resonators ring after excitation (filter Q derived from this)");
    globalBrightnessSlider.setTooltip("Base tonal brightness before energy modulation (affects filter Q)");
    globalSpreadLevelSlider.setTooltip("Level of the 6 spread voices relative to center voice");
    globalSpreadPanWidthSlider.setTooltip("Stereo width of spread voices (0=mono, 1=full stereo)");

    // Resonator controls tooltips
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

    // Convolution Section
    loadIRButton.setTooltip("Load an impulse response WAV file for convolution reverb");
    reverbMixSlider.setTooltip("Dry/wet mix for convolution reverb (0=dry, 1=wet)");
    convGainSlider.setTooltip("Output gain for convolution reverb");

    // Exciter Section
    exciterEnableButton.setTooltip("Enable harmonic exciter (adds brightness via saturation)");
    exciterFreqSlider.setTooltip("Highpass frequency - only frequencies above this are excited");
    exciterDriveSlider.setTooltip("Saturation amount (higher = more harmonics)");
    exciterMixSlider.setTooltip("Dry/wet mix for exciter (0=dry, 1=wet)");

    // Compressor Section
    compEnableButton.setTooltip("Enable 3-band multiband compressor");
    compThreshSlider.setTooltip("Level above which compression begins");
    compRatioSlider.setTooltip("Compression ratio (4:1 means 4dB over threshold becomes 1dB)");
    compAttackSlider.setTooltip("How fast compressor responds to transients");
    compReleaseSlider.setTooltip("How fast compressor releases after signal drops");

    // Master
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

MainComponent::~MainComponent()
{
    stopTimer();
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
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
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

    // Auto-loading disabled for debugging - use Load IR button instead
    // auto sourceDir = juce::File(__FILE__).getParentDirectory().getParentDirectory();
    // auto irFile = sourceDir.getChildFile("IRs").getChildFile("Gong1.wav");
    // if (irFile.existsAsFile())
    // {
    //     if (convolutionEngine.loadImpulseResponse(irFile))
    //     {
    //         irFileLabel.setText(irFile.getFileName(), juce::dontSendNotification);
    //         irWaveform.setIR(convolutionEngine.getIRBuffer(), convolutionEngine.getIRSampleRate());
    //     }
    // }
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

    auto* inputChannelData = bufferToFill.buffer->getArrayOfReadPointers();
    audioInputBuffer.copyFrom(0, 0, inputChannelData[0] + bufferToFill.startSample, numSamples);

    if (numInputChannels == 1 || numOutputChannels == 1)
        audioInputBuffer.copyFrom(1, 0, audioInputBuffer, 0, 0, numSamples);
    else
        audioInputBuffer.copyFrom(1, 0, inputChannelData[1] + bufferToFill.startSample, numSamples);

    currentInputLevel = audioInputBuffer.getMagnitude(0, numSamples);

    synthBuffer.clear();

    // Process gong synthesizer
    gongSynth.process(synthBuffer, audioInputBuffer, midiBuffer, numSamples);

    // DEBUG: Track level after synth
    debugLevelAfterSynth = synthBuffer.getMagnitude(0, 0, numSamples);

    // Copy to output FIRST (matching original working code)
    for (int channel = 0; channel < static_cast<int>(numOutputChannels); ++channel)
        bufferToFill.buffer->copyFrom(channel, bufferToFill.startSample, synthBuffer, channel, 0, numSamples);

    // Apply convolution on output buffer (matching original working code)
    juce::AudioBuffer<float> processBuffer(bufferToFill.buffer->getArrayOfWritePointers(),
                                           static_cast<int>(numOutputChannels),
                                           bufferToFill.startSample,
                                           numSamples);

    convolutionEngine.process(processBuffer);
    debugLevelAfterConv = processBuffer.getMagnitude(0, 0, numSamples);

    exciterProcessor.process(processBuffer);
    debugLevelAfterExciter = processBuffer.getMagnitude(0, 0, numSamples);

    multibandCompressor.process(processBuffer);
    debugLevelAfterComp = processBuffer.getMagnitude(0, 0, numSamples);

    // Apply master gain
    bufferToFill.buffer->applyGain(bufferToFill.startSample, numSamples, masterGain);
    debugLevelFinal = bufferToFill.buffer->getMagnitude(0, bufferToFill.startSample, numSamples);
}

void MainComponent::releaseResources()
{
    convolutionEngine.reset();
    exciterProcessor.reset();
    multibandCompressor.reset();
    gongSynth.releaseResources();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto area = getLocalBounds();
    int margin = 10;

    // Title
    auto titleArea = area.removeFromTop(35);
    g.setFont(juce::FontOptions(20.0f).withStyle("Bold"));
    g.setColour(juce::Colours::white);
    g.drawText("GONG ENERGY SYNTHESIZER", titleArea, juce::Justification::centred, true);

    auto contentArea = getLocalBounds().reduced(margin);
    contentArea.removeFromTop(35);

    // MIDI row
    contentArea.removeFromTop(32);

    // Helper to draw section boxes
    auto drawSection = [&](juce::Rectangle<int> bounds, const juce::String& title) {
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
        g.setColour(juce::Colour(0xff444444));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 5.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
        g.drawText(title, bounds.removeFromTop(20).reduced(8, 2), juce::Justification::centredLeft);
    };

    // Input Section
    auto inputArea = contentArea.removeFromTop(55);
    drawSection(inputArea, "INPUT / STRIKE DETECTION");

    contentArea.removeFromTop(5);

    // Energy Section
    auto energyArea = contentArea.removeFromTop(55);
    drawSection(energyArea, "ENERGY ACCUMULATOR");

    // Draw energy meter
    auto meterRect = juce::Rectangle<int>(energyArea.getRight() - 120, energyArea.getY() + 25, 100, 14);
    g.setColour(juce::Colours::black);
    g.fillRect(meterRect);
    g.setColour(juce::Colour(0xffff8800));
    g.fillRect(meterRect.withWidth(static_cast<int>(currentGlobalEnergy * meterRect.getWidth())));
    g.setColour(juce::Colours::grey);
    g.drawRect(meterRect);

    contentArea.removeFromTop(5);

    // Resonator Section
    auto resonatorArea = contentArea.removeFromTop(300);
    drawSection(resonatorArea, "RESONATOR BANK (4 resonators x 7 voices each)");

    contentArea.removeFromTop(5);

    // Convolution Section
    auto convArea = contentArea.removeFromTop(120);
    drawSection(convArea, "CONVOLUTION REVERB");

    contentArea.removeFromTop(5);

    // Output Processing Section
    auto outputArea = contentArea.removeFromTop(85);
    drawSection(outputArea, "OUTPUT PROCESSING (Exciter + 3-Band Compressor)");

    // Status lines at bottom
    auto statusArea = getLocalBounds().removeFromBottom(40);
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(juce::Colours::lightgrey);

    // Debug signal chain levels
    auto debugLine = statusArea.removeFromTop(18);
    juce::String debugText = "DEBUG LEVELS - In: " + juce::String(currentInputLevel, 3)
        + " | Synth: " + juce::String(debugLevelAfterSynth, 3)
        + " | Conv: " + juce::String(debugLevelAfterConv, 3)
        + " | Exc: " + juce::String(debugLevelAfterExciter, 3)
        + " | Comp: " + juce::String(debugLevelAfterComp, 3)
        + " | Out: " + juce::String(debugLevelFinal, 3);
    g.setColour(juce::Colours::yellow);
    g.drawText(debugText, debugLine, juce::Justification::centred, true);

    // Regular status line
    g.setColour(juce::Colours::lightgrey);
    float freq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(lastPlayedNote));
    juce::String statusText = "Energy: " + juce::String(static_cast<int>(currentGlobalEnergy * 100)) + "%"
        + "  |  IR: " + (convolutionEngine.isLoaded() ? convolutionEngine.getIRFileName() : juce::String("None"));

    g.drawText(statusText, statusArea, juce::Justification::centred, true);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(35); // Title

    // MIDI row
    auto midiRow = area.removeFromTop(28);
    midiInputListLabel.setBounds(midiRow.removeFromLeft(40));
    midiInputList.setBounds(midiRow.removeFromLeft(200));

    area.removeFromTop(4);

    // === INPUT SECTION ===
    auto inputArea = area.removeFromTop(55).reduced(5, 0);
    inputArea.removeFromTop(22); // Header
    auto inputRow = inputArea.removeFromTop(28);

    inputGainLabel.setBounds(inputRow.removeFromLeft(65));
    inputGainSlider.setBounds(inputRow.removeFromLeft(110));
    inputRow.removeFromLeft(15);
    strikeThreshLabel.setBounds(inputRow.removeFromLeft(80));
    strikeThreshSlider.setBounds(inputRow.removeFromLeft(110));
    inputRow.removeFromLeft(15);
    strikeHoldoffLabel.setBounds(inputRow.removeFromLeft(55));
    strikeHoldoffSlider.setBounds(inputRow.removeFromLeft(110));

    area.removeFromTop(5);

    // === ENERGY SECTION ===
    auto energyArea = area.removeFromTop(55).reduced(5, 0);
    energyArea.removeFromTop(22); // Header
    auto energyRow = energyArea.removeFromTop(28);

    energyDecayLabel.setBounds(energyRow.removeFromLeft(45));
    energyDecaySlider.setBounds(energyRow.removeFromLeft(120));
    energyRow.removeFromLeft(15);
    energyInjectionLabel.setBounds(energyRow.removeFromLeft(55));
    energyInjectionSlider.setBounds(energyRow.removeFromLeft(90));
    energyRow.removeFromLeft(15);
    energyPowerLabel.setBounds(energyRow.removeFromLeft(45));
    energyPowerSlider.setBounds(energyRow.removeFromLeft(90));

    area.removeFromTop(5);

    // === RESONATOR SECTION ===
    auto resonatorArea = area.removeFromTop(300).reduced(5, 0);
    resonatorArea.removeFromTop(22); // Header

    // Global controls row
    auto globalRow = resonatorArea.removeFromTop(26);
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

    resonatorArea.removeFromTop(8);

    // Column headers
    auto headerRow = resonatorArea.removeFromTop(18);
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

    resonatorArea.removeFromTop(4);

    // Per-resonator rows
    for (int i = 0; i < kNumResonators; ++i)
    {
        auto& rc = resonatorControls[i];
        auto resRow = resonatorArea.removeFromTop(50);

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

    area.removeFromTop(5);

    // === CONVOLUTION SECTION ===
    auto convArea = area.removeFromTop(120).reduced(5, 0);
    convArea.removeFromTop(22); // Header

    irWaveform.setBounds(convArea.removeFromTop(60).reduced(0, 2));

    auto convControlRow = convArea.removeFromTop(28);
    loadIRButton.setBounds(convControlRow.removeFromLeft(80));
    convControlRow.removeFromLeft(10);
    irFileLabel.setBounds(convControlRow.removeFromLeft(180));
    convControlRow.removeFromLeft(20);
    reverbMixLabel.setBounds(convControlRow.removeFromLeft(30));
    reverbMixSlider.setBounds(convControlRow.removeFromLeft(100));
    convControlRow.removeFromLeft(15);
    convGainLabel.setBounds(convControlRow.removeFromLeft(35));
    convGainSlider.setBounds(convControlRow.removeFromLeft(100));

    area.removeFromTop(5);

    // === OUTPUT PROCESSING SECTION ===
    auto outputArea = area.removeFromTop(85).reduced(5, 0);
    outputArea.removeFromTop(22); // Header

    // Exciter row
    auto exciterRow = outputArea.removeFromTop(28);
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

    outputArea.removeFromTop(4);

    // Compressor row
    auto compRow = outputArea.removeFromTop(28);
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

    area.removeFromTop(5);

    // === BOTTOM CONTROLS ===
    auto bottomRow = area.removeFromTop(35);
    volumeLabel.setBounds(bottomRow.removeFromLeft(50));
    volumeSlider.setBounds(bottomRow.removeFromLeft(200));
    bottomRow.removeFromLeft(20);
    panicButton.setBounds(bottomRow.removeFromRight(80).withHeight(28));
}
