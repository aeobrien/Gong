#include "SyntheticImpulseGenerator.h"

SyntheticImpulseGenerator::SyntheticImpulseGenerator()
{
}

void SyntheticImpulseGenerator::prepare(double newSampleRate, int newBlockSize)
{
    sampleRate = newSampleRate;
    blockSize = newBlockSize;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    noiseFilter.prepare(spec);
    noiseFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filterPrepared = true;

    reset();
}

void SyntheticImpulseGenerator::trigger(const StrikeDescriptor& descriptor)
{
    juce::SpinLock::ScopedLockType lock(descriptorLock);
    pendingDescriptor = descriptor;
    triggerPending.store(true);
}

void SyntheticImpulseGenerator::process(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    // Check for pending trigger
    if (triggerPending.exchange(false))
    {
        StrikeDescriptor desc;
        {
            juce::SpinLock::ScopedLockType lock(descriptorLock);
            desc = pendingDescriptor;
        }

        // Map velocity with power curve (gamma = 2.0 for natural feel)
        float velNorm = desc.velocityNorm();
        amplitude = std::pow(velNorm, 2.0f);

        // Step 9: Hertz contact law - velocity-dependent contact time
        // Contact time inversely proportional to velocity (harder = shorter = brighter)
        float baseContactTimeMs = 2.0f;
        float velocityForContact = std::max(0.01f, velNorm);
        float contactTimeMs = baseContactTimeMs * std::pow(velocityForContact, -0.2f);

        // Attack phase influenced by both attackSlope CC and contact time
        float attackMs = (1.0f - desc.attackSlopeNorm()) * contactTimeMs;
        attackSamples = std::max(1, static_cast<int>(attackMs * sampleRate / 1000.0));

        // Decay phase: decayShape 0 = ~0.5ms, 255 = ~5ms
        float decayMs = 0.5f + desc.decayShapeNorm() * 4.5f;
        decaySamples = std::max(1, static_cast<int>(decayMs * sampleRate / 1000.0));

        totalSamples = attackSamples + decaySamples;
        samplesRemaining = totalSamples;

        // HF energy ratio: mix between filtered and unfiltered noise
        hfMix = desc.hfEnergyRatioNorm();

        // Step 9: Velocity-dependent filter cutoff (Hertz contact law)
        // Bandwidth proportional to 1/contactTime - harder strikes = brighter
        float bandwidthHz = 1000.0f / contactTimeMs;
        float baseCutoff = desc.spectralCentroidHz();
        // Blend base cutoff with velocity-derived bandwidth
        float velocityCutoff = baseCutoff + bandwidthHz * velNorm;

        // Configure filter
        if (filterPrepared)
        {
            float cutoff = juce::jlimit(200.0f, 15000.0f, velocityCutoff);
            noiseFilter.setCutoffFrequency(cutoff);

            // Q varies with hfEnergyRatio: low ratio = higher Q for more resonant filtering
            float q = 0.707f + (1.0f - hfMix) * 2.0f;
            noiseFilter.setResonance(q);
            noiseFilter.reset();
        }
    }

    if (samplesRemaining <= 0)
    {
        buffer.clear();
        return;
    }

    int sampleIndex = totalSamples - samplesRemaining;

    for (int sample = 0; sample < numSamples && samplesRemaining > 0; ++sample, ++sampleIndex, --samplesRemaining)
    {
        // Generate white noise
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Filter the noise
        float filteredNoise = filterPrepared ? noiseFilter.processSample(0, noise) : noise;

        // Mix filtered and unfiltered based on hfEnergyRatio
        float shaped = filteredNoise * (1.0f - hfMix) + noise * hfMix;

        // Compute envelope
        float envelope = 0.0f;
        if (sampleIndex < attackSamples)
        {
            // Attack ramp (linear)
            envelope = static_cast<float>(sampleIndex + 1) / static_cast<float>(attackSamples);
        }
        else
        {
            // Decay (quadratic)
            int decayPos = sampleIndex - attackSamples;
            float decayProgress = static_cast<float>(decayPos) / static_cast<float>(decaySamples);
            envelope = (1.0f - decayProgress) * (1.0f - decayProgress);
        }

        float sampleValue = shaped * amplitude * envelope;

        for (int channel = 0; channel < numChannels; ++channel)
            buffer.setSample(channel, sample, sampleValue);
    }

    // Clear remaining samples if impulse finished mid-block
    if (samplesRemaining <= 0)
    {
        int samplesWritten = juce::jmin(totalSamples, numSamples);
        for (int channel = 0; channel < numChannels; ++channel)
        {
            for (int sample = samplesWritten; sample < numSamples; ++sample)
                buffer.setSample(channel, sample, 0.0f);
        }
    }
}

void SyntheticImpulseGenerator::reset()
{
    samplesRemaining = 0;
    totalSamples = 0;
    amplitude = 0.0f;
    triggerPending.store(false);

    if (filterPrepared)
        noiseFilter.reset();
}
