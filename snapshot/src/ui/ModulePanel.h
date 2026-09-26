#pragma once

#include <JuceHeader.h>
#include "RotaryKnob.h"
#include "MeterComponent.h"
#include "ModuleDescriptions.h"

/**
 * Configurable module box component.
 * Structure: color-coded title bar + description + knob grid + I/O meters + custom content slot.
 */

struct KnobDescriptor {
    juce::String id;
    juce::String name;
    juce::String suffix;
    double min, max, step;
    double defaultValue;
};

struct ModuleDescriptor {
    juce::String id;
    const ModuleDescriptions::ModuleInfo* info = nullptr;
    std::vector<KnobDescriptor> knobs;
    bool hasInputMeter = false;
    bool hasOutputMeter = false;
};

class ModulePanel : public juce::Component
{
public:
    ModulePanel();
    ~ModulePanel() override = default;

    void configure(const ModuleDescriptor& descriptor);

    // Access knobs by id
    RotaryKnob* getKnob(const juce::String& id);
    juce::Slider* getSlider(const juce::String& id);

    // Meter access
    MeterComponent& getInputMeter() { return inputMeter; }
    MeterComponent& getOutputMeter() { return outputMeter; }

    // Custom content slot
    void setCustomContent(juce::Component* content);

    // I/O port positions (for patch cable routing, in parent coordinates)
    juce::Point<int> getInputPortPosition() const;
    juce::Point<int> getOutputPortPosition() const;

    // Module colour
    juce::Colour getModuleColour() const { return moduleColour; }

    // Info button toggle
    void setInfoExpanded(bool expanded);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    ModuleDescriptor desc;
    juce::Colour moduleColour { 0xff444444 };

    // Header
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::TextButton infoButton { "?" };
    bool infoExpanded = false;
    juce::TextEditor infoText;

    // Knobs
    std::vector<std::unique_ptr<RotaryKnob>> knobs;
    std::unordered_map<juce::String, RotaryKnob*> knobMap;

    // Meters
    MeterComponent inputMeter;
    MeterComponent outputMeter;
    bool showInputMeter = false;
    bool showOutputMeter = false;

    // Custom content
    juce::Component* customContent = nullptr;

    static constexpr int kHeaderHeight = 38;
    static constexpr int kKnobWidth = 70;
    static constexpr int kKnobHeight = 80;
    static constexpr int kMeterWidth = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePanel)
};
