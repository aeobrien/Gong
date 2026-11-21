#include "MultibandCompressor.h"

MultibandCompressor::MultibandCompressor()
{
    // Set filter types
    lowMidLowpassL.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    lowMidLowpassR.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    lowMidHighpassL.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    lowMidHighpassR.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    midHighLowpassL.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    midHighLowpassR.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    midHighHighpassL.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    midHighHighpassR.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
}

void MultibandCompressor::prepare(double newSampleRate, int blockSize)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    // Prepare crossover filters
    lowMidLowpassL.prepare(spec);
    lowMidLowpassR.prepare(spec);
    lowMidHighpassL.prepare(spec);
    lowMidHighpassR.prepare(spec);

    midHighLowpassL.prepare(spec);
    midHighLowpassR.prepare(spec);
    midHighHighpassL.prepare(spec);
    midHighHighpassR.prepare(spec);

    // Prepare compressors
    for (int i = 0; i < kNumBands; ++i)
    {
        compressorsL[i].prepare(spec);
        compressorsR[i].prepare(spec);

        // Set compressor parameters
        compressorsL[i].setThreshold(bandSettings[i].threshold);
        compressorsL[i].setRatio(bandSettings[i].ratio);
        compressorsL[i].setAttack(bandSettings[i].attackMs);
        compressorsL[i].setRelease(bandSettings[i].releaseMs);

        compressorsR[i].setThreshold(bandSettings[i].threshold);
        compressorsR[i].setRatio(bandSettings[i].ratio);
        compressorsR[i].setAttack(bandSettings[i].attackMs);
        compressorsR[i].setRelease(bandSettings[i].releaseMs);
    }

    updateCrossovers();
}

void MultibandCompressor::process(juce::AudioBuffer<float>& buffer)
{
    if (!isEnabled)
        return;

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    if (numChannels < 2)
        return;

    float outputGainLinear = juce::Decibels::decibelsToGain(outputGainDb);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float inputL = buffer.getSample(0, sample);
        float inputR = buffer.getSample(1, sample);

        // Split into bands
        // Low band: lowpass at lowMidCrossover
        float lowL = lowMidLowpassL.processSample(0, inputL);
        float lowR = lowMidLowpassR.processSample(0, inputR);

        // Get high-passed signal (mid + high)
        float midHighL = lowMidHighpassL.processSample(0, inputL);
        float midHighR = lowMidHighpassR.processSample(0, inputR);

        // Mid band: lowpass the mid+high at midHighCrossover
        float midL = midHighLowpassL.processSample(0, midHighL);
        float midR = midHighLowpassR.processSample(0, midHighR);

        // High band: highpass the mid+high at midHighCrossover
        float highL = midHighHighpassL.processSample(0, midHighL);
        float highR = midHighHighpassR.processSample(0, midHighR);

        // Process each band through compressors
        float bands[kNumBands][2] = {
            {lowL, lowR},
            {midL, midR},
            {highL, highR}
        };

        float outputL = 0.0f;
        float outputR = 0.0f;

        for (int band = 0; band < kNumBands; ++band)
        {
            float bandL = bands[band][0];
            float bandR = bands[band][1];

            if (bandSettings[band].enabled)
            {
                bandL = compressorsL[band].processSample(0, bandL);
                bandR = compressorsR[band].processSample(0, bandR);

                // Apply makeup gain
                float makeupGain = juce::Decibels::decibelsToGain(bandSettings[band].makeupGainDb);
                bandL *= makeupGain;
                bandR *= makeupGain;
            }

            outputL += bandL;
            outputR += bandR;
        }

        // Apply output gain and write back
        buffer.setSample(0, sample, outputL * outputGainLinear);
        buffer.setSample(1, sample, outputR * outputGainLinear);
    }
}

void MultibandCompressor::reset()
{
    lowMidLowpassL.reset();
    lowMidLowpassR.reset();
    lowMidHighpassL.reset();
    lowMidHighpassR.reset();

    midHighLowpassL.reset();
    midHighLowpassR.reset();
    midHighHighpassL.reset();
    midHighHighpassR.reset();

    for (int i = 0; i < kNumBands; ++i)
    {
        compressorsL[i].reset();
        compressorsR[i].reset();
    }
}

void MultibandCompressor::setEnabled(bool enabled)
{
    isEnabled = enabled;
}

void MultibandCompressor::setLowMidCrossover(float hz)
{
    lowMidCrossover = juce::jlimit(50.0f, 500.0f, hz);
    updateCrossovers();
}

void MultibandCompressor::setMidHighCrossover(float hz)
{
    midHighCrossover = juce::jlimit(500.0f, 5000.0f, hz);
    updateCrossovers();
}

void MultibandCompressor::setBandSettings(int band, const BandSettings& settings)
{
    if (band >= 0 && band < kNumBands)
    {
        bandSettings[band] = settings;

        // Update compressor parameters
        compressorsL[band].setThreshold(settings.threshold);
        compressorsL[band].setRatio(settings.ratio);
        compressorsL[band].setAttack(settings.attackMs);
        compressorsL[band].setRelease(settings.releaseMs);

        compressorsR[band].setThreshold(settings.threshold);
        compressorsR[band].setRatio(settings.ratio);
        compressorsR[band].setAttack(settings.attackMs);
        compressorsR[band].setRelease(settings.releaseMs);
    }
}

const MultibandCompressor::BandSettings& MultibandCompressor::getBandSettings(int band) const
{
    return bandSettings[juce::jlimit(0, kNumBands - 1, band)];
}

void MultibandCompressor::setAllThresholds(float db)
{
    for (int i = 0; i < kNumBands; ++i)
    {
        bandSettings[i].threshold = db;
        compressorsL[i].setThreshold(db);
        compressorsR[i].setThreshold(db);
    }
}

void MultibandCompressor::setAllRatios(float ratio)
{
    for (int i = 0; i < kNumBands; ++i)
    {
        bandSettings[i].ratio = ratio;
        compressorsL[i].setRatio(ratio);
        compressorsR[i].setRatio(ratio);
    }
}

void MultibandCompressor::setAllAttacks(float ms)
{
    for (int i = 0; i < kNumBands; ++i)
    {
        bandSettings[i].attackMs = ms;
        compressorsL[i].setAttack(ms);
        compressorsR[i].setAttack(ms);
    }
}

void MultibandCompressor::setAllReleases(float ms)
{
    for (int i = 0; i < kNumBands; ++i)
    {
        bandSettings[i].releaseMs = ms;
        compressorsL[i].setRelease(ms);
        compressorsR[i].setRelease(ms);
    }
}

void MultibandCompressor::setOutputGainDb(float db)
{
    outputGainDb = juce::jlimit(-24.0f, 24.0f, db);
}

void MultibandCompressor::updateCrossovers()
{
    lowMidLowpassL.setCutoffFrequency(lowMidCrossover);
    lowMidLowpassR.setCutoffFrequency(lowMidCrossover);
    lowMidHighpassL.setCutoffFrequency(lowMidCrossover);
    lowMidHighpassR.setCutoffFrequency(lowMidCrossover);

    midHighLowpassL.setCutoffFrequency(midHighCrossover);
    midHighLowpassR.setCutoffFrequency(midHighCrossover);
    midHighHighpassL.setCutoffFrequency(midHighCrossover);
    midHighHighpassR.setCutoffFrequency(midHighCrossover);
}
