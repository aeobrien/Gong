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
    convolutionB.prepare(spec);
    dryWetMixer.prepare(spec);
    dryWetMixer.setWetMixProportion(wetDryMix);

    // Pre-allocate buffers
    dryBuffer.setSize(2, blockSize);
    convABuffer.setSize(2, blockSize);
    convBBuffer.setSize(2, blockSize);

    // Step 8: Prepare post-conv EQ
    juce::dsp::ProcessSpec monoSpec;
    monoSpec.sampleRate = sampleRate;
    monoSpec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    monoSpec.numChannels = 1;

    postLowShelfL.prepare(monoSpec);
    postLowShelfR.prepare(monoSpec);
    postLowShelfL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    postLowShelfR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    postLowShelfL.setCutoffFrequency(200.0f);
    postLowShelfR.setCutoffFrequency(200.0f);

    postMidPeakL.prepare(monoSpec);
    postMidPeakR.prepare(monoSpec);
    postMidPeakL.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    postMidPeakR.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    postMidPeakL.setCutoffFrequency(1000.0f);
    postMidPeakR.setCutoffFrequency(1000.0f);
    postMidPeakL.setResonance(0.707f);
    postMidPeakR.setResonance(0.707f);

    postHighShelfL.prepare(monoSpec);
    postHighShelfR.prepare(monoSpec);
    postHighShelfL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    postHighShelfR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    postHighShelfL.setCutoffFrequency(4000.0f);
    postHighShelfR.setCutoffFrequency(4000.0f);

    eqNeedsUpdate = true;
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

    // Ensure dry buffer is large enough
    if (dryBuffer.getNumSamples() < numSamples || dryBuffer.getNumChannels() < numChannels)
        dryBuffer.setSize(numChannels, numSamples, false, false, true);

    // Store dry signal
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Step 14: Multi-IR crossfade
    if (irLoadedB && irCrossfade > 0.001f)
    {
        // Ensure crossfade buffers are large enough
        if (convABuffer.getNumSamples() < numSamples || convABuffer.getNumChannels() < numChannels)
        {
            convABuffer.setSize(numChannels, numSamples, false, false, true);
            convBBuffer.setSize(numChannels, numSamples, false, false, true);
        }

        // Copy input to both convolution buffers
        for (int ch = 0; ch < numChannels; ++ch)
        {
            convABuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
            convBBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        }

        // Process through both convolutions
        {
            juce::dsp::AudioBlock<float> blockA(convABuffer);
            juce::dsp::ProcessContextReplacing<float> ctxA(blockA);
            convolution.process(ctxA);
        }
        {
            juce::dsp::AudioBlock<float> blockB(convBBuffer);
            juce::dsp::ProcessContextReplacing<float> ctxB(blockB);
            convolutionB.process(ctxB);
        }

        // Smooth crossfade
        irCrossfade += (irCrossfadeTarget - irCrossfade) * irCrossfadeSmoothing;
        float fadeA = 1.0f - irCrossfade;
        float fadeB = irCrossfade;

        // Mix A and B into buffer
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* dest = buffer.getWritePointer(ch);
            const auto* srcA = convABuffer.getReadPointer(ch);
            const auto* srcB = convBBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                dest[i] = srcA[i] * fadeA + srcB[i] * fadeB;
        }
    }
    else
    {
        // Single convolution (original path)
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        convolution.process(context);
    }

    // Mix dry/wet
    float wet = wetDryMix;
    float dry = 1.0f - wet;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* wetData = buffer.getWritePointer(ch);
        const auto* dryData = dryBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            wetData[i] = dryData[i] * dry + wetData[i] * wet;
    }

    // Step 8: Post-convolution EQ
    bool hasEQ = (postConvLowGainDb != 0.0f || postConvMidGainDb != 0.0f || postConvHighGainDb != 0.0f);
    if (hasEQ)
    {
        float lowGain = juce::Decibels::decibelsToGain(postConvLowGainDb);
        float midGain = juce::Decibels::decibelsToGain(postConvMidGainDb);
        float highGain = juce::Decibels::decibelsToGain(postConvHighGainDb);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float inL = buffer.getSample(0, sample);
            float inR = numChannels > 1 ? buffer.getSample(1, sample) : inL;

            // Parallel EQ: split into bands, apply gain, sum
            float lowL = postLowShelfL.processSample(0, inL);
            float midL = postMidPeakL.processSample(0, inL);
            float highL = postHighShelfL.processSample(0, inL);

            float outL = inL + lowL * (lowGain - 1.0f) + midL * (midGain - 1.0f) + highL * (highGain - 1.0f);
            buffer.setSample(0, sample, outL);

            if (numChannels > 1)
            {
                float lowR = postLowShelfR.processSample(0, inR);
                float midR = postMidPeakR.processSample(0, inR);
                float highR = postHighShelfR.processSample(0, inR);

                float outR = inR + lowR * (lowGain - 1.0f) + midR * (midGain - 1.0f) + highR * (highGain - 1.0f);
                buffer.setSample(1, sample, outR);
            }
        }
    }

    // Apply output gain
    if (outputGainDb != 0.0f)
        buffer.applyGain(juce::Decibels::decibelsToGain(outputGainDb));
}

