#include <JuceHeader.h>
#include "ConvolutionEngine.h"
#include <random>
#include <chrono>
#include <cmath>

//==============================================================================
// Utility functions
//==============================================================================

static juce::AudioBuffer<float> generateIR (double sampleRate, double durationSeconds, int numChannels = 2)
{
    int numSamples = static_cast<int> (sampleRate * durationSeconds);
    juce::AudioBuffer<float> ir (numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = ir.getWritePointer (ch);
        float decayRate = -6.0f / static_cast<float> (numSamples);

        std::mt19937 rng (static_cast<unsigned> (42 + ch));
        std::uniform_real_distribution<float> dist (-1.0f, 1.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            float envelope = std::exp (decayRate * static_cast<float> (i) * 10.0f);
            data[i] = dist (rng) * envelope;
        }
    }

    return ir;
}

static juce::AudioBuffer<float> generateWhiteNoise (int numSamples, int numChannels = 2)
{
    juce::AudioBuffer<float> buffer (numChannels, numSamples);
    std::mt19937 rng (12345);
    std::uniform_real_distribution<float> dist (-0.5f, 0.5f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = dist (rng);
    }

    return buffer;
}

static float measureRmsDb (const juce::AudioBuffer<float>& buffer)
{
    float sumSquared = 0.0f;
    int totalSamples = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            sumSquared += data[i] * data[i];
            totalSamples++;
        }
    }

    if (totalSamples == 0 || sumSquared < 1e-20f)
        return -200.0f;

    return 10.0f * std::log10 (sumSquared / static_cast<float> (totalSamples));
}

static juce::File writeIRToTempFile (const juce::AudioBuffer<float>& irBuffer, double sampleRate, const juce::String& name)
{
    juce::File tempFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                             .getChildFile (name + ".wav");

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer (
        wavFormat.createWriterFor (new juce::FileOutputStream (tempFile),
                                   sampleRate,
                                   static_cast<unsigned int> (irBuffer.getNumChannels()),
                                   16, {}, 0));
    if (writer != nullptr)
        writer->writeFromAudioSampleBuffer (irBuffer, 0, irBuffer.getNumSamples());

    return tempFile;
}

//==============================================================================
// Diagnostic test: simulate real-time behavior
//==============================================================================

