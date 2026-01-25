#include "ResonatorSynth.h"

ResonatorSynth::ResonatorSynth()
{
    // Add the sound
    addSound(new ResonatorSound());

    // Add voices
    for (int i = 0; i < numVoices; ++i)
    {
        addVoice(new ResonatorVoice());
    }

    // Configure audio input resonator
    audioInputResonator.setDecayTime(currentDecayTime);
    audioInputResonator.setBrightness(currentBrightness);
    audioInputResonator.setNumActiveModes(currentNumModes);
    audioInputResonator.setFundamentalFrequency(audioInputFundamental);

    // Configure audio input impulse generator for audio passthrough
    audioInputImpulseGen.setExcitationSource(ImpulseGenerator::ExcitationSource::AudioInput);
}

void ResonatorSynth::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }

    // Prepare audio input resonator
    audioInputResonator.prepare(sampleRate, samplesPerBlock);
    audioInputImpulseGen.prepare(sampleRate, samplesPerBlock);
    audioInputImpulseBuffer.setSize(2, samplesPerBlock);
    audioInputResonatorBuffer.setSize(2, samplesPerBlock);
}

void ResonatorSynth::setDecayTime(float decaySeconds)
{
    currentDecayTime = decaySeconds;
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setDecayTime(decaySeconds);
        }
    }
    audioInputResonator.setDecayTime(decaySeconds);
}

void ResonatorSynth::setBrightness(float brightness)
{
    currentBrightness = brightness;
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setBrightness(brightness);
        }
    }
    audioInputResonator.setBrightness(brightness);
}

void ResonatorSynth::setNumModes(int numModes)
{
    currentNumModes = numModes;
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setNumModes(numModes);
        }
    }
    audioInputResonator.setNumActiveModes(numModes);
}

void ResonatorSynth::setExcitationSource(ImpulseGenerator::ExcitationSource source)
{
    currentExcitationSource = source;
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setExcitationSource(source);
        }
    }
}

ImpulseGenerator::ExcitationSource ResonatorSynth::getExcitationSource() const
{
    return currentExcitationSource;
}

void ResonatorSynth::setFilterEnabled(bool enabled)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setFilterEnabled(enabled);
        }
    }
}

void ResonatorSynth::setFilterCutoff(float frequencyHz)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setFilterCutoff(frequencyHz);
        }
    }
    audioInputImpulseGen.setFilterCutoff(frequencyHz);
}

void ResonatorSynth::setFilterResonance(float resonance)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setFilterResonance(resonance);
        }
    }
    audioInputImpulseGen.setFilterResonance(resonance);
}

void ResonatorSynth::setFilterType(ImpulseGenerator::FilterType type)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setFilterType(type);
        }
    }
    audioInputImpulseGen.setFilterType(type);
}

void ResonatorSynth::setVelocityCurve(float curve)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setVelocityCurve(curve);
        }
    }
}

void ResonatorSynth::setModeFrequencyRatio(int mode, float ratio)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setModeFrequencyRatio(mode, ratio);
        }
    }
    audioInputResonator.setModeFrequencyRatio(mode, ratio);
}

void ResonatorSynth::setModeDecayMultiplier(int mode, float multiplier)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setModeDecayMultiplier(mode, multiplier);
        }
    }
    audioInputResonator.setModeDecayMultiplier(mode, multiplier);
}

void ResonatorSynth::setModeGainDb(int mode, float gainDb)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setModeGainDb(mode, gainDb);
        }
    }
    audioInputResonator.setModeGainDb(mode, gainDb);
}

void ResonatorSynth::setModeDetuneCents(int mode, float cents)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setModeDetuneCents(mode, cents);
        }
    }
    audioInputResonator.setModeDetuneCents(mode, cents);
}

void ResonatorSynth::setModeStereoWidth(int mode, float width)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setModeStereoWidth(mode, width);
        }
    }
    audioInputResonator.setModeStereoWidth(mode, width);
}

