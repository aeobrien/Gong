#include "ImpulseGenerator.h"

ImpulseGenerator::ImpulseGenerator()
{
}

void ImpulseGenerator::prepare(double newSampleRate, int newBlockSize)
{
    sampleRate = newSampleRate;
    blockSize = newBlockSize;

    // Scale noise burst length based on sample rate
    // Target ~1ms burst
    noiseBurstLengthSamples = static_cast<int>(sampleRate * 0.001);

    // Prepare filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    filterL.prepare(spec);
    filterR.prepare(spec);

    filterNeedsUpdate = true;
    updateFilter();
}

void ImpulseGenerator::trigger(float velocity)
{
    pendingVelocity.store(mapVelocity(velocity));
    triggerPending.store(true);
}

void ImpulseGenerator::process(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    // Check for pending trigger
    if (triggerPending.exchange(false))
    {
        currentVelocity = pendingVelocity.load();
        samplesRemaining = (impulseType == ImpulseType::SingleSample) ? 1 : noiseBurstLengthSamples;
    }

    if (samplesRemaining <= 0)
    {
        buffer.clear();
        return;
    }

    // Update filter if needed
    if (filterNeedsUpdate)
        updateFilter();

    for (int sample = 0; sample < numSamples && samplesRemaining > 0; ++sample)
    {
        float sampleValue = 0.0f;

        if (impulseType == ImpulseType::SingleSample)
        {
            // Single sample impulse
            sampleValue = currentVelocity;
        }
        else
        {
            // Noise burst with envelope
            float envelope = static_cast<float>(samplesRemaining) / static_cast<float>(noiseBurstLengthSamples);
            envelope = envelope * envelope; // Quadratic decay

            float noise = random.nextFloat() * 2.0f - 1.0f;
            sampleValue = noise * currentVelocity * envelope;
        }

        // Apply filter to left channel
        float filteredL = sampleValue;
        float filteredR = sampleValue;

        if (filterEnabled)
        {
            filteredL = filterL.processSample(0, sampleValue);
            filteredR = filterR.processSample(0, sampleValue);
        }

        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.setSample(channel, sample, (channel == 0) ? filteredL : filteredR);
        }

        --samplesRemaining;
    }

    // Clear remaining samples if impulse finished mid-block
    if (samplesRemaining <= 0)
    {
        int samplesCovered = (impulseType == ImpulseType::SingleSample) ? 1 : noiseBurstLengthSamples;
        int startSample = juce::jmin(samplesCovered, numSamples);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            for (int sample = startSample; sample < numSamples; ++sample)
            {
                buffer.setSample(channel, sample, 0.0f);
            }
        }
    }
}

void ImpulseGenerator::processWithAudioInput(juce::AudioBuffer<float>& output,
                                              const juce::AudioBuffer<float>& audioInput,
                                              float inputGain)
{
    auto numSamples = output.getNumSamples();
    auto numChannels = juce::jmin(output.getNumChannels(), audioInput.getNumChannels());

    // Update filter if needed
    if (filterNeedsUpdate)
        updateFilter();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            int inputChannel = juce::jmin(channel, audioInput.getNumChannels() - 1);
            float inputSample = audioInput.getSample(inputChannel, sample) * inputGain;

            if (filterEnabled)
            {
                if (channel == 0)
                    inputSample = filterL.processSample(0, inputSample);
                else
                    inputSample = filterR.processSample(0, inputSample);
            }

            output.setSample(channel, sample, inputSample);
        }
    }

    // Handle mono input to stereo output
    if (numChannels == 1 && output.getNumChannels() > 1)
    {
        for (int channel = 1; channel < output.getNumChannels(); ++channel)
        {
            output.copyFrom(channel, 0, output, 0, 0, numSamples);
        }
    }
}

void ImpulseGenerator::reset()
{
    samplesRemaining = 0;
    currentVelocity = 0.0f;
    triggerPending.store(false);
    filterL.reset();
    filterR.reset();
}

void ImpulseGenerator::setFilterEnabled(bool enabled)
{
    filterEnabled = enabled;
}

void ImpulseGenerator::setFilterCutoff(float frequencyHz)
{
    filterCutoff = juce::jlimit(20.0f, 20000.0f, frequencyHz);
    filterNeedsUpdate = true;
}

void ImpulseGenerator::setFilterResonance(float resonance)
{
    filterResonance = juce::jlimit(0.1f, 10.0f, resonance);
    filterNeedsUpdate = true;
}

void ImpulseGenerator::setFilterType(FilterType type)
{
    filterType = type;
    filterNeedsUpdate = true;
}

void ImpulseGenerator::setVelocityCurve(float curve)
{
    velocityCurve = juce::jlimit(0.1f, 4.0f, curve);
}

float ImpulseGenerator::mapVelocity(float velocity) const
{
    velocity = juce::jlimit(0.0f, 1.0f, velocity);
    return std::pow(velocity, velocityCurve);
}

void ImpulseGenerator::updateFilter()
{
    filterNeedsUpdate = false;

    // Set filter type
    switch (filterType)
    {
        case FilterType::Lowpass:
            filterL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            filterR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case FilterType::Highpass:
            filterL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            filterR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case FilterType::Bandpass:
            filterL.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            filterR.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
    }

    // Set cutoff and resonance
    filterL.setCutoffFrequency(filterCutoff);
    filterR.setCutoffFrequency(filterCutoff);
    filterL.setResonance(filterResonance);
    filterR.setResonance(filterResonance);
}
