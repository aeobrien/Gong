#include "EnvelopeFollower.h"

EnvelopeFollower::EnvelopeFollower()
{
    updateCoefficients();
}

void EnvelopeFollower::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    updateCoefficients();
    currentLevel.store(0.0f);
}

void EnvelopeFollower::reset()
{
    currentLevel.store(0.0f);
}

float EnvelopeFollower::process(const juce::AudioBuffer<float>& input)
{
    if (!isEnabled)
        return 0.0f;

    auto numSamples = input.getNumSamples();
    auto numChannels = input.getNumChannels();

    float level = currentLevel.load();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get max absolute value across all channels
        float inputValue = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            float sampleValue = std::abs(input.getSample(channel, sample));
            inputValue = std::max(inputValue, sampleValue);
        }

        // Apply attack/release envelope
        if (inputValue > level)
        {
            // Attack - fast rise
            level = attackCoeff * level + (1.0f - attackCoeff) * inputValue;
        }
        else
        {
            // Release - slow fall
            level = releaseCoeff * level + (1.0f - releaseCoeff) * inputValue;
        }
    }

    currentLevel.store(level);
    return level;
}

float EnvelopeFollower::processSample(float inputSample)
{
    if (!isEnabled)
        return 0.0f;

    float level = currentLevel.load();
    float absInput = std::abs(inputSample);

    if (absInput > level)
    {
        level = attackCoeff * level + (1.0f - attackCoeff) * absInput;
    }
    else
    {
        level = releaseCoeff * level + (1.0f - releaseCoeff) * absInput;
    }

    currentLevel.store(level);
    return level;
}

void EnvelopeFollower::setAttackMs(float ms)
{
    attackMs = juce::jlimit(0.1f, 1000.0f, ms);
    updateCoefficients();
}

void EnvelopeFollower::setReleaseMs(float ms)
{
    releaseMs = juce::jlimit(1.0f, 5000.0f, ms);
    updateCoefficients();
}

void EnvelopeFollower::updateCoefficients()
{
    // Convert time constants to coefficients
    // coefficient = exp(-1 / (time_in_seconds * sample_rate))
    float attackTimeSec = attackMs / 1000.0f;
    float releaseTimeSec = releaseMs / 1000.0f;

    attackCoeff = std::exp(-1.0f / (attackTimeSec * static_cast<float>(sampleRate)));
    releaseCoeff = std::exp(-1.0f / (releaseTimeSec * static_cast<float>(sampleRate)));
}
