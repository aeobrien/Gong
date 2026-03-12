# Gong Energy Synthesizer - Handover Document

## Project Overview

This is a JUCE-based audio application that transforms audio input (microphone) through a modal synthesis engine with convolution reverb. The project evolved from a "Gong Convolution Reverb" into an "Energy-Accumulation Gong Synthesizer".

### Core Concept
- Audio input excites 4 independent resonators (each with 7 voices: 1 center + 6 spread)
- An energy accumulation system replaces traditional envelope followers
- Energy modulates brightness, spread level, detune, and stereo width
- Output chain: Resonators → Convolution Reverb → Exciter → Multiband Compressor

## Project Structure

```
/Users/aidan/Dev/Gong/
├── src/
│   ├── Main.cpp
│   ├── MainComponent.cpp/h          # UI and audio callback
│   ├── GongSynthesizer.cpp/h        # Main synth engine
│   ├── EnergyAccumulator.cpp/h      # Energy reservoir with decay
│   ├── SpreadVoiceResonator.cpp/h   # 7-voice resonator with bandpass filters
│   ├── ResonatorBank.cpp/h          # Container for 4 resonators
│   ├── ImpulseGenerator.cpp/h       # Audio input filtering
│   ├── ConvolutionEngine.cpp/h      # JUCE convolution wrapper (WORKING - 10s limit)
│   ├── ExciterProcessor.cpp/h       # Harmonic exciter
│   ├── MultibandCompressor.cpp/h    # 3-band compressor
│   └── IRWaveformComponent.cpp/h    # IR visualization
├── IRs/
│   └── Gong1.wav                    # Default impulse response (28 seconds, 48kHz stereo)
├── build/                           # CMake build directory
├── xcode/                           # Xcode project (cmake -G Xcode)
└── JUCE/                            # JUCE framework v8.0.12 (submodule)
```

## Current Status

### What's Working

1. **Audio Input** - Microphone input captured and displayed
2. **GongSynthesizer** - Resonators process audio and produce output
3. **SpreadVoiceResonator** - 7-voice bandpass filtering with energy modulation
4. **Convolution Reverb** - WORKING (with 10 second IR limit)
5. **ExciterProcessor** - Harmonic exciter (tanh saturation)
6. **MultibandCompressor** - 3-band Linkwitz-Riley crossover compression
7. **UI** - All controls render and respond
8. **IR Loading via button** - Load IR button works correctly
9. **Dry/Wet Mix** - Manual mixing works at all levels
10. **Output Gain** - Works correctly

### Remaining Limitations

1. **IR length limited to 10 seconds** - Longer IRs cause silence (not just CPU overload)
2. **NonUniform/Latency modes have same limitation** - No benefit for long IRs
3. **JUCE DryWetMixer doesn't work** - Had to implement manual dry/wet mixing

## Convolution Fix - What We Discovered

### The Root Cause
The JUCE `dsp::Convolution` class requires `prepare()` to be called **after** `loadImpulseResponse()` when loading IRs dynamically.

### The Fix
```cpp
convolution.loadImpulseResponse(std::move(irBuffer), sampleRate, ...);

// THIS IS THE KEY FIX - re-prepare after loading
if (isPrepared)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(currentBlockSize);
    spec.numChannels = 2;
    convolution.prepare(spec);
}
```

### IR Length Testing Results

| Duration | Default Mode | NonUniform 512 | NonUniform 1024 |
|----------|--------------|----------------|-----------------|
| 5 sec    | Works        | Works          | Not tested      |
| 10 sec   | Works        | Works          | Not tested      |
| 12 sec   | Jittery      | Jittery        | Not tested      |
| 15 sec   | Silence      | Silence        | Silence         |
| 20 sec   | Silence      | Silence        | Silence         |
| 28 sec   | Silence      | Silence        | Silence         |

**Key Finding**: The ~10-12 second threshold is NOT a CPU overload issue - it causes complete silence, not glitches. NonUniform mode does not extend this threshold.

## Investigation Summary: Long IR Issue

### What We Tried
1. **Default convolution** - Works up to 10 seconds
2. **NonUniform { 512 }** - Same 10 second limit, no improvement
3. **NonUniform { 1024 }** - Silence at 20 seconds
4. **Latency { 512 }** - Silence (tested earlier)
5. **Different loading methods** - File vs buffer, same result
6. **Re-prepare after loading** - Fixed the basic issue but not length limit

