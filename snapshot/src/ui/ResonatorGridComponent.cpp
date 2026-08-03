#include "ResonatorGridComponent.h"
#include "../ResonatorBank.h"
#include "../SpreadVoiceResonator.h"

ResonatorGridComponent::ResonatorGridComponent()
{
    // Master knobs
    masterLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    masterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff9800));
    addAndMakeVisible(masterLabel);

    auto setupMasterKnob = [this](juce::Slider& s, double min, double max, double def, const juce::String& tip) {
        s.setRange(min, max, 0.01);
        s.setValue(def);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s.setTooltip(tip);
        s.onValueChange = [this] { applyMasterToAll(); };
        addAndMakeVisible(s);
    };

    setupMasterKnob(masterGainSlider, -24.0, 12.0, 0.0, "Master gain for all 4 resonators");
    setupMasterKnob(masterBrightSlider, 0.0, 1.0, 0.5, "Master brightness energy mod for all resonators");
    setupMasterKnob(masterSpreadSlider, 0.0, 1.0, 0.3, "Master spread energy mod for all resonators");
    setupMasterKnob(masterDetuneSlider, 0.0, 1.0, 0.2, "Master detune energy mod for all resonators");
    setupMasterKnob(masterPanSlider, 0.0, 1.0, 0.3, "Master pan width energy mod for all resonators");

    // Column headers
    static const char* headerNames[] = { "On", "Mode", "Frequency", "Gain", "Bright", "Spread", "Detune", "Pan" };
    for (int i = 0; i < 8; ++i)
    {
        headerLabels[i].setText(headerNames[i], juce::dontSendNotification);
        headerLabels[i].setFont(juce::FontOptions(10.0f));
        headerLabels[i].setJustificationType(juce::Justification::centred);
        headerLabels[i].setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
        addAndMakeVisible(headerLabels[i]);
    }

    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    float defaultFreqs[] = { 110.0f, 220.0f, 330.0f, 440.0f };
    int defaultNotes[] = { 45, 57, 64, 69 };

    for (int i = 0; i < kNumResonators; ++i)
    {
        auto& row = rows[i];

        row.nameLabel.setText("R" + juce::String(i + 1), juce::dontSendNotification);
        row.nameLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        row.nameLabel.setColour(juce::Label::textColourId, juce::Colour(0xff2196f3));
        addAndMakeVisible(row.nameLabel);

        row.enableButton.setToggleState(true, juce::dontSendNotification);
        row.enableButton.addListener(this);
        addAndMakeVisible(row.enableButton);

        row.freeModeButton.setRadioGroupId(100 + i);
        row.freeModeButton.setToggleState(true, juce::dontSendNotification);
        row.freeModeButton.addListener(this);
        addAndMakeVisible(row.freeModeButton);

        row.snapModeButton.setRadioGroupId(100 + i);
        row.snapModeButton.addListener(this);
        addAndMakeVisible(row.snapModeButton);

        row.freqSlider.setRange(20.0, 2000.0, 1.0);
        row.freqSlider.setSkewFactorFromMidPoint(220.0);
        row.freqSlider.setValue(defaultFreqs[i]);
        row.freqSlider.setTextValueSuffix(" Hz");
        row.freqSlider.addListener(this);
        addAndMakeVisible(row.freqSlider);

        for (int note = 24; note <= 96; ++note)
        {
            int octave = (note / 12) - 1;
            int noteIdx = note % 12;
            row.noteCombo.addItem(juce::String(noteNames[noteIdx]) + juce::String(octave), note + 1);
        }
        row.noteCombo.setSelectedId(defaultNotes[i] + 1, juce::dontSendNotification);
        row.noteCombo.addListener(this);
        addChildComponent(row.noteCombo);

        row.freqValueLabel.setFont(juce::FontOptions(9.0f));
        row.freqValueLabel.setJustificationType(juce::Justification::centred);
        row.freqValueLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
        addAndMakeVisible(row.freqValueLabel);

        row.gainSlider.setRange(-24.0, 12.0, 0.1);
        row.gainSlider.setValue(0.0);
        row.gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        row.gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        row.gainSlider.setTooltip("Resonator output level (-24 to +12 dB)");
        row.gainSlider.addListener(this);
        addAndMakeVisible(row.gainSlider);

        auto setupModKnob = [this](juce::Slider& s, float def, const juce::String& tip) {
            s.setRange(0.0, 1.0, 0.01);
            s.setValue(def);
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            s.setTooltip(tip);
            s.addListener(this);
            addAndMakeVisible(s);
        };

        setupModKnob(row.brightnessModSlider, 0.5f, "Energy -> brightness modulation amount");
        setupModKnob(row.spreadModSlider, 0.3f, "Energy -> spread level modulation amount");
        setupModKnob(row.detuneModSlider, 0.2f, "Energy -> detune modulation amount");
        setupModKnob(row.panModSlider, 0.3f, "Energy -> pan width modulation amount");
    }
}

