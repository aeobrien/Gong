#pragma once

#include <JuceHeader.h>

/**
 * Energy accumulation system for gong synthesizer.
 * Maintains global and per-band energy reservoirs with leaky integrator decay
 * and nonlinear saturation for natural gong behavior.
 *
 * Nonlinear features:
 * - Inter-band coupling: energy transfers between bands (Step 1)
 * - Per-band decay: configurable per-band decay times (Step 2)
 * - Damping/muting: fast exponential decay when active (Step 3)
 * - Roll priming: continuous low-level energy injection (Step 4)
 * - Power-law bloom: energy cascade at high levels (Step 5)
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

    // --- Step 1: Inter-band coupling ---
    void setCouplingRate(float rate);
    float getCouplingRate() const { return couplingRate.load(); }

    // --- Step 3: Damping/Muting ---
    void setDampActive(bool active);
    bool getDampActive() const { return dampActive.load(); }
    void setDampRate(float rate);
    float getDampRate() const { return dampRate.load(); }

    // --- Step 4: Roll Priming ---
    void setRollPrimeLevel(float level);
    float getRollPrimeLevel() const { return rollPrimeLevel.load(); }

    // --- Step 5: Power-law Bloom ---
    void setBloomThreshold(float threshold);
    float getBloomThreshold() const { return bloomThreshold.load(); }
    void setBloomRate(float rate);
    float getBloomRate() const { return bloomRate.load(); }

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

    // --- Step 1: Inter-band coupling ---
    // 4x4 coupling matrix: couplingMatrix[i][j] = fraction of energy transferred from band i to band j
    float couplingMatrix[kNumBands][kNumBands] = {
        // To:  low    mid-lo mid-hi high
        /*low*/   { 0.0f, 0.30f, 0.10f, 0.05f },
        /*mid-lo*/{ 0.15f, 0.0f, 0.40f, 0.15f },
        /*mid-hi*/{ 0.05f, 0.20f, 0.0f, 0.35f },
        /*high*/  { 0.02f, 0.05f, 0.20f, 0.0f  }
    };
    std::atomic<float> couplingRate { 0.001f };

    // --- Step 3: Damping/Muting ---
    std::atomic<bool> dampActive { false };
    std::atomic<float> dampRate { 0.95f };

    // --- Step 4: Roll Priming ---
    std::atomic<float> rollPrimeLevel { 0.0f };

    // --- Step 5: Power-law Bloom ---
    std::atomic<float> bloomThreshold { 0.7f };
    std::atomic<float> bloomRate { 0.002f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnergyAccumulator)
};
