#pragma once

#include <array>
#include <atomic>

/**
 * Step 15: Macro parameters.
 * 4 macro knobs that control multiple targets simultaneously.
 *
 * Macro 1 "Size": fundamental freq scale, decay times, coupling rate
 * Macro 2 "Material": brightness, pitch glide direction, damping rate
 * Macro 3 "Intensity": injection gain/power, bloom threshold, crash threshold
 * Macro 4 "Space": conv mix, spread width
 */
class MacroParameters
{
public:
    static constexpr int kNumMacros = 4;

    MacroParameters() { initDefaults(); }

    void setMacro(int index, float value);
    float getMacro(int index) const;

    // Get the offset contribution for various parameters from all macros
    // These return offsets that should be added to the base parameter values

    // Macro 1: Size
    float getDecayTimeScale() const;      // Multiplier for decay times (0.5 to 2.0)
    float getCouplingRateOffset() const;   // Offset for coupling rate
    float getFundamentalScale() const;     // Multiplier for fundamental frequency

    // Macro 2: Material
    float getBrightnessOffset() const;     // Offset for brightness
    float getGlideDirectionOffset() const; // Offset for pitch glide direction
    float getDampRateOffset() const;       // Offset for damping rate

    // Macro 3: Intensity
    float getInjectionGainOffset() const;  // Offset for injection gain
    float getBloomThresholdOffset() const; // Offset for bloom threshold
    float getCrashThresholdOffset() const; // Offset for crash threshold

    // Macro 4: Space
    float getConvMixOffset() const;        // Offset for convolution mix
    float getSpreadWidthOffset() const;    // Offset for spread width

private:
    std::array<std::atomic<float>, kNumMacros> macros{};

    // Default values set in constructor
public:
    void initDefaults() {
        for (auto& m : macros) m.store(0.5f);
    }
};
