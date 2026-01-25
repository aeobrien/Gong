#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(900, 850);

    // MIDI input selector
    addAndMakeVisible(midiInputListLabel);
    midiInputListLabel.setText("MIDI:", juce::dontSendNotification);

    addAndMakeVisible(midiInputList);
    midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs");
    midiInputList.addListener(this);

    updateMidiDeviceList();
    if (midiDevices.size() > 0)
        setMidiInput(0);

    // Excitation source radio buttons
    addAndMakeVisible(midiImpulseButton);
    midiImpulseButton.setRadioGroupId(1);
    midiImpulseButton.setToggleState(true, juce::dontSendNotification);
    midiImpulseButton.addListener(this);

    addAndMakeVisible(audioInputButton);
    audioInputButton.setRadioGroupId(1);
    audioInputButton.addListener(this);

    // Audio input frequency control
    addAndMakeVisible(audioInputFreqLabel);
    addAndMakeVisible(audioInputFreqSlider);
    audioInputFreqSlider.setRange(20.0, 2000.0, 1.0);
    audioInputFreqSlider.setValue(220.0);
    audioInputFreqSlider.setSkewFactorFromMidPoint(220.0);
    audioInputFreqSlider.setTextValueSuffix(" Hz");
    audioInputFreqSlider.addListener(this);
    // Initially hidden (shown when audio input mode is selected)
    audioInputFreqLabel.setVisible(false);
    audioInputFreqSlider.setVisible(false);

    // Filter controls
    addAndMakeVisible(filterCutoffLabel);
    addAndMakeVisible(filterCutoffSlider);
    filterCutoffSlider.setRange(100.0, 15000.0, 1.0);
    filterCutoffSlider.setValue(5000.0);
    filterCutoffSlider.setSkewFactorFromMidPoint(2000.0);
    filterCutoffSlider.setTextValueSuffix(" Hz");
    filterCutoffSlider.addListener(this);

    addAndMakeVisible(filterResoLabel);
    addAndMakeVisible(filterResoSlider);
    filterResoSlider.setRange(0.1, 5.0, 0.01);
    filterResoSlider.setValue(0.707);
    filterResoSlider.addListener(this);

    addAndMakeVisible(filterTypeLabel);
    addAndMakeVisible(filterTypeCombo);
    filterTypeCombo.addItem("Lowpass", 1);
    filterTypeCombo.addItem("Highpass", 2);
    filterTypeCombo.addItem("Bandpass", 3);
    filterTypeCombo.setSelectedId(1);
    filterTypeCombo.addListener(this);

    addAndMakeVisible(velocityCurveLabel);
    addAndMakeVisible(velocityCurveSlider);
    velocityCurveSlider.setRange(0.25, 3.0, 0.01);
    velocityCurveSlider.setValue(1.0);
    velocityCurveSlider.addListener(this);

    // Envelope Follower controls
    addAndMakeVisible(envFollowerEnableButton);
    envFollowerEnableButton.setToggleState(false, juce::dontSendNotification);
    envFollowerEnableButton.addListener(this);

    addAndMakeVisible(envAttackLabel);
    addAndMakeVisible(envAttackSlider);
    envAttackSlider.setRange(1.0, 200.0, 1.0);
    envAttackSlider.setValue(10.0);
    envAttackSlider.setTextValueSuffix(" ms");
    envAttackSlider.addListener(this);

    addAndMakeVisible(envReleaseLabel);
    addAndMakeVisible(envReleaseSlider);
    envReleaseSlider.setRange(10.0, 1000.0, 1.0);
    envReleaseSlider.setValue(100.0);
    envReleaseSlider.setTextValueSuffix(" ms");
    envReleaseSlider.addListener(this);

    addAndMakeVisible(envTargetLabel);
    addAndMakeVisible(envTargetCombo);
    envTargetCombo.addItem("Velocity", 1);
    envTargetCombo.addItem("Filter Cutoff", 2);
    envTargetCombo.addItem("Decay", 3);
    envTargetCombo.addItem("Brightness", 4);
    envTargetCombo.addItem("Reverb Mix", 5);
    envTargetCombo.setSelectedId(1);
    envTargetCombo.addListener(this);

    addAndMakeVisible(envAmountLabel);
    addAndMakeVisible(envAmountSlider);
    envAmountSlider.setRange(0.0, 1.0, 0.01);
    envAmountSlider.setValue(0.5);
    envAmountSlider.addListener(this);

    // Resonator global controls
    addAndMakeVisible(decayLabel);
    addAndMakeVisible(decaySlider);
    decaySlider.setRange(0.5, 15.0, 0.1);
    decaySlider.setValue(4.0);
    decaySlider.setTextValueSuffix(" s");
    decaySlider.addListener(this);

    addAndMakeVisible(brightnessLabel);
    addAndMakeVisible(brightnessSlider);
    brightnessSlider.setRange(0.0, 1.0, 0.01);
    brightnessSlider.setValue(0.6);
    brightnessSlider.addListener(this);

    addAndMakeVisible(resonatorGainLabel);
    addAndMakeVisible(resonatorGainSlider);
    resonatorGainSlider.setRange(-24.0, 24.0, 0.1);
    resonatorGainSlider.setValue(0.0);
    resonatorGainSlider.setTextValueSuffix(" dB");
    resonatorGainSlider.addListener(this);

    // Per-mode column headers
    addAndMakeVisible(modeHeaderLabel);
    modeHeaderLabel.setJustificationType(juce::Justification::centred);
    modeHeaderLabel.setFont(juce::FontOptions(11.0f));

    addAndMakeVisible(freqHeaderLabel);
    freqHeaderLabel.setJustificationType(juce::Justification::centred);
    freqHeaderLabel.setFont(juce::FontOptions(11.0f));

    addAndMakeVisible(decayHeaderLabel);
    decayHeaderLabel.setJustificationType(juce::Justification::centred);
    decayHeaderLabel.setFont(juce::FontOptions(11.0f));

    addAndMakeVisible(gainHeaderLabel);
    gainHeaderLabel.setJustificationType(juce::Justification::centred);
    gainHeaderLabel.setFont(juce::FontOptions(11.0f));

    addAndMakeVisible(detuneHeaderLabel);
    detuneHeaderLabel.setJustificationType(juce::Justification::centred);
    detuneHeaderLabel.setFont(juce::FontOptions(11.0f));

    addAndMakeVisible(widthHeaderLabel);
    widthHeaderLabel.setJustificationType(juce::Justification::centred);
    widthHeaderLabel.setFont(juce::FontOptions(11.0f));

    addAndMakeVisible(enableHeaderLabel);
    enableHeaderLabel.setJustificationType(juce::Justification::centred);
    enableHeaderLabel.setFont(juce::FontOptions(11.0f));

    addAndMakeVisible(ratioHeaderLabel);
    ratioHeaderLabel.setJustificationType(juce::Justification::centred);
    ratioHeaderLabel.setFont(juce::FontOptions(11.0f));

    // Per-mode controls
    const float baseRatios[kNumModes] = { 1.0f, 2.76f, 5.40f, 8.93f, 13.34f, 18.64f, 24.82f, 31.87f };

    for (int i = 0; i < kNumModes; ++i)
    {
        auto& mc = modeControls[i];

        mc.nameLabel.setText("M" + juce::String(i + 1), juce::dontSendNotification);
        addAndMakeVisible(mc.nameLabel);

        mc.freqSlider.setRange(0.5, 2.0, 0.01);
        mc.freqSlider.setValue(1.0);
        mc.freqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        mc.freqSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        mc.freqSlider.addListener(this);
        addAndMakeVisible(mc.freqSlider);

        mc.decaySlider.setRange(0.1, 3.0, 0.01);
        mc.decaySlider.setValue(1.0);
        mc.decaySlider.setSliderStyle(juce::Slider::LinearHorizontal);
        mc.decaySlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        mc.decaySlider.addListener(this);
        addAndMakeVisible(mc.decaySlider);

        mc.gainSlider.setRange(-24.0, 6.0, 0.1);
        mc.gainSlider.setValue(0.0);
        mc.gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        mc.gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        mc.gainSlider.addListener(this);
        addAndMakeVisible(mc.gainSlider);

        mc.detuneSlider.setRange(-100.0, 100.0, 1.0);
        mc.detuneSlider.setValue(0.0);
        mc.detuneSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        mc.detuneSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        mc.detuneSlider.addListener(this);
        addAndMakeVisible(mc.detuneSlider);

        mc.widthSlider.setRange(0.0, 1.0, 0.01);
        mc.widthSlider.setValue(0.0);
        mc.widthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        mc.widthSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        mc.widthSlider.addListener(this);
        addAndMakeVisible(mc.widthSlider);

        mc.enableButton.setToggleState(i < 6, juce::dontSendNotification);
        mc.enableButton.addListener(this);
        addAndMakeVisible(mc.enableButton);

        mc.ratioLabel.setText(juce::String(baseRatios[i], 2) + "x", juce::dontSendNotification);
        mc.ratioLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(mc.ratioLabel);
    }

    // Convolution section
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

    // Bottom controls
    addAndMakeVisible(volumeLabel);
    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.8);
    volumeSlider.addListener(this);

    addAndMakeVisible(panicButton);
    panicButton.addListener(this);

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

    // Start timer for UI updates
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    shutdownAudio();

    for (auto& device : midiDevices)
    {
        deviceManager.setMidiInputDeviceEnabled(device.identifier, false);
    }
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

