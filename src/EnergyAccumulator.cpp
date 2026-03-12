#include "EnergyAccumulator.h"

EnergyAccumulator::EnergyAccumulator()
{
    // Initialize per-band energy
    for (int i = 0; i < kNumBands; ++i)
    {
        bandEnergy[i].store(0.0f);
        bandDecayMs[i] = 1500.0f;  // Default 1.5 second band decay
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

    // Apply decay to global energy
    float global = globalEnergy.load();
    float decayFactor = std::pow(globalDecayCoeff, static_cast<float>(numSamples));
    global *= decayFactor;
    globalEnergy.store(global);

    // Apply decay to each band
    for (int i = 0; i < kNumBands; ++i)
    {
        float band = bandEnergy[i].load();
        float bandDecayFactor = std::pow(bandDecayCoeffs[i], static_cast<float>(numSamples));
        band *= bandDecayFactor;
        bandEnergy[i].store(band);
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
