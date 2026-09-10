#pragma once

#include <JuceHeader.h>

/**
 * Dark-themed LookAndFeel for the Gong Synthesizer.
 * Custom rotary knob rendering with arc indicators, module box styling, meters.
 */
class GongLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GongLookAndFeel();
    ~GongLookAndFeel() override = default;

    // Rotary slider with arc + dot indicator
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    // Linear slider styling
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // Toggle button styling
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    // Text button styling
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    // Label styling
    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    // ComboBox styling
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    // Popup menu
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    // Colors
    static constexpr juce::uint32 kBgDark     = 0xff121212;
    static constexpr juce::uint32 kBgPanel    = 0xff1e1e1e;
    static constexpr juce::uint32 kBgModule   = 0xff252525;
    static constexpr juce::uint32 kBorder     = 0xff3a3a3a;
    static constexpr juce::uint32 kTextBright = 0xffe0e0e0;
    static constexpr juce::uint32 kTextDim    = 0xff888888;
    static constexpr juce::uint32 kAccent     = 0xff64b5f6;
    static constexpr juce::uint32 kKnobBg     = 0xff2a2a2a;
    static constexpr juce::uint32 kKnobTrack  = 0xff3a3a3a;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GongLookAndFeel)
};