static void testRealtimeSimulation (double irDurationSec, double sampleRate, int blockSize)
{
    juce::Logger::writeToLog ("\n=== REALTIME SIMULATION: IR=" + juce::String (irDurationSec, 0) + "s"
                             + " block=" + juce::String (blockSize) + " ===");

    double budgetMs = (static_cast<double> (blockSize) / sampleRate) * 1000.0;
    juce::Logger::writeToLog ("  Budget per block: " + juce::String (budgetMs, 2) + "ms");

    ConvolutionEngine engine;
    engine.setWetDryMix (1.0f);  // 100% wet to isolate convolution output
    engine.prepare (sampleRate, blockSize);

    // Generate IR and write to temp file
    auto irBuffer = generateIR (sampleRate, irDurationSec);
    auto tempFile = writeIRToTempFile (irBuffer, sampleRate, "realtime_test_ir");

    // Load IR (this is what the app does)
    bool loaded = engine.loadImpulseResponse (tempFile);
    juce::Logger::writeToLog ("  IR loaded: " + juce::String (loaded ? "yes" : "no"));
    juce::Logger::writeToLog ("  Engine reports loaded: " + juce::String (engine.isLoaded() ? "yes" : "no"));

    // Generate 5 seconds of white noise
    int totalSamples = static_cast<int> (sampleRate * 5.0);
    auto inputSignal = generateWhiteNoise (totalSamples);

    // Process in blocks, checking output at intervals
    int blocksProcessed = 0;
    int blocksPerSecond = static_cast<int> (sampleRate / blockSize);
    int maxBlockTime = 0;
    int overBudgetCount = 0;

    float rmsAtStart = -200.0f;
    float rmsAt500ms = -200.0f;
    float rmsAt1s = -200.0f;
    float rmsAt2s = -200.0f;
    float rmsAt4s = -200.0f;

    // Accumulate output for RMS measurement at intervals
    juce::AudioBuffer<float> measureBuffer (2, blockSize);

    for (int pos = 0; pos < totalSamples; pos += blockSize)
    {
        int samplesThisBlock = juce::jmin (blockSize, totalSamples - pos);

        juce::AudioBuffer<float> block (2, samplesThisBlock);
        for (int ch = 0; ch < 2; ++ch)
            block.copyFrom (ch, 0, inputSignal.getReadPointer (ch, pos), samplesThisBlock);

        auto startTime = std::chrono::steady_clock::now();
        engine.process (block);
        auto endTime = std::chrono::steady_clock::now();

        int blockTimeUs = static_cast<int> (
            std::chrono::duration<double, std::micro> (endTime - startTime).count());
        int blockTimeMs = blockTimeUs / 1000;

        if (blockTimeMs > maxBlockTime)
            maxBlockTime = blockTimeMs;
        if (static_cast<double> (blockTimeUs) / 1000.0 > budgetMs)
            overBudgetCount++;

        float blockRms = measureRmsDb (block);

        // Snapshot at key intervals
        if (blocksProcessed == 0)
            rmsAtStart = blockRms;
        if (blocksProcessed == blocksPerSecond / 2)
            rmsAt500ms = blockRms;
        if (blocksProcessed == blocksPerSecond)
            rmsAt1s = blockRms;
        if (blocksProcessed == blocksPerSecond * 2)
            rmsAt2s = blockRms;
        if (blocksProcessed == blocksPerSecond * 4)
            rmsAt4s = blockRms;

        blocksProcessed++;
    }

    juce::Logger::writeToLog ("  Blocks processed: " + juce::String (blocksProcessed));
    juce::Logger::writeToLog ("  Max block time: " + juce::String (maxBlockTime) + "ms");
    juce::Logger::writeToLog ("  Over-budget blocks: " + juce::String (overBudgetCount)
                             + "/" + juce::String (blocksProcessed));
    juce::Logger::writeToLog ("  RMS at block 0:    " + juce::String (rmsAtStart, 1) + "dB");
    juce::Logger::writeToLog ("  RMS at 500ms:      " + juce::String (rmsAt500ms, 1) + "dB");
    juce::Logger::writeToLog ("  RMS at 1s:         " + juce::String (rmsAt1s, 1) + "dB");
    juce::Logger::writeToLog ("  RMS at 2s:         " + juce::String (rmsAt2s, 1) + "dB");
    juce::Logger::writeToLog ("  RMS at 4s:         " + juce::String (rmsAt4s, 1) + "dB");

    bool hasOutput = rmsAt4s > -80.0f;
    juce::Logger::writeToLog ("  VERDICT: " + juce::String (hasOutput ? "PRODUCING OUTPUT" : "NO OUTPUT"));

    tempFile.deleteFile();
}

//==============================================================================
// Diagnostic test: wait then process
//==============================================================================

static void testWithWait (double irDurationSec, double sampleRate, int blockSize, int waitMs)
{
    juce::Logger::writeToLog ("\n=== WAIT-THEN-PROCESS: IR=" + juce::String (irDurationSec, 0) + "s"
                             + " wait=" + juce::String (waitMs) + "ms ===");

    ConvolutionEngine engine;
    engine.setWetDryMix (1.0f);
    engine.prepare (sampleRate, blockSize);

    auto irBuffer = generateIR (sampleRate, irDurationSec);
    auto tempFile = writeIRToTempFile (irBuffer, sampleRate, "wait_test_ir");

    engine.loadImpulseResponse (tempFile);

    juce::Logger::writeToLog ("  Waiting " + juce::String (waitMs) + "ms for background thread...");
    juce::Thread::sleep (waitMs);

    // Process 1 second
    int totalSamples = static_cast<int> (sampleRate);
    auto inputSignal = generateWhiteNoise (totalSamples);

    juce::AudioBuffer<float> outputBuffer (2, totalSamples);
    outputBuffer.clear();

    for (int pos = 0; pos < totalSamples; pos += blockSize)
    {
        int samplesThisBlock = juce::jmin (blockSize, totalSamples - pos);

        juce::AudioBuffer<float> block (2, samplesThisBlock);
        for (int ch = 0; ch < 2; ++ch)
            block.copyFrom (ch, 0, inputSignal.getReadPointer (ch, pos), samplesThisBlock);

        engine.process (block);

        for (int ch = 0; ch < 2; ++ch)
            outputBuffer.copyFrom (ch, pos, block.getReadPointer (ch), samplesThisBlock);
    }

    float rms = measureRmsDb (outputBuffer);
    juce::Logger::writeToLog ("  Output RMS: " + juce::String (rms, 1) + "dB");
    juce::Logger::writeToLog ("  VERDICT: " + juce::String (rms > -80.0f ? "PRODUCING OUTPUT" : "NO OUTPUT"));

    tempFile.deleteFile();
}