void ResonatorGridComponent::setResonatorBank(ResonatorBank* bank)
{
    resonatorBank = bank;
    if (bank != nullptr)
        updateFromState();
}

void ResonatorGridComponent::refresh()
{
    if (resonatorBank == nullptr) return;
    for (int i = 0; i < kNumResonators; ++i)
        updateFreqDisplay(i);
}

void ResonatorGridComponent::updateFromState()
{
    if (resonatorBank == nullptr) return;

    for (int i = 0; i < kNumResonators; ++i)
    {
        auto& row = rows[i];
        auto& res = resonatorBank->getResonator(i);

        row.enableButton.setToggleState(res.getEnabled(), juce::dontSendNotification);

        bool isFree = res.getFrequencyMode() == SpreadVoiceResonator::FrequencyMode::Free;
        row.freeModeButton.setToggleState(isFree, juce::dontSendNotification);
        row.snapModeButton.setToggleState(!isFree, juce::dontSendNotification);
        row.freqSlider.setVisible(isFree);
        row.noteCombo.setVisible(!isFree);

        row.freqSlider.setValue(res.getFrequencyHz(), juce::dontSendNotification);
        row.noteCombo.setSelectedId(res.getMidiNote() + 1, juce::dontSendNotification);
        row.gainSlider.setValue(res.getGainDb(), juce::dontSendNotification);

        row.brightnessModSlider.setValue(res.getBrightnessEnergyAmount(), juce::dontSendNotification);
        row.spreadModSlider.setValue(res.getSpreadLevelEnergyAmount(), juce::dontSendNotification);
        row.detuneModSlider.setValue(res.getSpreadDetuneEnergyAmount(), juce::dontSendNotification);
        row.panModSlider.setValue(res.getPanWidthEnergyAmount(), juce::dontSendNotification);

        updateFreqDisplay(i);
    }
}

void ResonatorGridComponent::updateFreqDisplay(int index)
{
    if (resonatorBank == nullptr || index < 0 || index >= kNumResonators) return;
    auto& res = resonatorBank->getResonator(index);
    rows[index].freqValueLabel.setText(
        juce::String(static_cast<int>(res.getEffectiveFrequency())) + " Hz",
        juce::dontSendNotification);
}

void ResonatorGridComponent::sliderValueChanged(juce::Slider* slider)
{
    if (resonatorBank == nullptr) return;

    for (int i = 0; i < kNumResonators; ++i)
    {
        auto& row = rows[i];
        auto& res = resonatorBank->getResonator(i);

        if (slider == &row.freqSlider)        { res.setFrequencyHz(static_cast<float>(row.freqSlider.getValue())); updateFreqDisplay(i); return; }
        if (slider == &row.gainSlider)         { res.setGainDb(static_cast<float>(row.gainSlider.getValue())); return; }
        if (slider == &row.brightnessModSlider){ res.setBrightnessEnergyAmount(static_cast<float>(row.brightnessModSlider.getValue())); return; }
        if (slider == &row.spreadModSlider)    { res.setSpreadLevelEnergyAmount(static_cast<float>(row.spreadModSlider.getValue())); return; }
        if (slider == &row.detuneModSlider)    { res.setSpreadDetuneEnergyAmount(static_cast<float>(row.detuneModSlider.getValue())); return; }
        if (slider == &row.panModSlider)       { res.setPanWidthEnergyAmount(static_cast<float>(row.panModSlider.getValue())); return; }
    }
}

