#pragma once

#include <JuceHeader.h>
#include "StrikeDescriptor.h"

/**
 * Maps MIDI CC/notes to StrikeDescriptor fields.
 * Provides a controller-driven mock for the Teensy hardware.
 *
 * CC assignments (defaults):
 *   CC 1  (mod wheel)   → attackSlope
 *   CC 74 (brightness)  → spectralCentroid
 *   CC 71 (timbre)      → hfEnergyRatio
 *   CC 73 (attack)      → decayShape
 *
 * CCs update descriptor fields continuously.
 * Note On sets velocity + padId (note/32), returns true to signal "fire strike."
 */
class MidiControllerMock
{
public:
    MidiControllerMock();
    ~MidiControllerMock() = default;

    // Process a single MIDI message. Returns true if a strike should fire.
    bool processMidiMessage(const juce::MidiMessage& message);

    // Get the current descriptor (updated by CCs, velocity/padId set on Note On)
    const StrikeDescriptor& getCurrentDescriptor() const { return descriptor; }
    bool isDampingActive() const { return dampingActive; }

    // CC mapping configuration
    void setAttackSlopeCC(int cc)       { attackSlopeCC = cc; }
    void setSpectralCentroidCC(int cc)  { spectralCentroidCC = cc; }
    void setHfEnergyRatioCC(int cc)     { hfEnergyRatioCC = cc; }
    void setDecayShapeCC(int cc)        { decayShapeCC = cc; }

    int getAttackSlopeCC() const        { return attackSlopeCC; }
    int getSpectralCentroidCC() const   { return spectralCentroidCC; }
    int getHfEnergyRatioCC() const      { return hfEnergyRatioCC; }
    int getDecayShapeCC() const         { return decayShapeCC; }

private:
    StrikeDescriptor descriptor;

    // Default CC assignments
    int attackSlopeCC = 1;       // Mod wheel
    int spectralCentroidCC = 74; // Brightness (MPE)
    int hfEnergyRatioCC = 71;   // Timbre (MPE)
    int decayShapeCC = 73;       // Attack time

    // Damping state (toggled by CC64 sustain pedal)
    bool dampingActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiControllerMock)
};
