#pragma once

#include <JuceHeader.h>
#include "ModulePanel.h"

/**
 * Transparent overlay drawing cables between module I/O ports.
 * Same-row cables use gentle S-curves.
 * Cross-row cables use right-angle routing through the gap.
 */
class PatchCableOverlay : public juce::Component
{
public:
    PatchCableOverlay();
    ~PatchCableOverlay() override = default;

    struct Cable {
        ModulePanel* source = nullptr;
        ModulePanel* dest = nullptr;
        float signalLevel = 0.0f;
    };

    void clearCables();
    void addCable(ModulePanel* source, ModulePanel* dest);
    void setSignalLevel(int cableIndex, float level);

    // Set the Y coordinate where cross-row cables run horizontally
    // Call this from resized() after laying out modules
    void setCrossRowY(float y) { crossRowRouteY = y; }

    void paint(juce::Graphics& g) override;

private:
    std::vector<Cable> cables;
    float crossRowRouteY = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchCableOverlay)
};
