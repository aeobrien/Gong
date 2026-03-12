#include "ConvolutionEngine.h"

ConvolutionEngine::ConvolutionEngine()
{
    convolution.reset();
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

    isPrepared = true;
}

void ConvolutionEngine::process(juce::AudioBuffer<float>& buffer)
{
    if (!isPrepared)
        return;

    // Bypass if no IR loaded or mix is 0
    if (!irLoaded || wetDryMix < 0.001f)
    {
        if (outputGainDb != 0.0f)
            buffer.applyGain(juce::Decibels::decibelsToGain(outputGainDb));
        return;
    }

    // Store dry signal for mixing
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Process through convolution
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    convolution.process(context);

    // Mix dry/wet
    float wet = wetDryMix;
    float dry = 1.0f - wet;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wetData = buffer.getWritePointer(ch);
        const auto* dryData = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
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

        // Allocate buffer and read data
        int numChannels = static_cast<int>(reader->numChannels);
        int numSamples = static_cast<int>(reader->lengthInSamples);

        // Limit IR to 10 seconds (longer causes CPU overload/silence)
        int maxSamples = static_cast<int>(fileSampleRate * 10.0);
        numSamples = juce::jmin(numSamples, maxSamples);

        irBuffer.setSize(numChannels, numSamples);
        reader->read(&irBuffer, 0, numSamples, 0, true, true);

        juce::Logger::writeToLog("Read IR file: " + file.getFileName()
            + " (" + juce::String(numSamples) + " samples, "
            + juce::String(irSampleRate) + " Hz, "
            + juce::String(numChannels) + " channels)");
    } // reader is destroyed here, releasing file handle

    // Load into convolution from the buffer we already read (avoids file locking)
    // Make a copy since loadImpulseResponse takes ownership via move
    juce::AudioBuffer<float> irCopy;
    irCopy.makeCopyOf(irBuffer);

    convolution.loadImpulseResponse(
        std::move(irCopy),
        fileSampleRate,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,  // Match working settings
        juce::dsp::Convolution::Normalise::yes
    );

    // Re-prepare the convolution after loading IR (this makes it work!)
    if (isPrepared)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(currentBlockSize);
        spec.numChannels = 2;
        convolution.prepare(spec);
    }

    irLoaded = true;
    juce::Logger::writeToLog("Loaded IR into convolution engine");
    return true;
}

bool ConvolutionEngine::loadImpulseResponseFromData(const void* data, size_t dataSize)
{
    double fileSampleRate = 48000.0; // default

    // Load IR data into buffer
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

    // Limit IR to 10 seconds
    int maxSamples = static_cast<int>(fileSampleRate * 10.0);
    numSamples = juce::jmin(numSamples, maxSamples);

    irBuffer.setSize(numChannels, numSamples);
    reader->read(&irBuffer, 0, numSamples, 0, true, true);

    // Load from buffer
    juce::AudioBuffer<float> irCopy;
    irCopy.makeCopyOf(irBuffer);

    convolution.loadImpulseResponse(
        std::move(irCopy),
        fileSampleRate,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,
        juce::dsp::Convolution::Normalise::yes
    );

    // Re-prepare after loading
    if (isPrepared)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(currentBlockSize);
        spec.numChannels = 2;
        convolution.prepare(spec);
    }

    irLoaded = true;
    juce::Logger::writeToLog("Loaded IR from embedded data");
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