void ResonatorGridComponent::buttonClicked(juce::Button* button)
{
    if (resonatorBank == nullptr) return;

    for (int i = 0; i < kNumResonators; ++i)
    {
        auto& row = rows[i];
        auto& res = resonatorBank->getResonator(i);

        if (button == &row.enableButton)
        {
            res.setEnabled(row.enableButton.getToggleState());
            return;
        }
        if (button == &row.freeModeButton)
        {
            res.setFrequencyMode(SpreadVoiceResonator::FrequencyMode::Free);
            row.freqSlider.setVisible(true);
            row.noteCombo.setVisible(false);
            updateFreqDisplay(i);
            return;
        }
        if (button == &row.snapModeButton)
        {
            res.setFrequencyMode(SpreadVoiceResonator::FrequencyMode::Snap);
            row.freqSlider.setVisible(false);
            row.noteCombo.setVisible(true);
            int note = row.noteCombo.getSelectedId() - 1;
            res.setMidiNote(note);
            updateFreqDisplay(i);
            return;
        }
    }
}

void ResonatorGridComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (resonatorBank == nullptr) return;

    for (int i = 0; i < kNumResonators; ++i)
    {
        if (comboBox == &rows[i].noteCombo)
        {
            int note = rows[i].noteCombo.getSelectedId() - 1;
            resonatorBank->getResonator(i).setMidiNote(note);
            updateFreqDisplay(i);
            return;
        }
    }
}

void ResonatorGridComponent::applyMasterToAll()
{
    if (resonatorBank == nullptr) return;

    float gain = static_cast<float>(masterGainSlider.getValue());
    float bright = static_cast<float>(masterBrightSlider.getValue());
    float spread = static_cast<float>(masterSpreadSlider.getValue());
    float detune = static_cast<float>(masterDetuneSlider.getValue());
    float pan = static_cast<float>(masterPanSlider.getValue());

    for (int i = 0; i < kNumResonators; ++i)
    {
        auto& res = resonatorBank->getResonator(i);
        auto& row = rows[static_cast<size_t>(i)];
        res.setGainDb(gain);
        row.gainSlider.setValue(gain, juce::dontSendNotification);
        res.setBrightnessEnergyAmount(bright);
        row.brightnessModSlider.setValue(bright, juce::dontSendNotification);
        res.setSpreadLevelEnergyAmount(spread);
        row.spreadModSlider.setValue(spread, juce::dontSendNotification);
        res.setSpreadDetuneEnergyAmount(detune);
        row.detuneModSlider.setValue(detune, juce::dontSendNotification);
        res.setPanWidthEnergyAmount(pan);
        row.panModSlider.setValue(pan, juce::dontSendNotification);
    }
}

void ResonatorGridComponent::paint(juce::Graphics& g)
{
    // Row separators
    auto bounds = getLocalBounds();
    int headerH = 18;
    int rowH = (bounds.getHeight() - headerH) / kNumResonators;

    g.setColour(juce::Colour(0xff2a2a2a));
    for (int i = 1; i < kNumResonators; ++i)
    {
        int y = headerH + i * rowH;
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));
    }
}

