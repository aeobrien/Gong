#include "ConvolutionEngine.h"

ConvolutionEngine::ConvolutionEngine()
{
}

void ConvolutionEngine::prepare(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize = blockSize;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 2;

    convolution.prepare(spec);
    dryWetMixer.prepare(spec);
    dryWetMixer.setWetMixProportion(wetDryMix);

    // Pre-allocate dry buffer so we never allocate on the audio thread
    dryBuffer.setSize(2, blockSize);

    isPrepared = true;
}

void ConvolutionEngine::process(juce::AudioBuffer<float>& buffer)
{
    if (!isPrepared)
        return;

    // Bypass if no IR loaded, mix is 0, or currently loading
    if (!irLoaded || wetDryMix < 0.001f || isLoadingIR.load(std::memory_order_acquire))
    {
        if (outputGainDb != 0.0f)
            buffer.applyGain(juce::Decibels::decibelsToGain(outputGainDb));
        return;
    }

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    // Ensure dry buffer is large enough (no-op if already the right size)
    if (dryBuffer.getNumSamples() < numSamples || dryBuffer.getNumChannels() < numChannels)
        dryBuffer.setSize(numChannels, numSamples, false, false, true);

    // Measure input level before convolution
    float inputRms = buffer.getMagnitude(0, 0, numSamples);

    // Store dry signal for mixing (into pre-allocated buffer)
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Process through convolution
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    convolution.process(context);

    // Measure convolution output level
    float convRms = buffer.getMagnitude(0, 0, numSamples);

    // Periodic diagnostic logging (every ~2 seconds at 48kHz/512)
    static int diagCounter = 0;
    if (++diagCounter >= 188)  // ~2s at 48kHz/512
    {
        diagCounter = 0;
        juce::Logger::writeToLog("CONV DIAG: inputPeak=" + juce::String(inputRms, 4)
            + " convOutPeak=" + juce::String(convRms, 4)
            + " wetMix=" + juce::String(wetDryMix, 2)
            + " irLoaded=" + juce::String((int)irLoaded)
            + " isLoading=" + juce::String((int)isLoadingIR.load()));
    }

    // Mix dry/wet
    float wet = wetDryMix;
    float dry = 1.0f - wet;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* wetData = buffer.getWritePointer(ch);
        const auto* dryData = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            wetData[i] = dryData[i] * dry + wetData[i] * wet;
        }
    }

    // Apply output gain
    if (outputGainDb != 0.0f)
        buffer.applyGain(juce::Decibels::decibelsToGain(outputGainDb));
}

void ConvolutionEngine::reset()
{
    convolution.reset();
    dryWetMixer.reset();
}

bool ConvolutionEngine::loadImpulseResponse(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        juce::Logger::writeToLog("IR file not found: " + file.getFullPathName());
        return false;
    }

    double fileSampleRate = 0.0;

    // Scope the reader so file handle is released before convolution loads
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

        if (reader == nullptr)
        {
            juce::Logger::writeToLog("Could not read IR file: " + file.getFullPathName());
            return false;
        }

        // Store sample rate and load audio data
        fileSampleRate = reader->sampleRate;
        irSampleRate = static_cast<int>(reader->sampleRate);
        irFileName = file.getFileName();

        int numChannels = static_cast<int>(reader->numChannels);
        int numSamples = static_cast<int>(reader->lengthInSamples);

        irBuffer.setSize(numChannels, numSamples);
        reader->read(&irBuffer, 0, numSamples, 0, true, true);

        juce::Logger::writeToLog("Read IR file: " + file.getFileName()
            + " (" + juce::String(numSamples) + " samples, "
            + juce::String(irSampleRate) + " Hz, "
            + juce::String(numChannels) + " channels)");
    }

    // Load into convolution from buffer copy
    juce::AudioBuffer<float> irCopy;
    irCopy.makeCopyOf(irBuffer);

    convolution.loadImpulseResponse(
        std::move(irCopy),
        fileSampleRate,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,
        juce::dsp::Convolution::Normalise::yes
    );

    // Force synchronous engine creation by re-preparing.
    // Without this, the background thread computes the FFT partitions, but if the
    // audio thread is busy (overloads), the engine swap never happens and the
    // convolution outputs silence indefinitely. Re-prepare blocks briefly (~100ms
    // for a 28s IR) but guarantees the engine is immediately usable.
    // The isLoadingIR flag prevents the audio thread from calling process() on the
    // convolution during prepare(), avoiding a race condition.
    if (isPrepared)
    {
        isLoadingIR.store(true, std::memory_order_release);
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(currentBlockSize);
        spec.numChannels = 2;
        convolution.prepare(spec);
        isLoadingIR.store(false, std::memory_order_release);
    }

    irLoaded = true;
    juce::Logger::writeToLog("Loaded IR into convolution engine (NonUniform, head=4096)");
    return true;
}

bool ConvolutionEngine::loadImpulseResponseFromData(const void* data, size_t dataSize)
{
    double fileSampleRate = 48000.0;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    auto memStream = std::make_unique<juce::MemoryInputStream>(data, dataSize, false);
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(std::move(memStream)));

    if (reader == nullptr)
    {
        juce::Logger::writeToLog("Could not read embedded IR data");
        return false;
    }

    fileSampleRate = reader->sampleRate;
    irSampleRate = static_cast<int>(reader->sampleRate);
    irFileName = "embedded";

    int numChannels = static_cast<int>(reader->numChannels);
    int numSamples = static_cast<int>(reader->lengthInSamples);

    irBuffer.setSize(numChannels, numSamples);
    reader->read(&irBuffer, 0, numSamples, 0, true, true);

    juce::AudioBuffer<float> irCopy;
    irCopy.makeCopyOf(irBuffer);

    convolution.loadImpulseResponse(
        std::move(irCopy),
        fileSampleRate,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,
        juce::dsp::Convolution::Normalise::yes
    );

    if (isPrepared)
    {
        isLoadingIR.store(true, std::memory_order_release);
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(currentBlockSize);
        spec.numChannels = 2;
        convolution.prepare(spec);
        isLoadingIR.store(false, std::memory_order_release);
    }

    irLoaded = true;
    juce::Logger::writeToLog("Loaded IR from embedded data (NonUniform, head=4096)");
    return true;
}

void ConvolutionEngine::setWetDryMix(float wetRatio)
{
    wetDryMix = juce::jlimit(0.0f, 1.0f, wetRatio);
    dryWetMixer.setWetMixProportion(wetDryMix);
}

void ConvolutionEngine::setOutputGainDb(float gainDb)
{
    outputGainDb = juce::jlimit(-24.0f, 24.0f, gainDb);
}

float ConvolutionEngine::getIRLengthSeconds() const
{
    if (irSampleRate <= 0)
        return 0.0f;
    return static_cast<float>(irBuffer.getNumSamples()) / static_cast<float>(irSampleRate);
}
