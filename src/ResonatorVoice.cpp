#include "ResonatorVoice.h"

ResonatorVoice::ResonatorVoice()
{
    resonator.setDecayTime(4.0f);
    resonator.setBrightness(0.6f);
    resonator.setNumActiveModes(6);
}

bool ResonatorVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<ResonatorSound*>(sound) != nullptr;
}

void ResonatorVoice::startNote(int midiNoteNumber, float velocity,
                                juce::SynthesiserSound* /*sound*/,
                                int /*currentPitchWheelPosition*/)
{
    currentNote = midiNoteNumber;
    noteIsOn = true;
    samplesSinceNoteOn = 0;

    // Set resonator frequency based on MIDI note
    float frequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
    resonator.setFundamentalFrequency(frequency);
    resonator.reset(); // Clear any existing resonance

    // Only trigger impulse if using internal noise source
    if (impulseGenerator.getExcitationSource() == ImpulseGenerator::ExcitationSource::InternalNoise)
    {
        impulseGenerator.trigger(velocity);
    }

    // Calculate decay samples (how long the voice stays active)
    decaySamples = static_cast<int>(resonator.getDecayTime() * getSampleRate() * 1.5);
}

void ResonatorVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        // Allow natural decay
        noteIsOn = false;
    }
    else
    {
        // Immediate stop
        clearCurrentNote();
        currentNote = -1;
        noteIsOn = false;
        samplesSinceNoteOn = 0;
    }
}

void ResonatorVoice::pitchWheelMoved(int /*newPitchWheelValue*/)
{
    // Could implement pitch bend here
}

void ResonatorVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/)
{
    // Could implement CC control here
}

void ResonatorVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                      int startSample, int numSamples)
{
    if (currentNote < 0)
        return;

    // Check if voice should be released (natural decay finished)
    // In audio input mode, don't auto-stop while note is held - only stop after note-off + decay
    bool isAudioInputMode = (impulseGenerator.getExcitationSource() == ImpulseGenerator::ExcitationSource::AudioInput);

    if (!noteIsOn && samplesSinceNoteOn > decaySamples)
    {
        clearCurrentNote();
        currentNote = -1;
        samplesSinceNoteOn = 0;
        return;
    }

    // In audio input mode with note held, don't increment decay counter
    // (voice stays active indefinitely while note is held)
    if (isAudioInputMode && noteIsOn)
    {
        samplesSinceNoteOn = 0; // Reset decay counter while note is held
    }

    auto numChannels = outputBuffer.getNumChannels();

    // Ensure buffers are correct size
    if (impulseBuffer.getNumSamples() < numSamples)
        impulseBuffer.setSize(2, numSamples, false, false, true);
    if (resonatorBuffer.getNumSamples() < numSamples)
        resonatorBuffer.setSize(2, numSamples, false, false, true);

    // 1. Generate impulse (only for internal noise source)
    juce::AudioBuffer<float> impulseBlock(impulseBuffer.getArrayOfWritePointers(),
                                          juce::jmin(2, numChannels), 0, numSamples);

    if (impulseGenerator.getExcitationSource() == ImpulseGenerator::ExcitationSource::InternalNoise)
    {
        impulseGenerator.process(impulseBlock);
    }
    else
    {
        // Audio input mode - impulse will be provided externally
        impulseBlock.clear();
    }

    // 2. Process through resonator
    juce::AudioBuffer<float> resonatorBlock(resonatorBuffer.getArrayOfWritePointers(),
                                            juce::jmin(2, numChannels), 0, numSamples);
    resonator.process(impulseBlock, resonatorBlock);

    // 3. Add to output (mixing with other voices)
    for (int channel = 0; channel < numChannels; ++channel)
    {
        int sourceChannel = juce::jmin(channel, 1); // Clamp to stereo
        outputBuffer.addFrom(channel, startSample, resonatorBlock, sourceChannel, 0, numSamples);
    }

    samplesSinceNoteOn += numSamples;
}

