#include "EnergyAccumulator.h"

EnergyAccumulator::EnergyAccumulator()
{
    // Initialize per-band energy with physically-motivated defaults (Step 2)
    bandDecayMs[0] = 3000.0f;  // Low: longest decay
    bandDecayMs[1] = 2000.0f;  // Mid-low
    bandDecayMs[2] = 1200.0f;  // Mid-high
    bandDecayMs[3] = 800.0f;   // High: shortest decay

    for (int i = 0; i < kNumBands; ++i)
    {
        bandEnergy[i].store(0.0f);
        bandDecayCoeffs[i] = 0.999f;
    }
    updateCoefficients();
}

void EnergyAccumulator::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    updateCoefficients();
    reset();
}

void EnergyAccumulator::reset()
{
    globalEnergy.store(0.0f);
    for (int i = 0; i < kNumBands; ++i)
    {
        bandEnergy[i].store(0.0f);
    }
}

void EnergyAccumulator::process(int numSamples)
{
    if (!isEnabled)
        return;

    juce::SpinLock::ScopedLockType lock(coeffLock);

    float dt = static_cast<float>(numSamples) / static_cast<float>(sampleRate);

    // Apply decay to global energy
    float global = globalEnergy.load();
    float decayFactor = std::pow(globalDecayCoeff, static_cast<float>(numSamples));
    global *= decayFactor;

    // Load per-band energies into local array for coupling computation
    float E[kNumBands];
    for (int i = 0; i < kNumBands; ++i)
    {
        E[i] = bandEnergy[i].load();
        // Apply per-band decay
        float bandDecayFactor = std::pow(bandDecayCoeffs[i], static_cast<float>(numSamples));
        E[i] *= bandDecayFactor;
    }

    // --- Step 3: Damping/Muting ---
    if (dampActive.load())
    {
        float dampFactor = std::pow(dampRate.load(), static_cast<float>(numSamples));
        global *= dampFactor;
        for (int i = 0; i < kNumBands; ++i)
            E[i] *= dampFactor;
    }

    // --- Step 4: Roll Priming ---
    float rpl = rollPrimeLevel.load();
    if (rpl > 0.0f)
    {
        float injection = rpl * dt;
        E[0] += injection;
        // Also add small amount to global
        global += injection * 0.5f;
    }

    // --- Step 1: Inter-band energy coupling ---
    float rate = couplingRate.load();
    if (rate > 0.0f)
    {
        // Nonlinear coupling: rate increases with total energy
        float totalEnergy = 0.0f;
        for (int i = 0; i < kNumBands; ++i)
            totalEnergy += E[i];
        float effectiveRate = rate * (1.0f + totalEnergy * 2.0f);

        float transferOut[kNumBands];
        for (int i = 0; i < kNumBands; ++i)
            transferOut[i] = E[i] * effectiveRate * dt;

        for (int i = 0; i < kNumBands; ++i)
        {
            for (int j = 0; j < kNumBands; ++j)
            {
                if (i == j) continue;
                float transfer = transferOut[i] * couplingMatrix[i][j];
                E[j] += transfer;
                E[i] -= transfer;
            }
        }
    }

    // --- Step 5: Power-law bloom ---
    float bThresh = bloomThreshold.load();
    float bRate = bloomRate.load();
    float normalizedGlobal = global / globalMaxEnergy;
    if (normalizedGlobal > bThresh && bThresh < 1.0f)
    {
        float bloomFactor = std::pow((normalizedGlobal - bThresh) / (1.0f - bThresh), 1.0f / 3.0f);
        // Transfer energy upward through bands
        for (int i = 0; i < kNumBands - 1; ++i)
        {
            float transfer = E[i] * bloomFactor * bRate * dt;
            E[i + 1] += transfer;
            E[i] -= transfer * 0.5f; // Partial drain from source
        }
    }

    // Clamp and store
    global = std::min(global, globalMaxEnergy);
    globalEnergy.store(std::max(0.0f, global));

    for (int i = 0; i < kNumBands; ++i)
    {
        E[i] = juce::jlimit(0.0f, globalMaxEnergy, E[i]);
        bandEnergy[i].store(E[i]);
    }
}

