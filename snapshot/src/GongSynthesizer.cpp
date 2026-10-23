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
    syntheticImpulseGen.prepare(sampleRate, blockSize);
    combinationToneBank.prepare(sampleRate, blockSize);
    crashNoiseGenerator.prepare(sampleRate, blockSize);

    // Allocate processing buffers
    impulseBuffer.setSize(2, blockSize);
    resonatorBuffer.setSize(2, blockSize);
    stereoInputBuffer.setSize(2, blockSize);

    // Calculate holdoff samples
    strikeHoldoffSamples = static_cast<int>(strikeHoldoffMs * sampleRate / 1000.0);

    reset();
}

void GongSynthesizer::releaseResources()
{
    impulseBuffer.setSize(0, 0);
    resonatorBuffer.setSize(0, 0);
    stereoInputBuffer.setSize(0, 0);
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

    // Process MIDI
    if (midiEnabled)
    {
        processMidiMessages(midiMessages);
    }

    // Update energy accumulator (decay)
    energyAccumulator.process(numSamples);

    // Route energy to resonator bank
    resonatorBank.updateEnergy(energyAccumulator);

    // Generate impulse block based on excitation mode
    juce::AudioBuffer<float> impulseBlock(impulseBuffer.getArrayOfWritePointers(), 2, numSamples);

    if (excitationMode == ExcitationMode::AudioInput)
    {
        // === Existing audio input path (unchanged) ===
        processStrikeDetection(audioInput, numSamples);

        // Use pre-allocated stereo input buffer (no heap alloc on audio thread)
        int inputChannels = audioInput.getNumChannels();
        if (stereoInputBuffer.getNumSamples() < numSamples)
            stereoInputBuffer.setSize(2, numSamples, false, false, true);

        if (inputChannels >= 2)
        {
            stereoInputBuffer.copyFrom(0, 0, audioInput, 0, 0, numSamples);
            stereoInputBuffer.copyFrom(1, 0, audioInput, 1, 0, numSamples);
        }
        else if (inputChannels == 1)
        {
            stereoInputBuffer.copyFrom(0, 0, audioInput, 0, 0, numSamples);
            stereoInputBuffer.copyFrom(1, 0, audioInput, 0, 0, numSamples);
        }
        else
        {
            stereoInputBuffer.clear();
        }

        // Process through impulse generator (filters the input)
        juce::AudioBuffer<float> stereoInput(stereoInputBuffer.getArrayOfWritePointers(), 2, numSamples);
        impulseGenerator.processWithAudioInput(impulseBlock, stereoInput, inputGain);
    }
    else
    {
        // === Synthetic impulse path ===
        syntheticImpulseGen.process(impulseBlock);
    }

    // Process through resonator bank (same either way)
    juce::AudioBuffer<float> resonatorBlock(resonatorBuffer.getArrayOfWritePointers(), 2, numSamples);
    resonatorBank.process(impulseBlock, resonatorBlock);

    // Step 12: Update and process combination tones
    combinationToneBank.update(energyAccumulator, resonatorBank);
    combinationToneBank.process(resonatorBlock);

    // Step 13: Add crash noise at extreme energy levels
    crashNoiseGenerator.process(energyAccumulator.getNormalizedGlobalEnergy(), resonatorBlock);

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
    samplesSinceLastStrike += numSamples;

    if (peak > strikeThreshold &&
        peak > previousPeak * 1.5f &&
        samplesSinceLastStrike >= strikeHoldoffSamples)
    {
        float velocity = juce::jlimit(0.0f, 1.0f, peak);

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

        if (excitationMode == ExcitationMode::SyntheticImpulse)
        {
            // Route through MidiControllerMock
            bool strikeTriggered = midiControllerMock.processMidiMessage(message);

            // Step 3: Update damping state from CC64
            energyAccumulator.setDampActive(midiControllerMock.isDampingActive());

            if (strikeTriggered)
            {
                // Strike triggered
                const auto& desc = midiControllerMock.getCurrentDescriptor();
                int resIndex = juce::jlimit(0, ResonatorBank::kNumResonators - 1, static_cast<int>(desc.padId));

                // Trigger synthetic impulse
                syntheticImpulseGen.trigger(desc);

                // Inject energy with strike position modeling (Step 10)
                float velocity = desc.velocityNorm();
                injectStrike(velocity, resIndex);

                // Set resonator frequency in snap mode
                auto& resonator = resonatorBank.getResonator(resIndex);
                if (resonator.getFrequencyMode() == SpreadVoiceResonator::FrequencyMode::Snap)
                {
                    resonator.setMidiNote(message.getNoteNumber());
                }
            }
        }
        else
        {
            // Existing MIDI handling for AudioInput mode
            if (message.isNoteOn())
            {
                float velocity = message.getFloatVelocity();
                int noteNumber = message.getNoteNumber();

                int resonatorIndex = noteNumber / 32;
                resonatorIndex = juce::jlimit(0, ResonatorBank::kNumResonators - 1, resonatorIndex);

                injectStrike(velocity, resonatorIndex);

                auto& resonator = resonatorBank.getResonator(resonatorIndex);
                if (resonator.getFrequencyMode() == SpreadVoiceResonator::FrequencyMode::Snap)
                {
                    resonator.setMidiNote(noteNumber);
                }
            }
        }
    }
}

void GongSynthesizer::injectStrike(float velocity, int resonatorIndex)
{
    // Step 10: Strike position modal excitation patterns
    // Maps padId (resonatorIndex) to different modal excitation weights
    static const float positionScales[4][4] = {
        // padId 0 = center: strong fundamental, weak overtones
        { 1.0f, 0.3f, 0.1f, 0.05f },
        // padId 1 = mid-radius: balanced excitation
        { 0.6f, 1.0f, 0.7f, 0.3f },
        // padId 2 = edge: weak fundamental, strong overtones
        { 0.2f, 0.5f, 1.0f, 0.8f },
        // padId 3 = boss (nipple): resonant peak in mid-high
        { 0.4f, 0.8f, 0.4f, 1.0f }
    };

    int posIdx = (resonatorIndex >= 0 && resonatorIndex < 4) ? resonatorIndex : 0;

    for (int band = 0; band < EnergyAccumulator::kNumBands; ++band)
    {
        float scaledVelocity = velocity * positionScales[posIdx][band];
        energyAccumulator.injectEnergy(scaledVelocity, band);
    }
}

void GongSynthesizer::setExcitationMode(ExcitationMode mode)
{
    excitationMode = mode;
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
    syntheticImpulseGen.reset();
    combinationToneBank.reset();
    crashNoiseGenerator.reset();

    impulseBuffer.clear();
    resonatorBuffer.clear();

    samplesSinceLastStrike = strikeHoldoffSamples;
    previousPeak = 0.0f;
}

void GongSynthesizer::panic()
{
    reset();
}
