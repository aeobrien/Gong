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
│   ├── ConvolutionEngine.cpp/h      # JUCE convolution wrapper
│   ├── ExciterProcessor.cpp/h       # Harmonic exciter
│   ├── MultibandCompressor.cpp/h    # 3-band compressor
│   └── IRWaveformComponent.cpp/h    # IR visualization
├── tests/
│   └── ConvolutionTest/             # Headless convolution test harness
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
4. **Convolution Reverb** - Full-length IRs supported (28s Gong1.wav confirmed working)
5. **ExciterProcessor** - Harmonic exciter (tanh saturation)
6. **MultibandCompressor** - 3-band Linkwitz-Riley crossover compression
7. **UI** - All controls render and respond
8. **IR Loading via button** - Load IR button works correctly
9. **Dry/Wet Mix** - Manual mixing works at all levels
10. **Output Gain** - Works correctly

### Remaining Limitations

1. **JUCE DryWetMixer doesn't work** - Manual dry/wet mixing implemented as workaround

## Convolution — Resolved

### History
The convolution engine originally produced silence for IRs longer than ~10 seconds. A hard 10-second cap was added as a workaround.

### Root Cause
The silence was caused entirely by the 10-second cap in `ConvolutionEngine.cpp`, not by any JUCE limitation. A headless test harness (`tests/ConvolutionTest/`) confirmed that JUCE `dsp::Convolution` handles 28-second IRs without issues — all modes (Default, NonUniform with various head sizes) and all block sizes (64–1024) produce correct output.

### The Fix (March 2026)
1. Removed the `maxSamples` cap from both `loadImpulseResponse()` and `loadImpulseResponseFromData()`
2. The re-prepare-after-load pattern remains necessary and correct:
```cpp
convolution.loadImpulseResponse(std::move(irBuffer), sampleRate, ...);

// Re-prepare after loading — forces synchronous engine creation via popAll()
if (isPrepared)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(currentBlockSize);
    spec.numChannels = 2;
    convolution.prepare(spec);
}
```

### Test Harness Results (all PASS)
| IR Duration | Direct Convolution | ConvolutionEngine Wrapper |
|-------------|-------------------|--------------------------|
| 1s          | PASS (-28.9dB)    | PASS (-28.9dB)           |
| 5s          | PASS (-29.1dB)    | PASS (-29.1dB)           |
| 10s         | PASS (-29.3dB)    | PASS (-29.3dB)           |
| 15s         | PASS (-29.4dB)    | PASS (-29.4dB)           |
| 20s         | PASS (-29.7dB)    | PASS (-29.7dB)           |
| 28s         | PASS (-30.0dB)    | PASS (-30.0dB)           |

## Build Instructions

```bash
cd /Users/aidan/Dev/Gong/build
cmake --build .

# Run app
./GongSynth_artefacts/Gong\ Energy\ Synthesizer.app/Contents/MacOS/Gong\ Energy\ Synthesizer

# Run convolution tests
./tests/ConvolutionTest/ConvolutionTest_artefacts/Release/ConvolutionTest
```

Or use Xcode project at `/Users/aidan/Dev/Gong/xcode/GongEnergySynthesizer.xcodeproj`

## Parameter Reference

See the tooltips in the app for full parameter documentation:
- INPUT / STRIKE DETECTION
- ENERGY ACCUMULATOR
- RESONATOR BANK (4 resonators, 7 voices each)
- CONVOLUTION REVERB
- OUTPUT PROCESSING (Exciter + Compressor)