void ResonatorGridComponent::resized()
{
    auto bounds = getLocalBounds();
    int headerH = 16;

    // Column widths - knobs need square-ish space
    int nameW = 22, enableW = 22, modeW = 60, freqW = 110;
    int knobW = 40;
    int gap = 3;

    // Headers
    auto hdr = bounds.removeFromTop(headerH);
    hdr.removeFromLeft(nameW + enableW + gap);
    headerLabels[0].setBounds(hdr.getX() - enableW, hdr.getY(), enableW, headerH);
    headerLabels[1].setBounds(hdr.removeFromLeft(modeW));
    hdr.removeFromLeft(gap);
    headerLabels[2].setBounds(hdr.removeFromLeft(freqW));
    hdr.removeFromLeft(gap);
    headerLabels[3].setBounds(hdr.removeFromLeft(knobW));
    hdr.removeFromLeft(gap);
    headerLabels[4].setBounds(hdr.removeFromLeft(knobW));
    hdr.removeFromLeft(gap);
    headerLabels[5].setBounds(hdr.removeFromLeft(knobW));
    hdr.removeFromLeft(gap);
    headerLabels[6].setBounds(hdr.removeFromLeft(knobW));
    hdr.removeFromLeft(gap);
    headerLabels[7].setBounds(hdr.removeFromLeft(knobW));

    // Master row (ALL: knobs that affect all 4 resonators)
    int masterRowH = juce::jmin(36, bounds.getHeight() / 5);
    auto masterRow = bounds.removeFromTop(masterRowH);
    int ks = juce::jmin(masterRowH - 2, knobW);
    masterLabel.setBounds(masterRow.getX(), masterRow.getY(), nameW + enableW, masterRowH);
    int mx = masterRow.getX() + nameW + enableW + gap + modeW + gap + freqW + gap;
    masterGainSlider.setBounds(mx, masterRow.getY() + (masterRowH - ks) / 2, ks, ks);
    mx += knobW + gap;
    masterBrightSlider.setBounds(mx, masterRow.getY() + (masterRowH - ks) / 2, ks, ks);
    mx += knobW + gap;
    masterSpreadSlider.setBounds(mx, masterRow.getY() + (masterRowH - ks) / 2, ks, ks);
    mx += knobW + gap;
    masterDetuneSlider.setBounds(mx, masterRow.getY() + (masterRowH - ks) / 2, ks, ks);
    mx += knobW + gap;
    masterPanSlider.setBounds(mx, masterRow.getY() + (masterRowH - ks) / 2, ks, ks);

    bounds.removeFromTop(2); // gap after master row

    int rowH = bounds.getHeight() / kNumResonators;

    for (int i = 0; i < kNumResonators; ++i)
    {
        auto& row = rows[i];
        auto r = bounds.removeFromTop(rowH);
        int knobSize = juce::jmin(rowH - 2, knobW);

        row.nameLabel.setBounds(r.getX(), r.getY(), nameW, rowH);
        row.enableButton.setBounds(r.getX() + nameW, r.getY() + 2, enableW - 4, rowH - 4);

        int x = r.getX() + nameW + enableW + gap;

        auto modeArea = juce::Rectangle<int>(x, r.getY(), modeW, rowH);
        row.freeModeButton.setBounds(modeArea.removeFromTop(rowH / 2));
        row.snapModeButton.setBounds(modeArea);
        x += modeW + gap;

        auto freqArea = juce::Rectangle<int>(x, r.getY(), freqW, rowH);
        auto freqControl = freqArea.removeFromTop(juce::jmin(rowH - 14, 24));
        row.freqSlider.setBounds(freqControl);
        row.noteCombo.setBounds(freqControl);
        row.freqValueLabel.setBounds(freqArea.withHeight(14));
        x += freqW + gap;

        // Knobs: gain, bright, spread, detune, pan
        row.gainSlider.setBounds(x, r.getY() + (rowH - knobSize) / 2, knobSize, knobSize);
        x += knobW + gap;
        row.brightnessModSlider.setBounds(x, r.getY() + (rowH - knobSize) / 2, knobSize, knobSize);
        x += knobW + gap;
        row.spreadModSlider.setBounds(x, r.getY() + (rowH - knobSize) / 2, knobSize, knobSize);
        x += knobW + gap;
        row.detuneModSlider.setBounds(x, r.getY() + (rowH - knobSize) / 2, knobSize, knobSize);
        x += knobW + gap;
        row.panModSlider.setBounds(x, r.getY() + (rowH - knobSize) / 2, knobSize, knobSize);
    }
}
