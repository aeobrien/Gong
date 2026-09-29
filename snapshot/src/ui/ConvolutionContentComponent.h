#pragma once

#include <JuceHeader.h>
#include "../IRWaveformComponent.h"

/**
 * Custom content for the Convolution Reverb module panel.
 * Contains IR waveform display, Load IR A/B buttons, and file labels.
 */
class ConvolutionContentComponent : public juce::Component
{
public:
    ConvolutionContentComponent()
    {
        addAndMakeVisible(irWaveform);
        addAndMakeVisible(loadIRButton);
        addAndMakeVisible(loadIRBButton);
        addAndMakeVisible(irFileLabel);
        addAndMakeVisible(irBFileLabel);

        irFileLabel.setFont(juce::FontOptions(9.5f));
        irFileLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
        irBFileLabel.setFont(juce::FontOptions(9.5f));
        irBFileLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
    }

    ~ConvolutionContentComponent() override = default;

    IRWaveformComponent& getIRWaveform() { return irWaveform; }
    juce::TextButton& getLoadIRButton() { return loadIRButton; }
    juce::TextButton& getLoadIRBButton() { return loadIRBButton; }
    juce::Label& getIRFileLabel() { return irFileLabel; }
    juce::Label& getIRBFileLabel() { return irBFileLabel; }

    void resized() override
    {
        auto bounds = getLocalBounds();

        irWaveform.setBounds(bounds.removeFromTop(juce::jmax(24, bounds.getHeight() / 3)));
        bounds.removeFromTop(2);

        auto row1 = bounds.removeFromTop(18);
        loadIRButton.setBounds(row1.removeFromLeft(65));
        row1.removeFromLeft(4);
        irFileLabel.setBounds(row1);

        bounds.removeFromTop(2);

        auto row2 = bounds.removeFromTop(18);
        loadIRBButton.setBounds(row2.removeFromLeft(65));
        row2.removeFromLeft(4);
        irBFileLabel.setBounds(row2);
    }

private:
    IRWaveformComponent irWaveform;
    juce::TextButton loadIRButton { "Load IR A..." };
    juce::TextButton loadIRBButton { "Load IR B..." };
    juce::Label irFileLabel { {}, "No IR loaded" };
    juce::Label irBFileLabel { {}, "No IR B" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConvolutionContentComponent)
};