void EnergyAccumulator::injectEnergy(float strikeStrength, int band)
{
    if (!isEnabled)
        return;

    // Clamp strike strength
    strikeStrength = juce::jlimit(0.0f, 1.0f, strikeStrength);

    // Apply nonlinear curve: S^power
    float S_p = std::pow(strikeStrength, injectionPower);

    // Calculate injection with saturation headroom
    // E = clamp(E + k * S^p * (1 - E/E_max), E_max)
    float currentGlobal = globalEnergy.load();
    float headroom = 1.0f - (currentGlobal / globalMaxEnergy);
    headroom = std::max(0.0f, headroom);  // Ensure non-negative

    float injection = injectionGain * S_p * headroom;
    float newGlobal = std::min(currentGlobal + injection, globalMaxEnergy);
    globalEnergy.store(newGlobal);

    // Also inject into specific band if specified
    if (band >= 0 && band < kNumBands)
    {
        float currentBand = bandEnergy[band].load();
        float bandHeadroom = 1.0f - (currentBand / globalMaxEnergy);
        bandHeadroom = std::max(0.0f, bandHeadroom);

        float bandInjection = injectionGain * S_p * bandHeadroom;
        float newBand = std::min(currentBand + bandInjection, globalMaxEnergy);
        bandEnergy[band].store(newBand);
    }
}

float EnergyAccumulator::getBandEnergy(int band) const
{
    if (band >= 0 && band < kNumBands)
        return bandEnergy[band].load();
    return 0.0f;
}

float EnergyAccumulator::getNormalizedGlobalEnergy() const
{
    return globalEnergy.load() / globalMaxEnergy;
}

float EnergyAccumulator::getNormalizedBandEnergy(int band) const
{
    if (band >= 0 && band < kNumBands)
        return bandEnergy[band].load() / globalMaxEnergy;
    return 0.0f;
}

void EnergyAccumulator::setGlobalDecayMs(float ms)
{
    globalDecayMs = juce::jlimit(100.0f, 10000.0f, ms);
    updateCoefficients();
}

void EnergyAccumulator::setGlobalMaxEnergy(float max)
{
    globalMaxEnergy = juce::jlimit(0.1f, 10.0f, max);
}

void EnergyAccumulator::setInjectionGain(float gain)
{
    injectionGain = juce::jlimit(0.1f, 5.0f, gain);
}

void EnergyAccumulator::setInjectionPower(float power)
{
    injectionPower = juce::jlimit(0.5f, 3.0f, power);
}

void EnergyAccumulator::setBandDecayMs(int band, float ms)
{
    if (band >= 0 && band < kNumBands)
    {
        bandDecayMs[band] = juce::jlimit(100.0f, 10000.0f, ms);
        updateBandCoefficient(band);
    }
}

float EnergyAccumulator::getBandDecayMs(int band) const
{
    if (band >= 0 && band < kNumBands)
        return bandDecayMs[band];
    return 0.0f;
}

void EnergyAccumulator::setCouplingRate(float rate)
{
    couplingRate.store(juce::jlimit(0.0f, 0.1f, rate));
}

void EnergyAccumulator::setDampActive(bool active)
{
    dampActive.store(active);
}

void EnergyAccumulator::setDampRate(float rate)
{
    dampRate.store(juce::jlimit(0.5f, 0.999f, rate));
}

void EnergyAccumulator::setRollPrimeLevel(float level)
{
    rollPrimeLevel.store(juce::jlimit(0.0f, 1.0f, level));
}

void EnergyAccumulator::setBloomThreshold(float threshold)
{
    bloomThreshold.store(juce::jlimit(0.1f, 1.0f, threshold));
}

void EnergyAccumulator::setBloomRate(float rate)
{
    bloomRate.store(juce::jlimit(0.0f, 0.05f, rate));
}

void EnergyAccumulator::updateCoefficients()
{
    juce::SpinLock::ScopedLockType lock(coeffLock);

    // Calculate per-sample decay coefficient
    // We want energy to decay to near-zero in decayMs milliseconds
    // Using exponential decay: E(t) = E0 * coeff^t
    // For decay to 1% in decayMs: 0.01 = coeff^(decayMs * sampleRate / 1000)
    // coeff = 0.01^(1000 / (decayMs * sampleRate))

    float decayTimeSamples = (globalDecayMs / 1000.0f) * static_cast<float>(sampleRate);
    globalDecayCoeff = std::pow(0.01f, 1.0f / decayTimeSamples);

    // Update all band coefficients
    for (int i = 0; i < kNumBands; ++i)
    {
        float bandDecayTimeSamples = (bandDecayMs[i] / 1000.0f) * static_cast<float>(sampleRate);
        bandDecayCoeffs[i] = std::pow(0.01f, 1.0f / bandDecayTimeSamples);
    }
}

void EnergyAccumulator::updateBandCoefficient(int band)
{
    if (band >= 0 && band < kNumBands)
    {
        juce::SpinLock::ScopedLockType lock(coeffLock);
        float bandDecayTimeSamples = (bandDecayMs[band] / 1000.0f) * static_cast<float>(sampleRate);
        bandDecayCoeffs[band] = std::pow(0.01f, 1.0f / bandDecayTimeSamples);
    }
}