void ConvolutionEngine::reset()
{
    convolution.reset();
    convolutionB.reset();
    dryWetMixer.reset();
    postLowShelfL.reset(); postLowShelfR.reset();
    postMidPeakL.reset(); postMidPeakR.reset();
    postHighShelfL.reset(); postHighShelfR.reset();
}

bool ConvolutionEngine::loadImpulseResponse(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        juce::Logger::writeToLog("IR file not found: " + file.getFullPathName());
        return false;
    }

    double fileSampleRate = 0.0;

    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

        if (reader == nullptr)
        {
            juce::Logger::writeToLog("Could not read IR file: " + file.getFullPathName());
            return false;
        }

        fileSampleRate = reader->sampleRate;
        irSampleRate = static_cast<int>(reader->sampleRate);
        irFileName = file.getFileName();

        int numChannels = static_cast<int>(reader->numChannels);
        int numSamples = static_cast<int>(reader->lengthInSamples);

        irBuffer.setSize(numChannels, numSamples);
        reader->read(&irBuffer, 0, numSamples, 0, true, true);
    }

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
        return false;

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
    return true;
}

bool ConvolutionEngine::loadImpulseResponseB(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    double fileSampleRate = 0.0;

    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr)
            return false;

        fileSampleRate = reader->sampleRate;
        irFileNameB = file.getFileName();

        int numChannels = static_cast<int>(reader->numChannels);
        int numSamples = static_cast<int>(reader->lengthInSamples);

        juce::AudioBuffer<float> irBuf(numChannels, numSamples);
        reader->read(&irBuf, 0, numSamples, 0, true, true);

        juce::AudioBuffer<float> irCopy;
        irCopy.makeCopyOf(irBuf);

        convolutionB.loadImpulseResponse(
            std::move(irCopy),
            fileSampleRate,
            juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::no,
            juce::dsp::Convolution::Normalise::yes
        );
    }

    if (isPrepared)
    {
        isLoadingIR.store(true, std::memory_order_release);
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(currentBlockSize);
        spec.numChannels = 2;
        convolutionB.prepare(spec);
        isLoadingIR.store(false, std::memory_order_release);
    }

    irLoadedB = true;
    return true;
}

void ConvolutionEngine::setIRCrossfadeEnergy(float energy)
{
    // Map energy 0-1 to crossfade 0-1 (IR A at low energy, IR B at high)
    irCrossfadeTarget = juce::jlimit(0.0f, 1.0f, energy);
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

void ConvolutionEngine::setPostConvLowGainDb(float db)
{
    postConvLowGainDb = juce::jlimit(-12.0f, 12.0f, db);
}

void ConvolutionEngine::setPostConvMidGainDb(float db)
{
    postConvMidGainDb = juce::jlimit(-12.0f, 12.0f, db);
}

void ConvolutionEngine::setPostConvHighGainDb(float db)
{
    postConvHighGainDb = juce::jlimit(-12.0f, 12.0f, db);
}

void ConvolutionEngine::updatePostConvEQ()
{
    // Currently unused - EQ parameters are applied directly in process()
    eqNeedsUpdate = false;
}
