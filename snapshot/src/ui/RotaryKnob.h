#pragma once

#include <JuceHeader.h>

/**
 * Custom rotary slider with value arc, label, and energy-indicator ring
 * showing the modulated value vs the user-set base value.
 */
class RotaryKnob : public juce::Component
{
public:
    RotaryKnob(const juce::String& name, const juce::String& suffix = {},
               double min = 0.0, double max = 1.0, double step = 0.01);
    ~RotaryKnob() override = default;

    juce::Slider& getSlider() { return slider; }

    void setLabel(const juce::String& text);
    void setTooltipText(const juce::String& text);

    // Energy indicator ring: shows effective (modulated) value
    void setEffectiveValue(float value);
    void setEffectiveArcColour(juce::Colour colour);

    // Accent colour for the value arc
    void setArcColour(juce::Colour colour);

    // Center value display: show value inside the knob, label tight above
    void setCenterValueDisplay(bool enabled);

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    juce::Slider slider;
    juce::Label label;
    float effectiveValue = -1.0f;
    juce::Colour effectiveColour { 0xffff9800 };
    juce::Colour arcColour;
    bool hasCustomArcColour = false;
    bool centerValueMode = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotaryKnob)
};
