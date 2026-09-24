#include "CrashNoiseGenerator.h"

CrashNoiseGenerator::CrashNoiseGenerator()
{
}

void CrashNoiseGenerator::prepare(double sampleRate, int blockSize)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    bandpassL.prepare(spec);
    bandpassL.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    bandpassL.setCutoffFrequency(3500.0f);  // Center of 1-8kHz metallic band
    bandpassL.setResonance(0.5f);

    bandpassR.prepare(spec);
    bandpassR.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    bandpassR.setCutoffFrequency(3500.0f);
    bandpassR.setResonance(0.5f);

    currentNoiseLevel = 0.0f;
    isPrepared = true;
}

void CrashNoiseGenerator::reset()
{
    bandpassL.reset();
    bandpassR.reset();
    currentNoiseLevel = 0.0f;
}

void CrashNoiseGenerator::process(float globalEnergy, juce::AudioBuffer<float>& buffer)
{
    if (!isPrepared) return;

    auto numSamples = buffer.getNumSamples();

    // Target noise level based on energy vs threshold
    float targetLevel = 0.0f;
    if (globalEnergy > threshold)
    {
        targetLevel = (globalEnergy - threshold) / (1.0f - threshold);
        targetLevel = std::min(targetLevel, 1.0f) * mixLevel;
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Smooth the noise level to avoid clicks
        currentNoiseLevel += (targetLevel - currentNoiseLevel) * smoothingCoeff;

        if (currentNoiseLevel < 0.0001f) continue;

        // Generate white noise and bandpass filter for metallic character
        float noiseL = (random.nextFloat() * 2.0f - 1.0f);
        float noiseR = (random.nextFloat() * 2.0f - 1.0f);

        float filteredL = bandpassL.processSample(0, noiseL);
        float filteredR = bandpassR.processSample(0, noiseR);

        buffer.addSample(0, sample, filteredL * currentNoiseLevel);
        if (buffer.getNumChannels() > 1)
            buffer.addSample(1, sample, filteredR * currentNoiseLevel);
    }
}

void CrashNoiseGenerator::setThreshold(float t)
{
    threshold = juce::jlimit(0.1f, 1.0f, t);
}

void CrashNoiseGenerator::setMixLevel(float level)
{
    mixLevel = juce::jlimit(0.0f, 1.0f, level);
}
