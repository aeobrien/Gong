#include "MacroParameters.h"
#include <cmath>

void MacroParameters::setMacro(int index, float value)
{
    if (index >= 0 && index < kNumMacros)
    {
        if (value < 0.0f) value = 0.0f;
        if (value > 1.0f) value = 1.0f;
        macros[index].store(value);
    }
}

float MacroParameters::getMacro(int index) const
{
    if (index >= 0 && index < kNumMacros)
        return macros[index].load();
    return 0.5f;
}

// Macro 1: Size (0=small/tight, 1=large/expansive)
float MacroParameters::getDecayTimeScale() const
{
    float v = macros[0].load();
    return 0.5f + v * 1.5f;  // 0.5x to 2.0x
}

float MacroParameters::getCouplingRateOffset() const
{
    float v = macros[0].load();
    return (v - 0.5f) * 0.004f;  // -0.002 to +0.002
}

float MacroParameters::getFundamentalScale() const
{
    float v = macros[0].load();
    // Larger size = lower pitch: range 0.5x to 1.5x
    return 1.5f - v * 1.0f;
}

// Macro 2: Material (0=soft/warm, 1=hard/bright)
float MacroParameters::getBrightnessOffset() const
{
    float v = macros[1].load();
    return (v - 0.5f) * 0.6f;  // -0.3 to +0.3
}

float MacroParameters::getGlideDirectionOffset() const
{
    float v = macros[1].load();
    return (v - 0.5f) * 2.0f;  // -1.0 to +1.0 (softening to hardening)
}

float MacroParameters::getDampRateOffset() const
{
    float v = macros[1].load();
    return (v - 0.5f) * 0.1f;  // Less damping (bright) to more damping (muted)
}

// Macro 3: Intensity (0=gentle, 1=aggressive)
float MacroParameters::getInjectionGainOffset() const
{
    float v = macros[2].load();
    return (v - 0.5f) * 2.0f;  // -1.0 to +1.0
}

float MacroParameters::getBloomThresholdOffset() const
{
    float v = macros[2].load();
    return (0.5f - v) * 0.4f;  // Higher intensity = lower threshold
}

float MacroParameters::getCrashThresholdOffset() const
{
    float v = macros[2].load();
    return (0.5f - v) * 0.3f;  // Higher intensity = lower crash threshold
}

// Macro 4: Space (0=dry/close, 1=wet/far)
float MacroParameters::getConvMixOffset() const
{
    float v = macros[3].load();
    return (v - 0.5f) * 0.6f;  // -0.3 to +0.3
}

float MacroParameters::getSpreadWidthOffset() const
{
    float v = macros[3].load();
    return (v - 0.5f) * 0.6f;  // -0.3 to +0.3
}
