#pragma once

#include <JuceHeader.h>

/**
 * Circular visualization: 4 concentric arcs for band energies + center for global.
 * Provides real-time visual feedback of the energy accumulator state.
 */
class EnergyRingComponent : public juce::Component
{
public:
    EnergyRingComponent();
    ~EnergyRingComponent() override = default;

    void setGlobalEnergy(float energy);
    void setBandEnergy(int band, float energy);

    void paint(juce::Graphics& g) override;

private:
    float globalEnergy = 0.0f;
    float bandEnergies[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    static constexpr juce::uint32 kBandColours[4] = {
        0xfff44336, // Red (low)
        0xffff9800, // Orange (mid-low)
        0xff4caf50, // Green (mid-high)
        0xff2196f3  // Blue (high)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnergyRingComponent)
};
