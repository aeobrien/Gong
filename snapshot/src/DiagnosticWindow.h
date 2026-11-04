#pragma once

#include <JuceHeader.h>
#include "DiagnosticState.h"
#include "ModulationTypes.h"

// Forward declare
class DiagnosticContentComponent;

class DiagnosticWindow : public juce::DocumentWindow
{
public:
    DiagnosticWindow(DiagnosticState& state);
    ~DiagnosticWindow() override;

    void closeButtonPressed() override;

    // Call periodically from the main component's timer
    void refresh();

private:
    DiagnosticContentComponent* content = nullptr;
};

// ---- Signal Chain Tab ----
class SignalChainTab : public juce::Component
{
public:
    SignalChainTab(DiagnosticState& state);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void refresh();

private:
    DiagnosticState& state;

    // Test signal controls
    juce::ComboBox testTypeCombo;
    juce::Label testTypeLabel { {}, "Signal:" };
    juce::Slider testFreqSlider;
    juce::Label testFreqLabel { {}, "Freq:" };
    juce::ComboBox testInjectCombo;
    juce::Label testInjectLabel { {}, "Inject:" };
    juce::Slider testGainSlider;
    juce::Label testGainLabel { {}, "Gain:" };

    // Solo/Mute/Bypass buttons per stage
    struct StageButtons {
        juce::TextButton solo { "S" };
        juce::TextButton mute { "M" };
        juce::TextButton bypass { "B" };
    };
    StageButtons synthButtons, convButtons, exciterButtons, compButtons;

    void setupStageButtons(StageButtons& btns, std::atomic<int>& stateAtom);

    // Cached meter values for paint
    struct MeterValues {
        float rmsL, rmsR, peakL, timeUs;
    };
    std::array<MeterValues, 6> meterCache{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SignalChainTab)
};

// ---- Energy Monitor Tab ----
class EnergyMonitorTab : public juce::Component
{
public:
    EnergyMonitorTab(DiagnosticState& state);
    void paint(juce::Graphics& g) override;
    void refresh();

private:
    DiagnosticState& state;
    float globalEnergy = 0;
    std::array<float, 4> bandEnergies{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnergyMonitorTab)
};

// ---- Modulation Matrix Tab ----
class ModulationMatrixTab : public juce::Component
{
public:
    ModulationMatrixTab(DiagnosticState& state);
    void resized() override;
    void refresh();

    // Access routes for preset save/load
    std::vector<ModRoute> getRoutes() const;
    void setRoutes(const std::vector<ModRoute>& routes);

    float getLfo1Rate() const;
    float getLfo2Rate() const;
    int getLfo1Shape() const;
    int getLfo2Shape() const;
    void setLfoConfig(float rate1, int shape1, float rate2, int shape2);

private:
    DiagnosticState& state;

    struct RouteRow {
        juce::ComboBox source;
        juce::ComboBox target;
        juce::Slider amount;
        juce::ComboBox curve;
        juce::ToggleButton enabled;
        juce::TextButton deleteBtn { "X" };
    };
    juce::OwnedArray<RouteRow> rows;

    juce::TextButton addButton { "Add Route" };

    // LFO controls
    juce::Label lfo1RateLabel { {}, "LFO1 Rate:" };
    juce::Slider lfo1RateSlider;
    juce::ComboBox lfo1ShapeCombo;
    juce::Label lfo2RateLabel { {}, "LFO2 Rate:" };
    juce::Slider lfo2RateSlider;
    juce::ComboBox lfo2ShapeCombo;

    void addRouteRow(const ModRoute& route = {});
    void removeRouteRow(int index);
    void pushRoutesToState();

    juce::Viewport viewport;
    juce::Component routeContainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrixTab)
};

// ---- Content Component (tabbed) ----
class DiagnosticContentComponent : public juce::Component
{
public:
    DiagnosticContentComponent(DiagnosticState& state);
    void resized() override;
    void refresh();

    ModulationMatrixTab& getModulationTab() { return modTab; }

private:
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    SignalChainTab signalTab;
    EnergyMonitorTab energyTab;
    ModulationMatrixTab modTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiagnosticContentComponent)
};