void ResonatorVoice::processWithAudioInput(juce::AudioBuffer<float>& outputBuffer,
                                            const juce::AudioBuffer<float>& audioInput,
                                            int startSample, int numSamples,
                                            float inputGain)
{
    if (currentNote < 0)
        return;

    auto numChannels = outputBuffer.getNumChannels();

    // Ensure buffers are correct size
    if (impulseBuffer.getNumSamples() < numSamples)
        impulseBuffer.setSize(2, numSamples, false, false, true);
    if (resonatorBuffer.getNumSamples() < numSamples)
        resonatorBuffer.setSize(2, numSamples, false, false, true);

    // 1. Process audio input through impulse generator filter
    juce::AudioBuffer<float> impulseBlock(impulseBuffer.getArrayOfWritePointers(),
                                          juce::jmin(2, numChannels), 0, numSamples);
    impulseGenerator.processWithAudioInput(impulseBlock, audioInput, inputGain);

    // 2. Process through resonator
    juce::AudioBuffer<float> resonatorBlock(resonatorBuffer.getArrayOfWritePointers(),
                                            juce::jmin(2, numChannels), 0, numSamples);
    resonator.process(impulseBlock, resonatorBlock);

    // 3. Add to output
    for (int channel = 0; channel < numChannels; ++channel)
    {
        int sourceChannel = juce::jmin(channel, 1);
        outputBuffer.addFrom(channel, startSample, resonatorBlock, sourceChannel, 0, numSamples);
    }

    samplesSinceNoteOn += numSamples;
}

void ResonatorVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    setCurrentPlaybackSampleRate(sampleRate);

    impulseGenerator.prepare(sampleRate, samplesPerBlock);
    impulseBuffer.setSize(2, samplesPerBlock);

    resonator.prepare(sampleRate, samplesPerBlock);
    resonatorBuffer.setSize(2, samplesPerBlock);
}

void ResonatorVoice::setDecayTime(float decaySeconds)
{
    resonator.setDecayTime(decaySeconds);
}

void ResonatorVoice::setBrightness(float brightness)
{
    resonator.setBrightness(brightness);
}

void ResonatorVoice::setNumModes(int numModes)
{
    resonator.setNumActiveModes(numModes);
}

void ResonatorVoice::setExcitationSource(ImpulseGenerator::ExcitationSource source)
{
    impulseGenerator.setExcitationSource(source);
}

ImpulseGenerator::ExcitationSource ResonatorVoice::getExcitationSource() const
{
    return impulseGenerator.getExcitationSource();
}

void ResonatorVoice::setFilterEnabled(bool enabled)
{
    impulseGenerator.setFilterEnabled(enabled);
}

void ResonatorVoice::setFilterCutoff(float frequencyHz)
{
    impulseGenerator.setFilterCutoff(frequencyHz);
}

void ResonatorVoice::setFilterResonance(float resonance)
{
    impulseGenerator.setFilterResonance(resonance);
}

void ResonatorVoice::setFilterType(ImpulseGenerator::FilterType type)
{
    impulseGenerator.setFilterType(type);
}

void ResonatorVoice::setVelocityCurve(float curve)
{
    impulseGenerator.setVelocityCurve(curve);
}

void ResonatorVoice::setModeFrequencyRatio(int mode, float ratio)
{
    resonator.setModeFrequencyRatio(mode, ratio);
}

void ResonatorVoice::setModeDecayMultiplier(int mode, float multiplier)
{
    resonator.setModeDecayMultiplier(mode, multiplier);
}

void ResonatorVoice::setModeGainDb(int mode, float gainDb)
{
    resonator.setModeGainDb(mode, gainDb);
}

void ResonatorVoice::setModeDetuneCents(int mode, float cents)
{
    resonator.setModeDetuneCents(mode, cents);
}

void ResonatorVoice::setModeStereoWidth(int mode, float width)
{
    resonator.setModeStereoWidth(mode, width);
}

void ResonatorVoice::setModeEnabled(int mode, bool enabled)
{
    resonator.setModeEnabled(mode, enabled);
}
