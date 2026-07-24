#include "GongLookAndFeel.h"

GongLookAndFeel::GongLookAndFeel()
{
    // Dark colour scheme
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(kBgDark));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(kAccent));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(kKnobTrack));
    setColour(juce::Slider::thumbColourId, juce::Colours::white);
    setColour(juce::Label::textColourId, juce::Colour(kTextBright));
    setColour(juce::TextButton::buttonColourId, juce::Colour(kBgModule));
    setColour(juce::TextButton::textColourOnId, juce::Colour(kTextBright));
    setColour(juce::TextButton::textColourOffId, juce::Colour(kTextBright));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(kBgModule));
    setColour(juce::ComboBox::textColourId, juce::Colour(kTextBright));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(kBorder));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(kBgModule));
    setColour(juce::PopupMenu::textColourId, juce::Colour(kTextBright));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(kAccent).withAlpha(0.3f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff333333));
    setColour(juce::TooltipWindow::textColourId, juce::Colour(kTextBright));
    setColour(juce::TooltipWindow::outlineColourId, juce::Colour(kBorder));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(kBgModule));
    setColour(juce::TextEditor::textColourId, juce::Colour(kTextBright));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(kBorder));
    setColour(juce::AlertWindow::backgroundColourId, juce::Colour(kBgPanel));
    setColour(juce::AlertWindow::textColourId, juce::Colour(kTextBright));
    setColour(juce::AlertWindow::outlineColourId, juce::Colour(kBorder));
}

void GongLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto arcRadius = radius - 4.0f;

    // Background circle
    g.setColour(juce::Colour(kKnobBg));
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Track arc (full range background)
    juce::Path trackArc;
    trackArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                           0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(kKnobTrack));
    g.strokePath(trackArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    // Value arc
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                           0.0f, rotaryStartAngle, angle, true);

    auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
    g.setColour(fillColour);
    g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    // Dot indicator at current position
    auto dotRadius = 3.0f;
    auto dotX = centreX + (arcRadius - 1.0f) * std::cos(angle - juce::MathConstants<float>::halfPi);
    auto dotY = centreY + (arcRadius - 1.0f) * std::sin(angle - juce::MathConstants<float>::halfPi);
    g.setColour(juce::Colours::white);
    g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);

    // Center dot (skip if slider has no text box - means center value mode draws its own)
    if (slider.getTextBoxPosition() != juce::Slider::NoTextBox)
    {
        g.setColour(juce::Colour(kTextDim));
        g.fillEllipse(centreX - 2.0f, centreY - 2.0f, 4.0f, 4.0f);
    }
}

void GongLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearHorizontal)
    {
        auto trackY = y + height / 2;
        auto trackHeight = 4;

        // Track background
        g.setColour(juce::Colour(kKnobTrack));
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(trackY - trackHeight / 2),
                               static_cast<float>(width), static_cast<float>(trackHeight), 2.0f);

        // Filled portion
        auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
        g.setColour(fillColour.isTransparent() ? juce::Colour(kAccent) : fillColour);
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(trackY - trackHeight / 2),
                               sliderPos - static_cast<float>(x), static_cast<float>(trackHeight), 2.0f);

        // Thumb
        g.setColour(juce::Colours::white);
        g.fillEllipse(sliderPos - 5.0f, static_cast<float>(trackY) - 5.0f, 10.0f, 10.0f);
    }
    else if (style == juce::Slider::LinearVertical)
    {
        auto trackX = x + width / 2;
        auto trackWidth = 4;

        g.setColour(juce::Colour(kKnobTrack));
        g.fillRoundedRectangle(static_cast<float>(trackX - trackWidth / 2), static_cast<float>(y),
                               static_cast<float>(trackWidth), static_cast<float>(height), 2.0f);

        auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
        g.setColour(fillColour.isTransparent() ? juce::Colour(kAccent) : fillColour);
        g.fillRoundedRectangle(static_cast<float>(trackX - trackWidth / 2), sliderPos,
                               static_cast<float>(trackWidth),
                               static_cast<float>(y + height) - sliderPos, 2.0f);

        g.setColour(juce::Colours::white);
        g.fillEllipse(static_cast<float>(trackX) - 5.0f, sliderPos - 5.0f, 10.0f, 10.0f);
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0, 0, style, slider);
    }
}

void GongLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    bool isOn = button.getToggleState();

    // Background
    g.setColour(isOn ? juce::Colour(kAccent).withAlpha(0.3f) : juce::Colour(kKnobBg));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border
    g.setColour(isOn ? juce::Colour(kAccent) : juce::Colour(kBorder));
    if (shouldDrawButtonAsHighlighted)
        g.setColour(juce::Colour(kAccent).withAlpha(0.7f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Text
    g.setColour(isOn ? juce::Colours::white : juce::Colour(kTextDim));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

void GongLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& /*backgroundColour*/,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

    auto bgColour = juce::Colour(kBgModule);
    if (shouldDrawButtonAsDown)
        bgColour = juce::Colour(kAccent).withAlpha(0.4f);
    else if (shouldDrawButtonAsHighlighted)
        bgColour = juce::Colour(kBgModule).brighter(0.1f);

    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(kBorder));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void GongLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                      bool /*shouldDrawButtonAsHighlighted*/,
                                      bool /*shouldDrawButtonAsDown*/)
{
    g.setFont(juce::FontOptions(12.0f));
    g.setColour(button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                           : juce::TextButton::textColourOffId));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

void GongLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(label.getFont());
    g.drawText(label.getText(), label.getLocalBounds(), label.getJustificationType(), false);
}

void GongLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                    juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

    // Arrow
    auto arrowZone = juce::Rectangle<float>(static_cast<float>(width - 20), 0.0f, 14.0f, static_cast<float>(height));
    juce::Path arrow;
    arrow.addTriangle(arrowZone.getX() + 2.0f, arrowZone.getCentreY() - 3.0f,
                      arrowZone.getRight() - 2.0f, arrowZone.getCentreY() - 3.0f,
                      arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);
    g.setColour(juce::Colour(kTextDim));
    g.fillPath(arrow);
}

void GongLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted,
                                         bool isTicked, bool /*hasSubMenu*/,
                                         const juce::String& text, const juce::String& /*shortcutKeyText*/,
                                         const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isSeparator)
    {
        g.setColour(juce::Colour(kBorder));
        g.fillRect(area.reduced(5, 0).withHeight(1));
        return;
    }

    if (isHighlighted)
    {
        g.setColour(juce::Colour(kAccent).withAlpha(0.3f));
        g.fillRect(area);
    }

    g.setColour(isActive ? juce::Colour(kTextBright) : juce::Colour(kTextDim));
    g.setFont(juce::FontOptions(13.0f));

    auto textArea = area.reduced(10, 0);
    if (isTicked)
    {
        g.drawText("*", textArea.removeFromLeft(15), juce::Justification::centred);
    }
    g.drawText(text, textArea, juce::Justification::centredLeft);
}

void GongLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.setColour(juce::Colour(kBgModule));
    g.fillRoundedRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 4.0f);
    g.setColour(juce::Colour(kBorder));
    g.drawRoundedRectangle(0.5f, 0.5f, static_cast<float>(width - 1), static_cast<float>(height - 1), 4.0f, 1.0f);
}
