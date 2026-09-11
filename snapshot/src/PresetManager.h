#pragma once

#include <JuceHeader.h>
#include "ModulationTypes.h"

class GongSynthesizer;
class ConvolutionEngine;
class ExciterProcessor;
class MultibandCompressor;
struct DiagnosticState;

/**
 * Manages JSON presets for the Gong Energy Synthesizer.
 * Scans a presets directory for .json files, and provides
 * save/load functionality for all synthesizer parameters.
 */
class PresetManager
{
public:
    PresetManager();
    ~PresetManager() = default;

    // Set the directory where presets are stored
    void setPresetsDirectory(const juce::File& directory);
    juce::File getPresetsDirectory() const { return presetsDirectory; }

    // Set the directory where IR files are stored
    void setIRDirectory(const juce::File& directory);
    juce::File getIRDirectory() const { return irDirectory; }

    // Get list of available preset names (scans directory for .json files)
    juce::StringArray getPresetNames() const;

    // Save current state to a named preset
    bool savePreset(const juce::String& name,
                    const GongSynthesizer& synth,
                    const ConvolutionEngine& conv,
                    const ExciterProcessor& exciter,
                    const MultibandCompressor& comp,
                    float masterGain,
                    DiagnosticState* diagState = nullptr);

    // Load a named preset and apply it
    bool loadPreset(const juce::String& name,
                    GongSynthesizer& synth,
                    ConvolutionEngine& conv,
                    ExciterProcessor& exciter,
                    MultibandCompressor& comp,
                    float& masterGain,
                    DiagnosticState* diagState = nullptr);

    // Get/set current preset name
    juce::String getCurrentPresetName() const { return currentPresetName; }

private:
    juce::var serializeToJson(const GongSynthesizer& synth,
                              const ConvolutionEngine& conv,
                              const ExciterProcessor& exciter,
                              const MultibandCompressor& comp,
                              float masterGain,
                              const juce::String& name,
                              DiagnosticState* diagState) const;

    bool deserializeFromJson(const juce::var& json,
                             GongSynthesizer& synth,
                             ConvolutionEngine& conv,
                             ExciterProcessor& exciter,
                             MultibandCompressor& comp,
                             float& masterGain,
                             DiagnosticState* diagState);

    juce::File presetsDirectory;
    juce::File irDirectory;
    juce::String currentPresetName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