//==============================================================================
// Diagnostic: raw JUCE convolution NonUniform without re-prepare
//==============================================================================

static void testRawNonUniformNoReprepare (double irDurationSec, double sampleRate, int blockSize, int waitMs)
{
    juce::Logger::writeToLog ("\n=== RAW NONUNIFORM (no re-prepare): IR=" + juce::String (irDurationSec, 0) + "s"
                             + " wait=" + juce::String (waitMs) + "ms ===");

    juce::dsp::Convolution convolution (juce::dsp::Convolution::NonUniform { 1024 });

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (blockSize);
    spec.numChannels = 2;
    convolution.prepare (spec);

    auto irBuffer = generateIR (sampleRate, irDurationSec);
    juce::AudioBuffer<float> irCopy;
    irCopy.makeCopyOf (irBuffer);

    convolution.loadImpulseResponse (
        std::move (irCopy), sampleRate,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,
        juce::dsp::Convolution::Normalise::yes);

    // Do NOT call prepare() again — same as current ConvolutionEngine code
    juce::Logger::writeToLog ("  Waiting " + juce::String (waitMs) + "ms...");
    juce::Thread::sleep (waitMs);

    // Process 2 seconds in blocks, check output at intervals
    int totalSamples = static_cast<int> (sampleRate * 2.0);
    auto inputSignal = generateWhiteNoise (totalSamples);
    int blocksPerSecond = static_cast<int> (sampleRate / blockSize);

    float rmsFirst = -200.0f, rmsLast = -200.0f;
    int blockNum = 0;

    for (int pos = 0; pos < totalSamples; pos += blockSize)
    {
        int samplesThisBlock = juce::jmin (blockSize, totalSamples - pos);

        juce::AudioBuffer<float> block (2, samplesThisBlock);
        for (int ch = 0; ch < 2; ++ch)
            block.copyFrom (ch, 0, inputSignal.getReadPointer (ch, pos), samplesThisBlock);

        juce::dsp::AudioBlock<float> audioBlock (block);
        juce::dsp::ProcessContextReplacing<float> context (audioBlock);
        convolution.process (context);

        float rms = measureRmsDb (block);
        if (blockNum == 0) rmsFirst = rms;
        rmsLast = rms;

        if (blockNum < 5 || blockNum % blocksPerSecond == 0)
            juce::Logger::writeToLog ("  Block " + juce::String (blockNum) + ": " + juce::String (rms, 1) + "dB");

        blockNum++;
    }

    juce::Logger::writeToLog ("  First block: " + juce::String (rmsFirst, 1) + "dB");
    juce::Logger::writeToLog ("  Last block: " + juce::String (rmsLast, 1) + "dB");
    juce::Logger::writeToLog ("  VERDICT: " + juce::String (rmsLast > -80.0f ? "PRODUCING OUTPUT" : "NO OUTPUT"));
}

