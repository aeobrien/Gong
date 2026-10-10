#pragma once

#include <JuceHeader.h>

/**
 * Visual display component for impulse response waveforms.
 * Draws the waveform scaled to component bounds with L/R channel overlay.
 */
class IRWaveformComponent : public juce::Component
{
public:
    IRWaveformComponent();
    ~IRWaveformComponent() override = default;

    // Set the IR buffer to display (can be nullptr to clear)
    void setIR(const juce::AudioBuffer<float>* ir, int sampleRate);

    // Clear the display
    void clear();

    // Colors
    void setWaveformColour(juce::Colour colour) { waveformColour = colour; repaint(); }
    void setWaveformColourR(juce::Colour colour) { waveformColourR = colour; repaint(); }
    void setBackgroundColour(juce::Colour colour) { backgroundColour = colour; repaint(); }
    void setCenterLineColour(juce::Colour colour) { centerLineColour = colour; repaint(); }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void buildThumbnail();

    const juce::AudioBuffer<float>* irData = nullptr;
    int irSampleRate = 0;

    // Downsampled thumbnail for efficient display
    std::vector<float> thumbnailMinL;
    std::vector<float> thumbnailMaxL;
    std::vector<float> thumbnailMinR;
    std::vector<float> thumbnailMaxR;

    juce::Colour waveformColour { juce::Colours::cyan };
    juce::Colour waveformColourR { juce::Colours::orange.withAlpha(0.5f) };
    juce::Colour backgroundColour { juce::Colours::black };
    juce::Colour centerLineColour { juce::Colours::grey.withAlpha(0.5f) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRWaveformComponent)
};
