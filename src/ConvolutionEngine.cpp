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

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Push dry signal to mixer
    dryWetMixer.pushDrySamples(block);

    // Process through convolution
    if (irLoaded)
    {
        convolution.process(context);
    }

    // Mix dry/wet
    dryWetMixer.mixWetSamples(block);

    // Apply output gain
    if (outputGainDb != 0.0f)
    {
        float gainLinear = juce::Decibels::decibelsToGain(outputGainDb);
        buffer.applyGain(gainLinear);
    }
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

    // Load IR into buffer for visualization
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader == nullptr)
    {
        juce::Logger::writeToLog("Could not read IR file: " + file.getFullPathName());
        return false;
    }

    // Store sample rate and load audio data
    irSampleRate = static_cast<int>(reader->sampleRate);
    irFileName = file.getFileName();

    // Allocate buffer and read data
    int numChannels = static_cast<int>(reader->numChannels);
    int numSamples = static_cast<int>(reader->lengthInSamples);

    // Limit IR size to prevent memory issues (max 10 seconds at 96kHz)
    int maxSamples = 960000;
    numSamples = juce::jmin(numSamples, maxSamples);

    irBuffer.setSize(numChannels, numSamples);
    reader->read(&irBuffer, 0, numSamples, 0, true, true);

    // Load into convolution engine
    convolution.loadImpulseResponse(
        file,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes,
        0  // size (0 = use original size)
    );

    irLoaded = true;
    juce::Logger::writeToLog("Loaded IR: " + file.getFileName()
        + " (" + juce::String(numSamples) + " samples, "
        + juce::String(irSampleRate) + " Hz)");
    return true;
}

bool ConvolutionEngine::loadImpulseResponseFromData(const void* data, size_t dataSize)
{
    // Load IR data into buffer for visualization
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    // createReaderFor takes a unique_ptr to the stream
    auto memStream = std::make_unique<juce::MemoryInputStream>(data, dataSize, false);
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(std::move(memStream)));

    if (reader != nullptr)
    {
        irSampleRate = static_cast<int>(reader->sampleRate);
        irFileName = "embedded";

        int numChannels = static_cast<int>(reader->numChannels);
        int numSamples = static_cast<int>(reader->lengthInSamples);

        // Limit IR size
        int maxSamples = 960000;
        numSamples = juce::jmin(numSamples, maxSamples);

        irBuffer.setSize(numChannels, numSamples);
        reader->read(&irBuffer, 0, numSamples, 0, true, true);
    }

    convolution.loadImpulseResponse(
        data,
        dataSize,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes,
        0
    );

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
