#pragma once

#include <JuceHeader.h>

/**
 * Energy accumulation system for gong synthesizer.
 * Maintains global and per-band energy reservoirs with leaky integrator decay
 * and nonlinear saturation for natural gong behavior.
 */
class EnergyAccumulator
{
public:
    static constexpr int kNumBands = 4;  // Per-resonator energy tracking

    EnergyAccumulator();
    ~EnergyAccumulator() = default;

    void prepare(double sampleRate);
    void reset();

    // Process a block - updates energy levels based on input
    void process(int numSamples);

    // Inject energy from a strike (call when strike detected)
    // strikeStrength: 0.0 to 1.0 (from velocity or audio level)
    // band: 0-3 for per-band injection, or -1 for global only
    void injectEnergy(float strikeStrength, int band = -1);

    // Get current energy levels (thread-safe for UI reading)
    float getGlobalEnergy() const { return globalEnergy.load(); }
    float getBandEnergy(int band) const;

    // Global energy parameters
    void setGlobalDecayMs(float ms);
    float getGlobalDecayMs() const { return globalDecayMs; }

    void setGlobalMaxEnergy(float max);
    float getGlobalMaxEnergy() const { return globalMaxEnergy; }

    void setInjectionGain(float gain);
    float getInjectionGain() const { return injectionGain; }

    void setInjectionPower(float power);  // Exponent for strike strength curve
    float getInjectionPower() const { return injectionPower; }

    // Per-band decay (can be different from global)
    void setBandDecayMs(int band, float ms);
    float getBandDecayMs(int band) const;

    // Enable/disable energy system
    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() const { return isEnabled; }

    // Get normalized energy (0.0 to 1.0 based on max energy)
    float getNormalizedGlobalEnergy() const;
    float getNormalizedBandEnergy(int band) const;

private:
    void updateCoefficients();
    void updateBandCoefficient(int band);

    double sampleRate = 48000.0;

    // Global energy
    std::atomic<float> globalEnergy { 0.0f };
    float globalDecayMs = 2000.0f;      // How long energy takes to decay
    float globalMaxEnergy = 1.0f;       // Saturation ceiling
    float globalDecayCoeff = 0.999f;    // Per-sample decay coefficient

    // Injection parameters
    float injectionGain = 1.0f;         // Multiplier for energy injection
    float injectionPower = 1.5f;        // Exponent for nonlinear response

    // Per-band energy (for per-resonator modulation)
    std::array<std::atomic<float>, kNumBands> bandEnergy;
    std::array<float, kNumBands> bandDecayMs;
    std::array<float, kNumBands> bandDecayCoeffs;

    bool isEnabled = true;

    // For thread-safe coefficient updates
    juce::SpinLock coeffLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnergyAccumulator)
};
