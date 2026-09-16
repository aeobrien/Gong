#pragma once

#include <JuceHeader.h>
#include "SpreadVoiceResonator.h"
#include "EnergyAccumulator.h"
#include "ModalTemplate.h"

/**
 * Container managing 4 independent SpreadVoiceResonators.
 * Routes per-band energy from EnergyAccumulator to each resonator.
 */
class ResonatorBank
{
public:
    static constexpr int kNumResonators = 4;

    ResonatorBank();
    ~ResonatorBank() = default;

    void prepare(double sampleRate, int blockSize);
    void process(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output);
    void reset();

    // Access individual resonators
    SpreadVoiceResonator& getResonator(int index);
    const SpreadVoiceResonator& getResonator(int index) const;

    // Set energy for all resonators from the accumulator
    void updateEnergy(const EnergyAccumulator& accumulator);

    // Convenience methods that apply to all resonators
    void setAllDecayTimes(float seconds);
    void setAllBaseBrightness(float brightness);
    void setAllSpreadLevel(float level);
    void setAllSpreadPanWidth(float width);

    // Step 6: Pitch glide for all resonators
    void setAllPitchGlideDirection(float dir);
    void setAllPitchGlideSensitivity(float cents);

    // Step 11: Modal template selection
    void applyModalTemplate(int templateIndex, float fundamentalHz);
    int getCurrentTemplateIndex() const { return currentTemplateIndex; }

    // Global master gain
    void setMasterGainDb(float db);
    float getMasterGainDb() const { return masterGainDb; }

private:
    std::array<SpreadVoiceResonator, kNumResonators> resonators;

    float masterGainDb = 0.0f;
    int currentTemplateIndex = -1; // -1 = no template (manual)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResonatorBank)
};