void MainComponent::handleIncomingMidiMessage(juce::MidiInput* /*source*/, const juce::MidiMessage& message)
{
    midiCollector.addMessageToQueue(message);

    if (message.isNoteOn())
    {
        lastPlayedNote = message.getNoteNumber();
    }
}

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &decaySlider)
    {
        synth.setDecayTime(static_cast<float>(decaySlider.getValue()));
    }
    else if (slider == &brightnessSlider)
    {
        synth.setBrightness(static_cast<float>(brightnessSlider.getValue()));
    }
    else if (slider == &reverbMixSlider)
    {
        convolutionEngine.setWetDryMix(static_cast<float>(reverbMixSlider.getValue()));
    }
    else if (slider == &volumeSlider)
    {
        masterGain = static_cast<float>(volumeSlider.getValue());
    }
    else if (slider == &audioInputFreqSlider)
    {
        synth.setAudioInputFundamental(static_cast<float>(audioInputFreqSlider.getValue()));
    }
    else if (slider == &resonatorGainSlider)
    {
        synth.setMasterGainDb(static_cast<float>(resonatorGainSlider.getValue()));
    }
    else if (slider == &convGainSlider)
    {
        convolutionEngine.setOutputGainDb(static_cast<float>(convGainSlider.getValue()));
    }
    else if (slider == &filterCutoffSlider)
    {
        synth.setFilterCutoff(static_cast<float>(filterCutoffSlider.getValue()));
    }
    else if (slider == &filterResoSlider)
    {
        synth.setFilterResonance(static_cast<float>(filterResoSlider.getValue()));
    }
    else if (slider == &velocityCurveSlider)
    {
        synth.setVelocityCurve(static_cast<float>(velocityCurveSlider.getValue()));
    }
    else if (slider == &envAttackSlider)
    {
        envelopeFollower.setAttackMs(static_cast<float>(envAttackSlider.getValue()));
    }
    else if (slider == &envReleaseSlider)
    {
        envelopeFollower.setReleaseMs(static_cast<float>(envReleaseSlider.getValue()));
    }
    else if (slider == &envAmountSlider)
    {
        envFollowerAmount = static_cast<float>(envAmountSlider.getValue());
    }
    else
    {
        // Check per-mode sliders
        for (int i = 0; i < kNumModes; ++i)
        {
            auto& mc = modeControls[i];
            if (slider == &mc.freqSlider)
            {
                synth.setModeFrequencyRatio(i, static_cast<float>(mc.freqSlider.getValue()));
                return;
            }
            else if (slider == &mc.decaySlider)
            {
                synth.setModeDecayMultiplier(i, static_cast<float>(mc.decaySlider.getValue()));
                return;
            }
            else if (slider == &mc.gainSlider)
            {
                synth.setModeGainDb(i, static_cast<float>(mc.gainSlider.getValue()));
                return;
            }
            else if (slider == &mc.detuneSlider)
            {
                synth.setModeDetuneCents(i, static_cast<float>(mc.detuneSlider.getValue()));
                return;
            }
            else if (slider == &mc.widthSlider)
            {
                synth.setModeStereoWidth(i, static_cast<float>(mc.widthSlider.getValue()));
                return;
            }
        }
    }
}

