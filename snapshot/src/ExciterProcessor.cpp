#include "ExciterProcessor.h"

ExciterProcessor::ExciterProcessor()
{
    highpassL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    highpassR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
}

void ExciterProcessor::prepare(double newSampleRate, int blockSize)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    highpassL.prepare(spec);
    highpassR.prepare(spec);

    spec.numChannels = 2;
    mixer.prepare(spec);
    mixer.setWetMixProportion(dryWetMix);

    updateHighpass();
}

void ExciterProcessor::process(juce::AudioBuffer<float>& buffer)
{
    // When disabled, pass through unchanged (don't block audio)
    if (!isEnabled)
        return;

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    if (numChannels < 2)
        return;

    // Push dry signal to mixer
    juce::dsp::AudioBlock<float> block(buffer);
    mixer.pushDrySamples(block);

    // Apply output gain
    float outputGainLinear = juce::Decibels::decibelsToGain(outputGainDb);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Left channel
        float sampleL = buffer.getSample(0, sample);
        // Highpass to get only high frequency content
        float filteredL = highpassL.processSample(0, sampleL);
        // Apply drive and tanh saturation
        float saturatedL = std::tanh(filteredL * saturationDrive);
        buffer.setSample(0, sample, saturatedL * outputGainLinear);

        // Right channel
        float sampleR = buffer.getSample(1, sample);
        float filteredR = highpassR.processSample(0, sampleR);
        float saturatedR = std::tanh(filteredR * saturationDrive);
        buffer.setSample(1, sample, saturatedR * outputGainLinear);
    }

    // Mix with dry signal
    mixer.mixWetSamples(block);
}

void ExciterProcessor::reset()
{
    highpassL.reset();
    highpassR.reset();
    mixer.reset();
}

void ExciterProcessor::setEnabled(bool enabled)
{
    isEnabled = enabled;
}

void ExciterProcessor::setHighpassFrequency(float hz)
{
    highpassFreq = juce::jlimit(100.0f, 5000.0f, hz);
    updateHighpass();
}

void ExciterProcessor::setSaturationDrive(float drive)
{
    saturationDrive = juce::jlimit(1.0f, 10.0f, drive);
}

void ExciterProcessor::setDryWetMix(float wet)
{
    dryWetMix = juce::jlimit(0.0f, 1.0f, wet);
    mixer.setWetMixProportion(dryWetMix);
}

void ExciterProcessor::setOutputGainDb(float db)
{
    outputGainDb = juce::jlimit(-24.0f, 12.0f, db);
}

void ExciterProcessor::updateHighpass()
{
    highpassL.setCutoffFrequency(highpassFreq);
    highpassR.setCutoffFrequency(highpassFreq);
}