### What We Know
- The issue is NOT CPU overload (silence, not glitches)
- The issue affects ALL convolution modes equally
- The threshold is around 10-12 seconds regardless of mode
- Trivial IRs (64 samples) work in all modes
- The prepare() fix is required for all modes to work at all

### Theories
1. **Internal buffer limit** - JUCE may have a hardcoded max IR size
2. **Memory allocation failure** - Silent failure when IR too large
3. **FFT size limit** - May hit a maximum partition count
4. **Background thread timeout** - IR processing may time out for long IRs

### Next Steps to Investigate
1. **Check JUCE source code** - Look for internal limits in Convolution.cpp
2. **Monitor memory** - Check if allocation fails for long IRs
3. **Add debug logging** - Log convolution internal state after loading
4. **Test with JUCE example** - See if JUCE's own demos have same limit
5. **Try older JUCE version** - Check if this is a JUCE 8 regression
6. **Alternative libraries** - Consider FFTConvolver or other libraries

## Build Instructions

```bash
cd /Users/aidan/Dev/Gong/build
cmake --build .

# Run
./GongSynth_artefacts/Gong\ Energy\ Synthesizer.app/Contents/MacOS/Gong\ Energy\ Synthesizer
```

Or use Xcode project at `/Users/aidan/Dev/Gong/xcode/GongEnergySynthesizer.xcodeproj`

## Current Working Code

### ConvolutionEngine.h
```cpp
private:
    // Default convolution - works up to ~10 seconds
    juce::dsp::Convolution convolution;
```

### ConvolutionEngine.cpp - Key Parts
```cpp
void ConvolutionEngine::process(juce::AudioBuffer<float>& buffer)
{
    if (!isPrepared) return;
    if (!irLoaded || wetDryMix < 0.001f) { /* bypass with gain */ return; }

    // Store dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Process convolution
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    convolution.process(context);

    // Manual dry/wet mix (JUCE DryWetMixer doesn't work)
    float wet = wetDryMix;
    float dry = 1.0f - wet;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wetData = buffer.getWritePointer(ch);
        const auto* dryData = dryBuffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            wetData[i] = dryData[i] * dry + wetData[i] * wet;
    }

    if (outputGainDb != 0.0f)
        buffer.applyGain(juce::Decibels::decibelsToGain(outputGainDb));
}

bool ConvolutionEngine::loadImpulseResponse(const juce::File& file)
{
    // ... read file into buffer ...

    // Limit IR to 10 seconds
    int maxSamples = static_cast<int>(fileSampleRate * 10.0);
    numSamples = juce::jmin(numSamples, maxSamples);

    // Load from buffer
    convolution.loadImpulseResponse(std::move(irCopy), fileSampleRate, ...);

    // KEY FIX: Re-prepare after loading
    if (isPrepared)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(currentBlockSize);
        spec.numChannels = 2;
        convolution.prepare(spec);
    }

    irLoaded = true;
    return true;
}
```

## Files Changed

| File | Changes |
|------|---------|
| ConvolutionEngine.cpp | Fixed IR loading, manual dry/wet, 10s limit, prepare() fix |
| ConvolutionEngine.h | Clean implementation |
| MainComponent.cpp/h | Complete synth architecture |
| GongSynthesizer.cpp/h | NEW - main synth engine |
| EnergyAccumulator.cpp/h | NEW |
| SpreadVoiceResonator.cpp/h | NEW |
| ResonatorBank.cpp/h | NEW |
| ExciterProcessor.cpp/h | NEW |
| MultibandCompressor.cpp/h | NEW |

## Workaround for Long IRs

Until the root cause is found, the practical workaround is:
1. **Trim IR files to 10 seconds** - First 10 seconds usually captures the essential character
2. **Use shorter reverb IRs** - Most quality IRs are under 10 seconds anyway
3. **Layer multiple short IRs** - Could potentially chain convolutions (not implemented)

## Parameter Reference

See the tooltips in the app for full parameter documentation:
- INPUT / STRIKE DETECTION
- ENERGY ACCUMULATOR
- RESONATOR BANK (4 resonators, 7 voices each)
- CONVOLUTION REVERB (working, 10s limit)
- OUTPUT PROCESSING (Exciter + Compressor)
