#include "ModulePanel.h"

ModulePanel::ModulePanel()
{
    titleLabel.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setFont(juce::FontOptions(9.5f));
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);
    subtitleLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(subtitleLabel);

    infoButton.setSize(20, 20);
    infoButton.onClick = [this] { setInfoExpanded(!infoExpanded); };
    addAndMakeVisible(infoButton);

    infoText.setMultiLine(true);
    infoText.setReadOnly(true);
    infoText.setScrollbarsShown(true);
    infoText.setFont(juce::FontOptions(10.0f));
    addChildComponent(infoText);

    addChildComponent(inputMeter);
    addChildComponent(outputMeter);
}

void ModulePanel::configure(const ModuleDescriptor& descriptor)
{
    desc = descriptor;

    if (desc.info != nullptr)
    {
        moduleColour = desc.info->colour;
        titleLabel.setText(desc.info->title, juce::dontSendNotification);
        titleLabel.setColour(juce::Label::textColourId, moduleColour.brighter(0.5f));
        subtitleLabel.setText(desc.info->subtitle, juce::dontSendNotification);
        subtitleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
        infoText.setText(desc.info->description, false);
    }

    // Create knobs
    knobs.clear();
    knobMap.clear();
    for (auto& kd : desc.knobs)
    {
        auto knob = std::make_unique<RotaryKnob>(kd.name, kd.suffix, kd.min, kd.max, kd.step);
        knob->getSlider().setValue(kd.defaultValue, juce::dontSendNotification);
        knob->setArcColour(moduleColour);

        auto tooltip = ModuleDescriptions::getTooltip(kd.id);
        if (tooltip.isNotEmpty())
            knob->setTooltipText(tooltip);

        knobMap[kd.id] = knob.get();
        addAndMakeVisible(*knob);
        knobs.push_back(std::move(knob));
    }

    // Meters
    showInputMeter = desc.hasInputMeter;
    showOutputMeter = desc.hasOutputMeter;
    inputMeter.setColour(moduleColour);
    outputMeter.setColour(moduleColour);
    inputMeter.setVisible(showInputMeter);
    outputMeter.setVisible(showOutputMeter);

    resized();
}

RotaryKnob* ModulePanel::getKnob(const juce::String& id)
{
    auto it = knobMap.find(id);
    return it != knobMap.end() ? it->second : nullptr;
}

juce::Slider* ModulePanel::getSlider(const juce::String& id)
{
    auto* knob = getKnob(id);
    return knob != nullptr ? &knob->getSlider() : nullptr;
}

void ModulePanel::setCustomContent(juce::Component* content)
{
    if (customContent != nullptr)
        removeChildComponent(customContent);

    customContent = content;
    if (customContent != nullptr)
    {
        addAndMakeVisible(customContent);
        resized();
    }
}

juce::Point<int> ModulePanel::getInputPortPosition() const
{
    return getPosition() + juce::Point<int>(0, getHeight() / 2);
}

juce::Point<int> ModulePanel::getOutputPortPosition() const
{
    return getPosition() + juce::Point<int>(getWidth(), getHeight() / 2);
}

void ModulePanel::setInfoExpanded(bool expanded)
{
    infoExpanded = expanded;
    infoText.setVisible(expanded);
    resized();
    repaint();
}

void ModulePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Module background
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Border with module colour
    g.setColour(moduleColour.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.5f);

    // Header background
    auto headerBounds = bounds.withHeight(static_cast<float>(kHeaderHeight));
    g.setColour(moduleColour.withAlpha(0.15f));
    // Fill top-left and top-right rounded, bottom square
    juce::Path headerPath;
    headerPath.addRoundedRectangle(headerBounds.getX(), headerBounds.getY(),
                                   headerBounds.getWidth(), headerBounds.getHeight(),
                                   6.0f, 6.0f, true, true, false, false);
    g.fillPath(headerPath);

    // I/O port dots
    auto portRadius = 4.0f;
    if (showInputMeter)
    {
        g.setColour(moduleColour.withAlpha(0.8f));
        g.fillEllipse(-portRadius, static_cast<float>(getHeight()) / 2.0f - portRadius,
                       portRadius * 2.0f, portRadius * 2.0f);
    }
    if (showOutputMeter)
    {
        g.setColour(moduleColour.withAlpha(0.8f));
        g.fillEllipse(static_cast<float>(getWidth()) - portRadius,
                       static_cast<float>(getHeight()) / 2.0f - portRadius,
                       portRadius * 2.0f, portRadius * 2.0f);
    }
}

void ModulePanel::resized()
{
    auto bounds = getLocalBounds();

    // Header
    auto header = bounds.removeFromTop(kHeaderHeight);
    auto titleRow = header.reduced(4, 2);

    infoButton.setBounds(titleRow.removeFromRight(18).withHeight(18));
    titleRow.removeFromRight(2);

    // Shrink title font if panel is narrow
    float titleFontSize = getWidth() < 140 ? 10.5f : 13.0f;
    titleLabel.setFont(juce::FontOptions(titleFontSize).withStyle("Bold"));

    auto titleArea = titleRow.removeFromTop(18);
    titleLabel.setBounds(titleArea);
    subtitleLabel.setBounds(titleRow);

    // Info text (expanded) - take up to 40% of remaining height
    if (infoExpanded)
    {
        int infoH = juce::jmax(60, bounds.getHeight() * 2 / 5);
        auto infoArea = bounds.removeFromTop(infoH);
        infoText.setBounds(infoArea.reduced(6, 2));
    }

    // Input meter
    auto contentArea = bounds.reduced(6, 4);
    if (showInputMeter)
    {
        inputMeter.setBounds(contentArea.removeFromLeft(kMeterWidth));
        contentArea.removeFromLeft(4);
    }

    // Output meter
    if (showOutputMeter)
    {
        outputMeter.setBounds(contentArea.removeFromRight(kMeterWidth));
        contentArea.removeFromRight(4);
    }

    // Custom content gets remaining space below knobs (or all space if no knobs)
    if (customContent != nullptr && knobs.empty())
    {
        customContent->setBounds(contentArea);
    }
    else
    {
        // Layout knobs in a grid
        int numKnobs = static_cast<int>(knobs.size());
        if (numKnobs > 0)
        {
            int availW = contentArea.getWidth();
            int knobsPerRow = juce::jmax(1, availW / kKnobWidth);
            int actualKnobW = availW / juce::jmax(1, juce::jmin(knobsPerRow, numKnobs));

            int row = 0, col = 0;
            for (auto& knob : knobs)
            {
                int kx = contentArea.getX() + col * actualKnobW;
                int ky = contentArea.getY() + row * kKnobHeight;
                knob->setBounds(kx, ky, actualKnobW, kKnobHeight);

                ++col;
                if (col >= knobsPerRow)
                {
                    col = 0;
                    ++row;
                }
            }

            // Custom content below knobs
            if (customContent != nullptr)
            {
                int knobRows = (numKnobs + knobsPerRow - 1) / knobsPerRow;
                auto customArea = contentArea.withTrimmedTop(knobRows * kKnobHeight);
                if (customArea.getHeight() > 10)
                    customContent->setBounds(customArea);
            }
        }
    }
}
