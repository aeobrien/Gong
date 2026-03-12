#include "ResonatorBank.h"

ResonatorBank::ResonatorBank()
{
    // Set default frequencies for each resonator (spread across frequency range)
    // These create a nice fundamental + harmonic spread
    resonators[0].setFrequencyHz(110.0f);   // A2
    resonators[1].setFrequencyHz(220.0f);   // A3 (octave)
    resonators[2].setFrequencyHz(330.0f);   // E4 (fifth)
    resonators[3].setFrequencyHz(440.0f);   // A4 (double octave)

    // Set corresponding MIDI notes for snap mode
    resonators[0].setMidiNote(45);  // A2
    resonators[1].setMidiNote(57);  // A3
    resonators[2].setMidiNote(64);  // E4
    resonators[3].setMidiNote(69);  // A4
}

void ResonatorBank::prepare(double sampleRate, int blockSize)
{
    for (int i = 0; i < kNumResonators; ++i)
    {
        resonators[i].prepare(sampleRate, blockSize);
    }
}

void ResonatorBank::process(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output)
{
    output.clear();

    // Apply master gain
    float masterGainLinear = juce::Decibels::decibelsToGain(masterGainDb);

    // Temporary buffer for each resonator's output
    juce::AudioBuffer<float> tempBuffer(output.getNumChannels(), output.getNumSamples());

    for (int i = 0; i < kNumResonators; ++i)
    {
        if (!resonators[i].getEnabled())
            continue;

        tempBuffer.clear();
        resonators[i].process(input, tempBuffer);

        // Add to output with master gain
        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            output.addFrom(ch, 0, tempBuffer, ch, 0, output.getNumSamples(), masterGainLinear);
        }
    }
}

void ResonatorBank::reset()
{
    for (int i = 0; i < kNumResonators; ++i)
    {
        resonators[i].reset();
    }
}

SpreadVoiceResonator& ResonatorBank::getResonator(int index)
{
    return resonators[juce::jlimit(0, kNumResonators - 1, index)];
}

const SpreadVoiceResonator& ResonatorBank::getResonator(int index) const
{
    return resonators[juce::jlimit(0, kNumResonators - 1, index)];
}

void ResonatorBank::updateEnergy(const EnergyAccumulator& accumulator)
{
    // Route per-band energy to each resonator
    for (int i = 0; i < kNumResonators; ++i)
    {
        float energy = accumulator.getNormalizedBandEnergy(i);
        resonators[i].setEnergy(energy);
    }
}

void ResonatorBank::setAllDecayTimes(float seconds)
{
    for (int i = 0; i < kNumResonators; ++i)
    {
        resonators[i].setDecayTime(seconds);
    }
}

void ResonatorBank::setAllBaseBrightness(float brightness)
{
    for (int i = 0; i < kNumResonators; ++i)
    {
        resonators[i].setBaseBrightness(brightness);
    }
}

void ResonatorBank::setAllSpreadLevel(float level)
{
    for (int i = 0; i < kNumResonators; ++i)
    {
        resonators[i].setSpreadLevel(level);
    }
}

void ResonatorBank::setAllSpreadPanWidth(float width)
{
    for (int i = 0; i < kNumResonators; ++i)
    {
        resonators[i].setSpreadPanWidth(width);
    }
}

void ResonatorBank::setMasterGainDb(float db)
{
    masterGainDb = juce::jlimit(-24.0f, 24.0f, db);
}
