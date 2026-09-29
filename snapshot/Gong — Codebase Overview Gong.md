# Gong Energy Synthesizer — Codebase Overview

## 1. Architecture Overview

The Gong Energy Synthesizer is a **standalone JUCE audio application** implementing a modal synthesis engine that simulates gong-like resonance. The architecture follows a linear signal processing pipeline:

```
Audio Input / MIDI → ImpulseGenerator → ResonatorBank (4 × 7-voice) → ConvolutionReverb → ExciterProcessor → MultibandCompressor → Output
                                              ↑
                                    EnergyAccumulator (modulates resonator behavior)
```

**Key architectural patterns:**
- **Component-based DSP pipeline** — each processing stage is an independent class with `prepare()`, `process()`, and `reset()` lifecycle methods (mirroring JUCE DSP conventions)
- **Energy-accumulation model** — rather than traditional envelope followers, a leaky-integrator energy system modulates resonator parameters (brightness, spread, detune, stereo width) for organic gong behavior
- **MainComponent as orchestrator** — `MainComponent` inherits from `AudioAppComponent` and wires together all DSP modules, MIDI input handling, and the UI

## 2. Key Modules/Directories

| Path | Purpose |
|------|---------|
| `src/` | All application source code (flat structure, no subdirectories) |
| `JUCE/` | JUCE framework (added as CMake subdirectory) |
| Root | CMake build config, documentation/handover files |

The project uses a **flat source layout** — all ~24 source/header files live directly in `src/`.

## 3. Important Files

### Core Engine
| File | Role |
|------|------|
| `GongSynthesizer.cpp/h` | **Central synth engine** — orchestrates ImpulseGenerator, EnergyAccumulator, and ResonatorBank into a unified processing pipeline |
| `EnergyAccumulator.cpp/h` | Maintains global + per-band (4-band) energy reservoirs with leaky integrator decay and nonlinear saturation |
| `ResonatorBank.cpp/h` | Manages 4 independent `SpreadVoiceResonator` instances, routes per-band energy, default tuning: A2/A3/E4/A4 |
| `SpreadVoiceResonator.cpp/h` | Single resonator with **7 voices** (1 center + 6 spread) — energy modulates brightness, detune, pan width, and frequency bending |
| `ImpulseGenerator.cpp/h` | Generates excitation signals — supports Dirac delta, noise bursts, audio input passthrough, and MIDI triggering with configurable filtering |

### Effects Chain
| File | Role |
|------|------|
| `ConvolutionEngine.cpp/h` | FFT-based convolution reverb using JUCE's `dsp::Convolution`; loads IR files or embedded data; dry/wet mixing |
| `ExciterProcessor.cpp/h` | Harmonic exciter: highpass → saturation → dry/wet blend |
| `MultibandCompressor.cpp/h` | 3-band compressor with Linkwitz-Riley crossovers at 200Hz/2kHz; per-band threshold, ratio, attack/release |

### Application Shell
| File | Role |
|------|------|
| `Main.cpp` | JUCE application entry point and window management |
| `MainComponent.cpp/h` | **UI + audio callback hub** — MIDI input selection, slider/button listeners, timer-based UI updates; wires all DSP modules together |
| `IRWaveformComponent.cpp/h` | Visual waveform display for loaded impulse responses |

### Documentation
| File | Role |
|------|------|
| `HANDOVER.md` | Developer handover with architecture, structure, and context |
| `Technical Brief for Gong Convolution Reverb.md` | Original design specification and requirements |

## 4. Tech Stack

| Category | Details |
|----------|---------|
| **Language** | C++17 |
| **Framework** | JUCE (audio, GUI, DSP, MIDI) |
| **Build System** | CMake 3.22+ with JUCE's CMake API (`juce_add_gui_app`) |
| **DSP Dependencies** | `juce::dsp::Convolution`, `juce::dsp::StateVariableTPTFilter`, `juce::dsp::LinkwitzRileyFilter`, `juce::dsp::DryWetMixer` |
| **Target Platforms** | macOS (primary dev via Xcode), planned Raspberry Pi / embedded Linux deployment |
| **App Type** | Standalone GUI application (not a plugin) with microphone input |

## 5. Current State Summary

**Maturity: Mid-stage functional prototype (v2.0.0)**

- The project has evolved from a simple convolution reverb into a full **energy-accumulation gong synthesizer** — the architecture pivot is documented in the handover
- All major DSP components are implemented: modal resonator bank with 28 total voices (4×7), convolution reverb, exciter, and multiband compression
- The UI supports MIDI device selection, parameter sliders, IR waveform visualization, and real-time control
- Audio input excitation with strike detection is the default mode (microphone permissions configured)
- The codebase is clean and modular with consistent JUCE DSP lifecycle patterns, but uses a flat file structure suggesting a small-team/solo project
- No test infrastructure, plugin format support, or preset system is visible
- Ready for integration testing and deployment hardening toward the embedded target