//==============================================================================
// Main
//==============================================================================

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    juce::Logger::writeToLog ("=== Convolution Diagnostic Test ===\n");

    const double sampleRate = 48000.0;

    // Test 1: Raw JUCE NonUniform WITHOUT re-prepare (mirrors our ConvolutionEngine)
    // With various wait times to see when output appears
    testRawNonUniformNoReprepare (28.0, sampleRate, 512, 0);
    testRawNonUniformNoReprepare (28.0, sampleRate, 512, 2000);
    testRawNonUniformNoReprepare (28.0, sampleRate, 512, 10000);

    // Test 2: Raw JUCE NonUniform WITH re-prepare (old behavior)
    juce::Logger::writeToLog ("\n=== RAW NONUNIFORM (WITH re-prepare): IR=28s ===");
    {
        juce::dsp::Convolution convolution (juce::dsp::Convolution::NonUniform { 1024 });
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 512;
        spec.numChannels = 2;
        convolution.prepare (spec);

        auto irBuffer = generateIR (sampleRate, 28.0);
        juce::AudioBuffer<float> irCopy;
        irCopy.makeCopyOf (irBuffer);

        auto loadStart = std::chrono::steady_clock::now();
        convolution.loadImpulseResponse (
            std::move (irCopy), sampleRate,
            juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::no,
            juce::dsp::Convolution::Normalise::yes);

        // Re-prepare — this is what the old code did
        convolution.prepare (spec);
        auto loadEnd = std::chrono::steady_clock::now();

        double loadTimeMs = std::chrono::duration<double, std::milli> (loadEnd - loadStart).count();
        juce::Logger::writeToLog ("  Load + re-prepare time: " + juce::String (loadTimeMs, 0) + "ms");

        // Process 1 second
        int totalSamples = static_cast<int> (sampleRate);
        auto inputSignal = generateWhiteNoise (totalSamples);

        float rmsFirst = -200.0f, rmsLast = -200.0f;
        int blockNum = 0;
        double maxBlockMs = 0.0;

        for (int pos = 0; pos < totalSamples; pos += 512)
        {
            int samplesThisBlock = juce::jmin (512, totalSamples - pos);
            juce::AudioBuffer<float> block (2, samplesThisBlock);
            for (int ch = 0; ch < 2; ++ch)
                block.copyFrom (ch, 0, inputSignal.getReadPointer (ch, pos), samplesThisBlock);

            auto t0 = std::chrono::steady_clock::now();
            juce::dsp::AudioBlock<float> audioBlock (block);
            juce::dsp::ProcessContextReplacing<float> context (audioBlock);
            convolution.process (context);
            auto t1 = std::chrono::steady_clock::now();

            double blockMs = std::chrono::duration<double, std::milli> (t1 - t0).count();
            if (blockMs > maxBlockMs) maxBlockMs = blockMs;

            float rms = measureRmsDb (block);
            if (blockNum == 0) rmsFirst = rms;
            rmsLast = rms;
            blockNum++;
        }

        juce::Logger::writeToLog ("  First block: " + juce::String (rmsFirst, 1) + "dB");
        juce::Logger::writeToLog ("  Last block: " + juce::String (rmsLast, 1) + "dB");
        juce::Logger::writeToLog ("  Max block processing time: " + juce::String (maxBlockMs, 2) + "ms");
        juce::Logger::writeToLog ("  Budget: " + juce::String (512.0 / sampleRate * 1000.0, 2) + "ms");
        juce::Logger::writeToLog ("  VERDICT: " + juce::String (rmsLast > -80.0f ? "PRODUCING OUTPUT" : "NO OUTPUT"));
    }

    // Test 3: ConvolutionEngine wrapper with realtime simulation
    testRealtimeSimulation (1.0, sampleRate, 512);
    testRealtimeSimulation (28.0, sampleRate, 512);
    testRealtimeSimulation (28.0, sampleRate, 256);
    testRealtimeSimulation (28.0, sampleRate, 128);

    // Test 4: ConvolutionEngine wrapper with explicit wait
    testWithWait (28.0, sampleRate, 512, 0);
    testWithWait (28.0, sampleRate, 512, 5000);
    testWithWait (28.0, sampleRate, 512, 15000);

    juce::Logger::writeToLog ("\n=== DONE ===");
    return 0;
}