void ResonatorSynth::setModeEnabled(int mode, bool enabled)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->setModeEnabled(mode, enabled);
        }
    }
    audioInputResonator.setModeEnabled(mode, enabled);
}

void ResonatorSynth::setMasterGainDb(float gainDb)
{
    currentMasterGainDb = gainDb;
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(getVoice(i)))
        {
            voice->getResonatorMutable().setMasterGainDb(gainDb);
        }
    }
    audioInputResonator.setMasterGainDb(gainDb);
}

void ResonatorSynth::setAudioInputFundamental(float frequencyHz)
{
    audioInputFundamental = juce::jlimit(20.0f, 2000.0f, frequencyHz);
    audioInputResonator.setFundamentalFrequency(audioInputFundamental);
}

void ResonatorSynth::processWithAudioInput(juce::AudioBuffer<float>& outputBuffer,
                                            const juce::AudioBuffer<float>& audioInput,
                                            juce::MidiBuffer& midiMessages,
                                            int numSamples,
                                            float inputGain)
{
    if (currentExcitationSource != ImpulseGenerator::ExcitationSource::AudioInput)
    {
        // Not in audio input mode, just render normally
        renderNextBlock(outputBuffer, midiMessages, 0, numSamples);
        return;
    }

    // Audio input mode - use dedicated audio input resonator (no MIDI required)
    auto outputChannels = outputBuffer.getNumChannels();
    auto inputChannels = audioInput.getNumChannels();

    // Ensure buffers are correct size (always stereo for processing)
    if (audioInputImpulseBuffer.getNumSamples() < numSamples)
        audioInputImpulseBuffer.setSize(2, numSamples, false, false, true);
    if (audioInputResonatorBuffer.getNumSamples() < numSamples)
        audioInputResonatorBuffer.setSize(2, numSamples, false, false, true);

    // Create a stereo input buffer - duplicate mono to both channels if needed
    juce::AudioBuffer<float> stereoInput(2, numSamples);
    if (inputChannels >= 2)
    {
        // Stereo input - copy both channels
        stereoInput.copyFrom(0, 0, audioInput, 0, 0, numSamples);
        stereoInput.copyFrom(1, 0, audioInput, 1, 0, numSamples);
    }
    else if (inputChannels == 1)
    {
        // Mono input - duplicate to both channels
        stereoInput.copyFrom(0, 0, audioInput, 0, 0, numSamples);
        stereoInput.copyFrom(1, 0, audioInput, 0, 0, numSamples);
    }
    else
    {
        stereoInput.clear();
    }

    // Process audio input through impulse generator filter (always stereo)
    juce::AudioBuffer<float> impulseBlock(audioInputImpulseBuffer.getArrayOfWritePointers(), 2, numSamples);
    audioInputImpulseGen.processWithAudioInput(impulseBlock, stereoInput, inputGain);

    // Process through dedicated audio input resonator (always stereo)
    juce::AudioBuffer<float> resonatorBlock(audioInputResonatorBuffer.getArrayOfWritePointers(), 2, numSamples);
    audioInputResonator.process(impulseBlock, resonatorBlock);

    // Add to output (stereo to however many output channels we have)
    for (int channel = 0; channel < static_cast<int>(outputChannels); ++channel)
    {
        int sourceChannel = juce::jmin(channel, 1); // Clamp to stereo source
        outputBuffer.addFrom(channel, 0, resonatorBlock, sourceChannel, 0, numSamples);
    }
}

void ResonatorSynth::panic()
{
    allNotesOff(0, false); // Channel 0, don't allow tail off
}

const Resonator* ResonatorSynth::getFirstVoiceResonator() const
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ResonatorVoice*>(const_cast<ResonatorSynth*>(this)->getVoice(i)))
        {
            return &voice->getResonator();
        }
    }
    return nullptr;
}
