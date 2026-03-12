#include "GongSynthesizer.h"

GongSynthesizer::GongSynthesizer()
{
    // Configure impulse generator for audio input by default
    impulseGenerator.setExcitationSource(ImpulseGenerator::ExcitationSource::AudioInput);
}

void GongSynthesizer::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    blockSize = samplesPerBlock;

    energyAccumulator.prepare(sampleRate);
    resonatorBank.prepare(sampleRate, blockSize);
    impulseGenerator.prepare(sampleRate, blockSize);

    // Allocate processing buffers
    impulseBuffer.setSize(2, blockSize);
    resonatorBuffer.setSize(2, blockSize);

    // Calculate holdoff samples
    strikeHoldoffSamples = static_cast<int>(strikeHoldoffMs * sampleRate / 1000.0);

    reset();
}

void GongSynthesizer::releaseResources()
{
    impulseBuffer.setSize(0, 0);
    resonatorBuffer.setSize(0, 0);
}

void GongSynthesizer::process(juce::AudioBuffer<float>& outputBuffer,
                               const juce::AudioBuffer<float>& audioInput,
                               juce::MidiBuffer& midiMessages,
                               int numSamples)
{
    // Ensure buffers are correct size
    if (impulseBuffer.getNumSamples() < numSamples)
        impulseBuffer.setSize(2, numSamples, false, false, true);
    if (resonatorBuffer.getNumSamples() < numSamples)
        resonatorBuffer.setSize(2, numSamples, false, false, true);

    // Process MIDI for strikes
    if (midiEnabled)
    {
        processMidiMessages(midiMessages);
    }

    // Process audio input for strike detection
    processStrikeDetection(audioInput, numSamples);

    // Update energy accumulator (decay)
    energyAccumulator.process(numSamples);

    // Route energy to resonator bank
    resonatorBank.updateEnergy(energyAccumulator);

    // Process audio input through impulse generator filter
    juce::AudioBuffer<float> impulseBlock(impulseBuffer.getArrayOfWritePointers(), 2, numSamples);

    // Create stereo input if needed
    int inputChannels = audioInput.getNumChannels();
    juce::AudioBuffer<float> stereoInput(2, numSamples);

    if (inputChannels >= 2)
    {
        stereoInput.copyFrom(0, 0, audioInput, 0, 0, numSamples);
        stereoInput.copyFrom(1, 0, audioInput, 1, 0, numSamples);
    }
    else if (inputChannels == 1)
    {
        stereoInput.copyFrom(0, 0, audioInput, 0, 0, numSamples);
        stereoInput.copyFrom(1, 0, audioInput, 0, 0, numSamples);
    }
    else
    {
        stereoInput.clear();
    }

    // Process through impulse generator (filters the input)
    impulseGenerator.processWithAudioInput(impulseBlock, stereoInput, inputGain);

    // Process through resonator bank
    juce::AudioBuffer<float> resonatorBlock(resonatorBuffer.getArrayOfWritePointers(), 2, numSamples);
    resonatorBank.process(impulseBlock, resonatorBlock);

    // Add to output
    auto outputChannels = outputBuffer.getNumChannels();
    for (int channel = 0; channel < static_cast<int>(outputChannels); ++channel)
    {
        int sourceChannel = juce::jmin(channel, 1);
        outputBuffer.addFrom(channel, 0, resonatorBlock, sourceChannel, 0, numSamples);
    }
}

void GongSynthesizer::processStrikeDetection(const juce::AudioBuffer<float>& audioInput, int numSamples)
{
    // Find peak in this block
    float peak = 0.0f;
    for (int ch = 0; ch < audioInput.getNumChannels(); ++ch)
    {
        float channelPeak = audioInput.getMagnitude(ch, 0, numSamples);
        peak = std::max(peak, channelPeak);
    }

    // Apply input gain to peak
    peak *= inputGain;

    // Detect strike: peak above threshold and rising above previous peak
    // Also check holdoff time
    samplesSinceLastStrike += numSamples;

    if (peak > strikeThreshold &&
        peak > previousPeak * 1.5f &&  // Significant rise
        samplesSinceLastStrike >= strikeHoldoffSamples)
    {
        // Strike detected!
        float velocity = juce::jlimit(0.0f, 1.0f, peak);

        // Inject energy to all bands
        for (int band = 0; band < EnergyAccumulator::kNumBands; ++band)
        {
            energyAccumulator.injectEnergy(velocity, band);
        }

        samplesSinceLastStrike = 0;
    }

    previousPeak = peak;
}

void GongSynthesizer::processMidiMessages(juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            float velocity = message.getFloatVelocity();
            int noteNumber = message.getNoteNumber();

            // Map MIDI note to resonator (4 resonators, spread across keyboard)
            // Notes 0-31 -> Resonator 0
            // Notes 32-63 -> Resonator 1
            // Notes 64-95 -> Resonator 2
            // Notes 96-127 -> Resonator 3
            int resonatorIndex = noteNumber / 32;
            resonatorIndex = juce::jlimit(0, ResonatorBank::kNumResonators - 1, resonatorIndex);

            injectStrike(velocity, resonatorIndex);

            // Also set the resonator frequency based on MIDI note in snap mode
            auto& resonator = resonatorBank.getResonator(resonatorIndex);
            if (resonator.getFrequencyMode() == SpreadVoiceResonator::FrequencyMode::Snap)
            {
                resonator.setMidiNote(noteNumber);
            }
        }
    }
}

void GongSynthesizer::injectStrike(float velocity, int resonatorIndex)
{
    if (resonatorIndex < 0)
    {
        // Inject to all bands
        for (int band = 0; band < EnergyAccumulator::kNumBands; ++band)
        {
            energyAccumulator.injectEnergy(velocity, band);
        }
    }
    else
    {
        // Inject to specific band
        energyAccumulator.injectEnergy(velocity, resonatorIndex);
    }
}

void GongSynthesizer::setStrikeThreshold(float threshold)
{
    strikeThreshold = juce::jlimit(0.01f, 1.0f, threshold);
}

void GongSynthesizer::setStrikeHoldoffMs(float ms)
{
    strikeHoldoffMs = juce::jlimit(10.0f, 500.0f, ms);
    strikeHoldoffSamples = static_cast<int>(strikeHoldoffMs * sampleRate / 1000.0);
}

void GongSynthesizer::setInputGain(float gain)
{
    inputGain = juce::jlimit(0.0f, 10.0f, gain);
}

void GongSynthesizer::setMidiEnabled(bool enabled)
{
    midiEnabled = enabled;
}

void GongSynthesizer::setGlobalDecayTime(float seconds)
{
    resonatorBank.setAllDecayTimes(seconds);
}

void GongSynthesizer::setGlobalBrightness(float brightness)
{
    resonatorBank.setAllBaseBrightness(brightness);
}

void GongSynthesizer::setGlobalSpreadLevel(float level)
{
    resonatorBank.setAllSpreadLevel(level);
}

void GongSynthesizer::setGlobalSpreadPanWidth(float width)
{
    resonatorBank.setAllSpreadPanWidth(width);
}

void GongSynthesizer::reset()
{
    energyAccumulator.reset();
    resonatorBank.reset();
    impulseGenerator.reset();

    impulseBuffer.clear();
    resonatorBuffer.clear();

    samplesSinceLastStrike = strikeHoldoffSamples;  // Allow immediate first strike
    previousPeak = 0.0f;
}

void GongSynthesizer::panic()
{
    reset();
}
