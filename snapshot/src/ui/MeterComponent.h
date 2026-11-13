#pragma once

#include <JuceHeader.h>
#include "../DiagnosticState.h"

/**
 * Reusable stereo RMS+peak level meter.
 * Reads from a StageMeter* pointer on timer callback.
 */
class MeterComponent : public juce::Component
{
public:
    MeterComponent();
    ~MeterComponent() override = default;

    void setMeter(const StageMeter* meter);
    void setColour(juce::Colour colour);

    // Call from timer to update values
    void refresh();

    void paint(juce::Graphics& g) override;

private:
    const StageMeter* source = nullptr;
    float rmsL = 0.0f, rmsR = 0.0f;
    float peakL = 0.0f, peakR = 0.0f;
    juce::Colour barColour { 0xff4caf50 };

    // Peak hold with decay
    float peakHoldL = 0.0f, peakHoldR = 0.0f;
    static constexpr float kPeakDecay = 0.95f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterComponent)
};