void MainComponent::buttonClicked(juce::Button* button)
{
    if (button == &panicButton)
    {
        synth.panic();
    }
    else if (button == &midiImpulseButton)
    {
        synth.setExcitationSource(ImpulseGenerator::ExcitationSource::InternalNoise);
        audioInputFreqLabel.setVisible(false);
        audioInputFreqSlider.setVisible(false);
        velocityCurveLabel.setVisible(true);
        velocityCurveSlider.setVisible(true);
    }
    else if (button == &audioInputButton)
    {
        synth.setExcitationSource(ImpulseGenerator::ExcitationSource::AudioInput);
        audioInputFreqLabel.setVisible(true);
        audioInputFreqSlider.setVisible(true);
        velocityCurveLabel.setVisible(false);
        velocityCurveSlider.setVisible(false);
    }
    else if (button == &loadIRButton)
    {
        loadIRFile();
    }
    else if (button == &envFollowerEnableButton)
    {
        envelopeFollower.setEnabled(envFollowerEnableButton.getToggleState());
    }
    else
    {
        // Check per-mode enable buttons
        for (int i = 0; i < kNumModes; ++i)
        {
            if (button == &modeControls[i].enableButton)
            {
                synth.setModeEnabled(i, modeControls[i].enableButton.getToggleState());
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
    else if (comboBox == &filterTypeCombo)
    {
        int typeId = filterTypeCombo.getSelectedId();
        ImpulseGenerator::FilterType type = ImpulseGenerator::FilterType::Lowpass;
        if (typeId == 2) type = ImpulseGenerator::FilterType::Highpass;
        else if (typeId == 3) type = ImpulseGenerator::FilterType::Bandpass;
        synth.setFilterType(type);
    }
    else if (comboBox == &envTargetCombo)
    {
        envFollowerTarget = envTargetCombo.getSelectedId() - 1;
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

    // Keep the chooser alive
    static std::unique_ptr<juce::FileChooser> currentChooser;
    currentChooser = std::move(fileChooser);
}

void MainComponent::updateEnvelopeFollowerTarget()
{
    if (!envFollowerEnableButton.getToggleState())
        return;

    float level = envelopeFollower.getCurrentLevel();
    float modAmount = level * envFollowerAmount;

    switch (envFollowerTarget)
    {
        case 0: // Velocity - handled in audio callback
            break;
        case 1: // Filter Cutoff
        {
            float baseCutoff = static_cast<float>(filterCutoffSlider.getValue());
            float modCutoff = baseCutoff * (1.0f + modAmount * 2.0f);
            synth.setFilterCutoff(juce::jlimit(100.0f, 15000.0f, modCutoff));
            break;
        }
        case 2: // Decay
        {
            float baseDecay = static_cast<float>(decaySlider.getValue());
            synth.setDecayTime(baseDecay * (1.0f - modAmount * 0.5f));
            break;
        }
        case 3: // Brightness
        {
            float baseBrightness = static_cast<float>(brightnessSlider.getValue());
            synth.setBrightness(juce::jlimit(0.0f, 1.0f, baseBrightness + modAmount * 0.5f));
            break;
        }
        case 4: // Reverb Mix
        {
            float baseMix = static_cast<float>(reverbMixSlider.getValue());
            convolutionEngine.setWetDryMix(juce::jlimit(0.0f, 1.0f, baseMix + modAmount * 0.3f));
            break;
        }
    }
}

void MainComponent::timerCallback()
{
    currentInputLevel = envelopeFollower.getCurrentLevel();
    updateEnvelopeFollowerTarget();
    repaint();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;

    midiCollector.reset(sampleRate);

    synth.prepareToPlay(sampleRate, samplesPerBlockExpected);
    synth.setDecayTime(static_cast<float>(decaySlider.getValue()));
    synth.setBrightness(static_cast<float>(brightnessSlider.getValue()));
    synth.setNumModes(kNumModes);
    synth.setFilterCutoff(static_cast<float>(filterCutoffSlider.getValue()));
    synth.setFilterResonance(static_cast<float>(filterResoSlider.getValue()));

    synthBuffer.setSize(2, samplesPerBlockExpected);
    audioInputBuffer.setSize(2, samplesPerBlockExpected);

    convolutionEngine.prepare(sampleRate, samplesPerBlockExpected);
    convolutionEngine.setWetDryMix(static_cast<float>(reverbMixSlider.getValue()));

    envelopeFollower.prepare(sampleRate);

    // Try to load default IR
    auto resourcesDir = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                            .getParentDirectory()
                            .getChildFile("resources");
    auto irFile = resourcesDir.getChildFile("default_ir.wav");

    if (irFile.existsAsFile())
    {
        if (convolutionEngine.loadImpulseResponse(irFile))
        {
            irFileLabel.setText(irFile.getFileName(), juce::dontSendNotification);
            irWaveform.setIR(convolutionEngine.getIRBuffer(), convolutionEngine.getIRSampleRate());
        }
    }
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

    // Get actual number of input channels from the device
    auto activeInputChannels = device->getActiveInputChannels();
    int numInputChannels = activeInputChannels.countNumberOfSetBits();
    if (numInputChannels == 0)
        numInputChannels = 1; // Fallback to at least 1

    // Ensure buffers are correct size (always stereo for processing)
    if (synthBuffer.getNumChannels() != numOutputChannels || synthBuffer.getNumSamples() < numSamples)
        synthBuffer.setSize(numOutputChannels, numSamples, false, false, true);
    if (audioInputBuffer.getNumChannels() < 2 || audioInputBuffer.getNumSamples() < numSamples)
        audioInputBuffer.setSize(2, numSamples, false, false, true);

    // Get audio input for envelope follower and possible audio excitation
    auto* inputChannelData = bufferToFill.buffer->getArrayOfReadPointers();

    // Copy input channel 0 to our buffer
    audioInputBuffer.copyFrom(0, 0, inputChannelData[0] + bufferToFill.startSample, numSamples);

    // If mono input, duplicate to channel 1; otherwise copy channel 1
    if (numInputChannels == 1 || numOutputChannels == 1)
    {
        // Mono input - duplicate to right channel
        audioInputBuffer.copyFrom(1, 0, audioInputBuffer, 0, 0, numSamples);
    }
    else
    {
        // Stereo input - copy right channel
        audioInputBuffer.copyFrom(1, 0, inputChannelData[1] + bufferToFill.startSample, numSamples);
    }

    // Process envelope follower
    envelopeFollower.process(audioInputBuffer);

    // Clear synth buffer
    synthBuffer.clear();

    // Process synth
    if (synth.getExcitationSource() == ImpulseGenerator::ExcitationSource::AudioInput)
    {
        synth.processWithAudioInput(synthBuffer, audioInputBuffer, midiBuffer, numSamples);
    }
    else
    {
        synth.renderNextBlock(synthBuffer, midiBuffer, 0, numSamples);
    }

    // Copy to output
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        bufferToFill.buffer->copyFrom(channel, bufferToFill.startSample,
                                       synthBuffer, channel, 0, numSamples);
    }

    // Apply convolution reverb
    juce::AudioBuffer<float> processBuffer(bufferToFill.buffer->getArrayOfWritePointers(),
                                           numOutputChannels,
                                           bufferToFill.startSample,
                                           numSamples);
    convolutionEngine.process(processBuffer);

    // Apply master volume
    bufferToFill.buffer->applyGain(bufferToFill.startSample, numSamples, masterGain);
}

void MainComponent::releaseResources()
{
    convolutionEngine.reset();
    envelopeFollower.reset();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    auto area = getLocalBounds();

    // Title
    auto titleArea = area.removeFromTop(40);
    g.setFont(juce::FontOptions(22.0f));
    g.setColour(juce::Colours::white);
    g.drawText("GONG CONVOLUTION REVERB", titleArea, juce::Justification::centred, true);

    // Section headers
    g.setFont(juce::FontOptions(12.0f));
    g.setColour(juce::Colours::grey);

    // Draw section borders
    auto contentArea = getLocalBounds().reduced(10);
    contentArea.removeFromTop(40); // Skip title

    // MIDI/Audio Input row
    contentArea.removeFromTop(35);

    // Excitation section
    auto excitationArea = contentArea.removeFromTop(65);
    g.setColour(juce::Colours::darkgrey);
    g.drawRoundedRectangle(excitationArea.toFloat().reduced(2), 4.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawText("EXCITATION", excitationArea.removeFromTop(16).reduced(8, 0), juce::Justification::centredLeft);

    // Envelope follower section
    auto envArea = contentArea.removeFromTop(60);
    g.setColour(juce::Colours::darkgrey);
    g.drawRoundedRectangle(envArea.toFloat().reduced(2), 4.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawText("ENVELOPE FOLLOWER", envArea.removeFromTop(16).reduced(8, 0), juce::Justification::centredLeft);

    // Draw level meter
    auto meterArea = juce::Rectangle<int>(envArea.getRight() - 160, envArea.getY() + 20, 100, 12);
    g.setColour(juce::Colours::black);
    g.fillRect(meterArea);
    g.setColour(juce::Colours::green);
    int meterWidth = static_cast<int>(currentInputLevel * meterArea.getWidth());
    g.fillRect(meterArea.withWidth(meterWidth));

    // Resonator section
    auto resonatorArea = contentArea.removeFromTop(295);
    g.setColour(juce::Colours::darkgrey);
    g.drawRoundedRectangle(resonatorArea.toFloat().reduced(2), 4.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawText("RESONATOR", resonatorArea.removeFromTop(16).reduced(8, 0), juce::Justification::centredLeft);

    // Convolution section
    auto convArea = contentArea.removeFromTop(140);
    g.setColour(juce::Colours::darkgrey);
    g.drawRoundedRectangle(convArea.toFloat().reduced(2), 4.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawText("CONVOLUTION", convArea.removeFromTop(16).reduced(8, 0), juce::Justification::centredLeft);

    // Status line
    auto statusArea = getLocalBounds().removeFromBottom(25);
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(juce::Colours::lightgrey);

    float freq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(lastPlayedNote));
    juce::String statusText = "Last: " + juce::MidiMessage::getMidiNoteName(lastPlayedNote, true, true, 4)
        + " (" + juce::String(static_cast<int>(freq)) + " Hz)";
    statusText += " | Voices: " + juce::String(synth.getNumVoices());
    statusText += " | IR: " + (convolutionEngine.isLoaded() ? convolutionEngine.getIRFileName() : juce::String("None"));
    statusText += " | Input: " + juce::String(static_cast<int>(currentInputLevel * 100)) + "%";

    g.drawText(statusText, statusArea, juce::Justification::centred, true);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(40); // Title

    // MIDI/Audio Input row
    auto midiRow = area.removeFromTop(30);
    midiInputListLabel.setBounds(midiRow.removeFromLeft(40));
    midiInputList.setBounds(midiRow.removeFromLeft(200));

    area.removeFromTop(5);

    // Excitation Section
    auto excitationArea = area.removeFromTop(65).reduced(5);
    excitationArea.removeFromTop(16); // Header
    auto excitRow1 = excitationArea.removeFromTop(22);
    midiImpulseButton.setBounds(excitRow1.removeFromLeft(110));
    audioInputButton.setBounds(excitRow1.removeFromLeft(100));
    excitRow1.removeFromLeft(20);
    audioInputFreqLabel.setBounds(excitRow1.removeFromLeft(60));
    audioInputFreqSlider.setBounds(excitRow1.removeFromLeft(150));

    auto excitRow2 = excitationArea.removeFromTop(22);
    filterCutoffLabel.setBounds(excitRow2.removeFromLeft(45));
    filterCutoffSlider.setBounds(excitRow2.removeFromLeft(120));
    filterResoLabel.setBounds(excitRow2.removeFromLeft(35));
    filterResoSlider.setBounds(excitRow2.removeFromLeft(70));
    filterTypeLabel.setBounds(excitRow2.removeFromLeft(35));
    filterTypeCombo.setBounds(excitRow2.removeFromLeft(90));
    velocityCurveLabel.setBounds(excitRow2.removeFromLeft(55));
    velocityCurveSlider.setBounds(excitRow2.removeFromLeft(80));

    area.removeFromTop(5);

    // Envelope Follower Section
    auto envArea = area.removeFromTop(60).reduced(5);
    envArea.removeFromTop(16); // Header
    auto envRow = envArea.removeFromTop(30);
    envFollowerEnableButton.setBounds(envRow.removeFromLeft(70));
    envAttackLabel.setBounds(envRow.removeFromLeft(45));
    envAttackSlider.setBounds(envRow.removeFromLeft(90));
    envReleaseLabel.setBounds(envRow.removeFromLeft(50));
    envReleaseSlider.setBounds(envRow.removeFromLeft(90));
    envTargetLabel.setBounds(envRow.removeFromLeft(45));
    envTargetCombo.setBounds(envRow.removeFromLeft(100));
    envAmountLabel.setBounds(envRow.removeFromLeft(50));
    envAmountSlider.setBounds(envRow.removeFromLeft(80));

    area.removeFromTop(5);

    // Resonator Section
    auto resonatorArea = area.removeFromTop(295).reduced(5);
    resonatorArea.removeFromTop(16); // Header

    // Global controls
    auto globalRow = resonatorArea.removeFromTop(25);
    decayLabel.setBounds(globalRow.removeFromLeft(40));
    decaySlider.setBounds(globalRow.removeFromLeft(120));
    globalRow.removeFromLeft(10);
    brightnessLabel.setBounds(globalRow.removeFromLeft(70));
    brightnessSlider.setBounds(globalRow.removeFromLeft(120));
    globalRow.removeFromLeft(10);
    resonatorGainLabel.setBounds(globalRow.removeFromLeft(55));
    resonatorGainSlider.setBounds(globalRow.removeFromLeft(120));

    // Column headers for per-mode controls
    auto headerRow = resonatorArea.removeFromTop(20);
    modeHeaderLabel.setBounds(headerRow.removeFromLeft(30));
    freqHeaderLabel.setBounds(headerRow.removeFromLeft(60));
    decayHeaderLabel.setBounds(headerRow.removeFromLeft(60));
    gainHeaderLabel.setBounds(headerRow.removeFromLeft(60));
    detuneHeaderLabel.setBounds(headerRow.removeFromLeft(60));
    widthHeaderLabel.setBounds(headerRow.removeFromLeft(60));
    enableHeaderLabel.setBounds(headerRow.removeFromLeft(30));
    ratioHeaderLabel.setBounds(headerRow.removeFromLeft(60));

    resonatorArea.removeFromTop(5); // Small gap between headers and sliders

    // Per-mode controls
    int rowHeight = 25;
    for (int i = 0; i < kNumModes; ++i)
    {
        auto& mc = modeControls[i];
        auto modeRow = resonatorArea.removeFromTop(rowHeight);

        mc.nameLabel.setBounds(modeRow.removeFromLeft(30));
        mc.freqSlider.setBounds(modeRow.removeFromLeft(60));
        mc.decaySlider.setBounds(modeRow.removeFromLeft(60));
        mc.gainSlider.setBounds(modeRow.removeFromLeft(60));
        mc.detuneSlider.setBounds(modeRow.removeFromLeft(60));
        mc.widthSlider.setBounds(modeRow.removeFromLeft(60));
        mc.enableButton.setBounds(modeRow.removeFromLeft(30));
        mc.ratioLabel.setBounds(modeRow.removeFromLeft(60));
    }

    area.removeFromTop(5);

    // Convolution Section
    auto convArea = area.removeFromTop(140).reduced(5);
    convArea.removeFromTop(16); // Header

    irWaveform.setBounds(convArea.removeFromTop(80).reduced(5));

    auto convControlRow = convArea.removeFromTop(30);
    loadIRButton.setBounds(convControlRow.removeFromLeft(80));
    convControlRow.removeFromLeft(10);
    irFileLabel.setBounds(convControlRow.removeFromLeft(150));
    convControlRow.removeFromLeft(10);
    reverbMixLabel.setBounds(convControlRow.removeFromLeft(30));
    reverbMixSlider.setBounds(convControlRow.removeFromLeft(100));
    convControlRow.removeFromLeft(10);
    convGainLabel.setBounds(convControlRow.removeFromLeft(65));
    convGainSlider.setBounds(convControlRow.removeFromLeft(100));

    area.removeFromTop(5);

    // Bottom controls
    auto bottomRow = area.removeFromTop(40);
    volumeLabel.setBounds(bottomRow.removeFromLeft(50));
    volumeSlider.setBounds(bottomRow.removeFromLeft(200));
    bottomRow.removeFromLeft(20);
    panicButton.setBounds(bottomRow.removeFromRight(80).withHeight(30));
}
