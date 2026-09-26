#include "CombinationToneBank.h"

CombinationToneBank::CombinationToneBank()
{
}

void CombinationToneBank::prepare(double newSampleRate, int blockSize)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    for (auto& tone : tones)
    {
        tone.filterL.prepare(spec);
        tone.filterL.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        tone.filterL.setResonance(5.0f);
        tone.filterR.prepare(spec);
        tone.filterR.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        tone.filterR.setResonance(5.0f);
    }

    isPrepared = true;
}

void CombinationToneBank::reset()
{
    for (auto& tone : tones)
    {
        tone.filterL.reset();
        tone.filterR.reset();
        tone.gain = 0.0f;
    }
}

void CombinationToneBank::update(const EnergyAccumulator& energy, const ResonatorBank& resonators)
{
    if (!isPrepared) return;

    float globalE = energy.getNormalizedGlobalEnergy();
    if (globalE < activationThreshold)
    {
        // Below threshold: silence all combination tones
        for (auto& tone : tones)
            tone.gain = 0.0f;
        return;
    }

    // Find the 3 strongest bands
    struct BandInfo { int idx; float energy; float freq; };
    std::array<BandInfo, 4> bands;
    for (int i = 0; i < 4; ++i)
    {
        bands[i] = { i, energy.getNormalizedBandEnergy(i),
                      resonators.getResonator(i).getEffectiveFrequency() };
    }

    // Sort by energy descending
    std::sort(bands.begin(), bands.end(),
              [](const BandInfo& a, const BandInfo& b) { return a.energy > b.energy; });

    // Compute combination tones from top 3 modes
    int toneIdx = 0;
    float activationScale = (globalE - activationThreshold) / (1.0f - activationThreshold);

    for (int i = 0; i < 3 && toneIdx < kNumTones; ++i)
    {
        for (int j = i + 1; j < 3 && toneIdx < kNumTones; ++j)
        {
            float f1 = bands[i].freq;
            float f2 = bands[j].freq;
            float combinedEnergy = bands[i].energy * bands[j].energy;

            // Sum tone
            float sumFreq = f1 + f2;
            if (sumFreq > 20.0f && sumFreq < sampleRate * 0.45f)
            {
                tones[toneIdx].freq = sumFreq;
                tones[toneIdx].gain = combinedEnergy * activationScale * mixLevel;
                tones[toneIdx].filterL.setCutoffFrequency(sumFreq);
                tones[toneIdx].filterR.setCutoffFrequency(sumFreq);
                toneIdx++;
            }

            // Difference tone
            if (toneIdx < kNumTones)
            {
                float diffFreq = std::abs(f1 - f2);
                if (diffFreq > 20.0f && diffFreq < sampleRate * 0.45f)
                {
                    tones[toneIdx].freq = diffFreq;
                    tones[toneIdx].gain = combinedEnergy * activationScale * mixLevel * 0.7f;
                    tones[toneIdx].filterL.setCutoffFrequency(diffFreq);
                    tones[toneIdx].filterR.setCutoffFrequency(diffFreq);
                    toneIdx++;
                }
            }
        }
    }

    // Zero remaining tones
    for (; toneIdx < kNumTones; ++toneIdx)
        tones[toneIdx].gain = 0.0f;
}

void CombinationToneBank::process(juce::AudioBuffer<float>& buffer)
{
    if (!isPrepared) return;

    auto numSamples = buffer.getNumSamples();

    for (auto& tone : tones)
    {
        if (tone.gain < 0.001f) continue;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float inL = buffer.getSample(0, sample);
            float inR = buffer.getNumChannels() > 1 ? buffer.getSample(1, sample) : inL;

            float outL = tone.filterL.processSample(0, inL) * tone.gain;
            float outR = tone.filterR.processSample(0, inR) * tone.gain;

            buffer.addSample(0, sample, outL);
            if (buffer.getNumChannels() > 1)
                buffer.addSample(1, sample, outR);
        }
    }
}

void CombinationToneBank::setMixLevel(float level)
{
    mixLevel = juce::jlimit(0.0f, 1.0f, level);
}

void CombinationToneBank::setActivationThreshold(float threshold)
{
    activationThreshold = juce::jlimit(0.0f, 1.0f, threshold);
}
