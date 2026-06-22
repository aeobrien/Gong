#pragma once

#include <JuceHeader.h>

class ResonatorBank;
class SpreadVoiceResonator;

/**
 * 4-row resonator grid with controls per resonator row.
 * Each row: Enable | Mode (Free/Snap) | Frequency | Gain | Energy Mod Amounts
 * Replaces the flat slider grid from the old UI.
 */
class ResonatorGridComponent : public juce::Component,
                                private juce::Slider::Listener,
                                private juce::Button::Listener,
                                private juce::ComboBox::Listener
{
public:
    static constexpr int kNumResonators = 4;

    ResonatorGridComponent();
    ~ResonatorGridComponent() override = default;

    // Bind to the audio processor
    void setResonatorBank(ResonatorBank* bank);

    // Refresh display (call from timer)
    void refresh();

    // Called when a preset is loaded to sync UI
    void updateFromState();

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;

    void updateFreqDisplay(int index);
    void applyMasterToAll();

    ResonatorBank* resonatorBank = nullptr;

    // Master knobs (affect all 4 resonators)
    juce::Slider masterGainSlider;
    juce::Slider masterBrightSlider;
    juce::Slider masterSpreadSlider;
    juce::Slider masterDetuneSlider;
    juce::Slider masterPanSlider;
    juce::Label masterLabel { {}, "ALL:" };

    struct Row {
        juce::Label nameLabel;
        juce::ToggleButton enableButton;
        juce::ToggleButton freeModeButton { "Free" };
        juce::ToggleButton snapModeButton { "Snap" };
        juce::Slider freqSlider;
        juce::ComboBox noteCombo;
        juce::Label freqValueLabel;
        juce::Slider gainSlider;
        juce::Slider brightnessModSlider;
        juce::Slider spreadModSlider;
        juce::Slider detuneModSlider;
        juce::Slider panModSlider;
    };
    std::array<Row, kNumResonators> rows;

    // Column headers
    juce::Label headerLabels[8]; // On, Mode, Freq, Gain, Bright, Spread, Detune, Pan

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResonatorGridComponent)
};
