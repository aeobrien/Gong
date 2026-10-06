#include "RotaryKnob.h"

RotaryKnob::RotaryKnob(const juce::String& name, const juce::String& suffix,
                         double min, double max, double step)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
    slider.setRange(min, max, step);
    if (suffix.isNotEmpty())
        slider.setTextValueSuffix(suffix);
    slider.setName(name);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(10.0f));
    label.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(label);
}

void RotaryKnob::setLabel(const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
}

void RotaryKnob::setTooltipText(const juce::String& text)
{
    slider.setTooltip(text);
}

void RotaryKnob::setEffectiveValue(float value)
{
    if (effectiveValue != value)
    {
        effectiveValue = value;
        repaint();
    }
}

void RotaryKnob::setEffectiveArcColour(juce::Colour colour)
{
    effectiveColour = colour;
}

void RotaryKnob::setArcColour(juce::Colour colour)
{
    arcColour = colour;
    hasCustomArcColour = true;
    slider.setColour(juce::Slider::rotarySliderFillColourId, colour);
}

void RotaryKnob::setCenterValueDisplay(bool enabled)
{
    centerValueMode = enabled;
    resized();
}

void RotaryKnob::resized()
{
    auto bounds = getLocalBounds();

    if (centerValueMode)
    {
        // Label sits tight above the knob, no text box below
        int labelH = 13;
        label.setBounds(bounds.removeFromTop(labelH));
        slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        slider.setBounds(bounds);
    }
    else
    {
        int labelH = 12;
        int textBoxH = 12;
        label.setBounds(bounds.removeFromTop(labelH));
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                               juce::jmin(56, bounds.getWidth()), textBoxH);
        slider.setBounds(bounds);
    }
}

void RotaryKnob::paint(juce::Graphics& g)
{
    // Center value display: draw value text in the middle of the knob
    if (centerValueMode)
    {
        auto sliderBounds = slider.getBounds().toFloat().reduced(2.0f);
        auto cx = sliderBounds.getCentreX();
        auto cy = sliderBounds.getCentreY();

        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));

        juce::String valueText = juce::String(slider.getValue(), 2);
        g.drawText(valueText, juce::Rectangle<float>(cx - 20.0f, cy - 6.0f, 40.0f, 12.0f),
                   juce::Justification::centred, false);
    }

    // Energy indicator ring
    if (effectiveValue < 0.0f)
        return;

    auto sliderBounds = slider.getBounds().toFloat().reduced(2.0f);
    auto radius = juce::jmin(sliderBounds.getWidth(), sliderBounds.getHeight()) / 2.0f;
    auto centreX = sliderBounds.getCentreX();
    auto centreY = sliderBounds.getCentreY();
    auto arcRadius = radius + 1.0f;

    auto rotaryStartAngle = juce::MathConstants<float>::pi * 1.25f;
    auto rotaryEndAngle = juce::MathConstants<float>::pi * 2.75f;

    auto range = slider.getRange();
    float baseNorm = static_cast<float>((slider.getValue() - range.getStart()) / range.getLength());
    float effNorm = static_cast<float>((effectiveValue - range.getStart()) / range.getLength());
    effNorm = juce::jlimit(0.0f, 1.0f, effNorm);

    float baseAngle = rotaryStartAngle + baseNorm * (rotaryEndAngle - rotaryStartAngle);
    float effAngle = rotaryStartAngle + effNorm * (rotaryEndAngle - rotaryStartAngle);

    if (std::abs(effAngle - baseAngle) > 0.01f)
    {
        juce::Path effArc;
        float startArc = juce::jmin(baseAngle, effAngle);
        float endArc = juce::jmax(baseAngle, effAngle);
        effArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                             0.0f, startArc, endArc, true);
        g.setColour(effectiveColour.withAlpha(0.7f));
        g.strokePath(effArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }
}
