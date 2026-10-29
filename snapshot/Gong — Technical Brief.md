## Technology Stack

**Audio engine: C++17 with JUCE v8.0.12.** JUCE provides the audio runtime, DSP primitives (convolution, filters, mixing), MIDI handling, and build system integration. It's a mature, well-documented framework for real-time audio and is the existing foundation of the working prototype. The engine runs as a standalone headless application on the Raspberry Pi 5.

**Build system: CMake 3.22+.** Using JUCE's CMake API (`juce_add_gui_app` for desktop, headless target for Pi). Cross-compilation for the Pi from the development machine is the intended workflow, though native compilation on the Pi is a fallback.

**Strike capture: Teensy 4.0 (ARM Cortex-M7, 600MHz).** Handles all time-critical analog work — ADC sampling of four piezo channels at 44.1kHz per channel, onset detection, and feature extraction including a 256-point FFT. Programmed in C/C++ using the Arduino/Teensyduino toolchain with the CMSIS-DSP library for FFT.

**Signal conditioning: TL074 quad op-amp circuit.** A single IC buffers, scales, and protects all four piezo inputs. Runs on 3.3V single-supply from the Teensy. Total component cost approximately £5–10.

**Performance UI: Embedded web server + HTML/CSS/JS.** A lightweight HTTP server (cpp-httplib or similar single-header library) embedded in the JUCE application serves a single-page touch interface to the circular screen's Chromium browser. The UI is vanilla HTML/CSS/JS — no framework. This approach was chosen because CSS provides the best tools for building a visually polished, touch-fluid interface within a circular display mask, and because it keeps the UI layer minimal and decoupled from the audio engine.

**Target platform: Raspberry Pi 5.** Runs the JUCE audio engine headless, the embedded web server, and drives the 5-inch circular Waveshare display. Audio output via ALSA or JACK, depending on latency requirements during testing.

**Desktop development environment: macOS.** The existing JUCE application with full desktop GUI is used for sound design, parameter tuning, and preset creation. This is a development tool, not a performance tool — it does not need to run on the Pi.

## Architecture

The system has three major subsystems with clear responsibility boundaries:

**1. Strike Capture (Teensy 4.0).** Four piezo discs are each buffered through a TL074 op-amp channel with passive voltage clamping and configurable gain. The Teensy samples all four channels at 44.1kHz using two hardware ADCs operating simultaneously. When a strike is detected (threshold-with-debounce, ~70µs detection time), a 256-sample analysis window is captured and five features are extracted: peak velocity, attack slope, spectral centroid, HF energy ratio, and decay shape. These are packed into an 11-byte binary descriptor and sent to the Pi over USB serial at 1Mbaud. A debug mode appends the raw 256-sample window for offline analysis.

**2. Audio Engine (Raspberry Pi 5, JUCE headless).** The engine receives strike descriptors from a serial reader (separate thread, lock-free ring buffer). Each descriptor drives a synthetic impulse generator that creates a short excitation signal shaped by the descriptor's features — velocity controls amplitude, spectral centroid and HF ratio shape the excitation filter, attack slope shapes the envelope, decay shape adjusts damping. This synthetic impulse feeds into the existing signal chain at the same point microphone input currently does, meaning the entire downstream pipeline is unchanged:

```
Strike Descriptor → Synthetic Impulse Generator → ResonatorBank (4 × 7-voice spread resonators)
                                                        ↑
                                              EnergyAccumulator (modulates brightness, spread, detune, pan width)
                                                        ↓
                                                 ConvolutionReverb → ExciterProcessor → MultibandCompressor → Audio Output
```

The resonator bank runs four independent `SpreadVoiceResonator` instances, each with seven bandpass filter voices (one centre, six stereo-spread). The `EnergyAccumulator` maintains per-band and global energy reservoirs using leaky integrators with nonlinear saturation, modulating resonator behaviour over time to create the organic, evolving character of a real gong.

**3. Performance UI (embedded web server).** A minimal REST API exposes preset management to the circular screen's browser. The API surface is small: list available presets, activate a preset, report current state. The browser-based UI provides a touch-optimised interface for switching between impulse responses and resonator frequency presets during live performance. The specific interaction pattern (split-screen swiping, card-based browsing, or other) is to be determined through prototyping on the actual hardware.

## Data Model

**Strike descriptors.** Fixed 11-byte binary packets: sync byte, pad ID (0–3), velocity (0–127), attack slope (0–255), spectral centroid (uint16, Hz), HF energy ratio (0–255), decay shape (0–255), timestamp (uint16, ms), XOR checksum. This is the only data that crosses the Teensy-Pi boundary during performance.

**Presets.** Each preset defines a complete instrument configuration: resonator frequencies for all four channels, detuning and voicing parameters, convolution IR selection, and effect chain settings. Presets are stored as configuration files on the Pi's filesystem (format TBD — likely JSON or a simple custom format). Presets are created and tuned using the desktop JUCE application and transferred to the Pi.

**Impulse responses.** WAV files stored on the Pi. The engine loads IRs via JUCE's `dsp::Convolution`. Full-length IRs (28+ seconds) are supported — a previous 10-second cap has been removed after testing confirmed JUCE handles long IRs correctly when `prepare()` is called after `loadImpulseResponse()`.

**Audio signal chain state.** All DSP parameters are held in memory by the JUCE engine. The `EnergyAccumulator` uses atomic variables for thread-safe access between the audio thread and modulation. No persistent state beyond preset files and IR WAVs.

## Key Decisions

**Teensy as strike preprocessor, not raw audio into Pi.** Sending four channels of raw audio to the Pi would require a multi-channel USB audio interface and would load the Pi with onset detection and feature extraction work. The Teensy handles all time-critical analog capture and analysis, sending only compact descriptors. This keeps the Pi's CPU budget entirely for DSP and gives deterministic, sub-7ms strike-to-descriptor latency.

**Synthetic impulse injection rather than engine rework.** The strike descriptor drives a synthetic impulse generator that feeds into the existing audio pipeline at the microphone input point. This preserves the entire working signal chain — resonators, energy accumulator, convolution, exciter, compressor — without modification. The descriptor's features shape the synthetic impulse's amplitude, spectral content, envelope, and damping, giving the engine the same information it would extract from a real audio transient but in a cleaner, more controllable form.

**Web-based performance UI over native.** A browser-based UI on the circular screen provides the best tools for building a visually polished touch interface within an unusual form factor (circular 5-inch display). CSS handles circular masking, smooth animations, and responsive touch targets naturally. The UI is served locally by the JUCE process, keeping the system self-contained with no external dependencies.

**Headless JUCE on Pi, full GUI on desktop.** The desktop application retains its complete development UI for sound design and preset creation. The Pi runs the same engine code but headless, with the web server providing the only user-facing interface. This avoids maintaining two native GUIs and keeps the Pi's resources focused on audio processing.

**MDF for acoustic deadness.** The striking surface is 25mm MDF, chosen specifically because it is acoustically dead at musical frequencies. Each strike produces a short, sharp impulse with minimal acoustic sustain, giving the software full control over the sound. Wood veneer is applied for aesthetics without compromising the acoustic properties.

**Rubber grommet isolation at all mounting points.** Every mechanical connection between an MDF segment and the steel frame passes through rubber grommets with oversized bolt holes (10mm holes for M6 bolts). No rigid contact exists between any segment and the frame, or between adjacent segments. This is a core architectural constraint — the instrument's expressiveness depends on four genuinely independent input channels.

## Integration Points

**Teensy → Pi: USB serial at 1Mbaud.** Binary strike descriptor packets with sync byte framing and XOR checksum. The Pi-side serial reader runs in a dedicated thread and pushes validated descriptors into a lock-free ring buffer consumed by the audio thread. A debug packet variant (different sync byte, 523 bytes) includes the raw analysis window for development use.

**Pi audio output: ALSA or JACK.** The JUCE engine outputs audio through the Pi's audio subsystem. JACK is preferred for lower latency (target: 256 samples at 48kHz, ~5ms buffer latency) but ALSA is the fallback. The output destination is either the Pi's onboard audio, a USB audio interface, or an I2S DAC HAT — to be determined during hardware integration.

**Embedded web server → browser: HTTP on localhost.** The JUCE process serves static assets (HTML/CSS/JS) and a REST API on a local port. The circular screen runs Chromium in kiosk mode pointing at localhost. No network dependency — this works without WiFi.

**Desktop → Pi: file transfer for presets and IRs.** Presets created on the desktop are transferred to the Pi via SCP, USB drive, or similar. There is no live connection between the desktop app and the Pi during performance. Wireless configuration (editing presets from a laptop over WiFi) is a possible future enhancement but not a requirement.

## Constraints

**Latency: sub-20ms strike-to-sound.** The latency budget is approximately 6–7ms for strike capture and descriptor transmission, plus 5–10ms for the Pi's audio output buffer. The combined ~12–17ms is below the perceptual threshold for most performers. The analysis window (5.8ms) is the dominant contributor and can be shortened to ~2.9ms (128 samples) if needed, at the cost of reduced frequency resolution in the spectral features.

**Pi 5 compute budget.** The resonator bank (28 bandpass filter voices), convolution reverb, exciter, and multiband compressor must all run within the Pi 5's CPU capacity at 48kHz with a 256-sample buffer. The existing JUCE convolution implementation uses approximately 20% CPU on desktop — replacing it with FFTConvolver is expected to reduce this dramatically (reported under 1% CPU).

**Convolution engine must support long IRs.** The instrument's sound design depends on convolution with impulse responses that may exceed 10 seconds. This is resolved — JUCE `dsp::Convolution` handles 28-second IRs correctly. The key requirement is calling `prepare()` after `loadImpulseResponse()` to force synchronous engine creation.

**Circular display: 5-inch, 1080×1080 pixels.** The performance UI must be designed for this specific form factor. Touch targets must be large enough for reliable operation during performance. The circular mask means corners are unusable — effective UI area is reduced compared to a rectangular display of the same diagonal.

**Single-supply 3.3V for signal conditioning.** The op-amp circuit runs from the Teensy's 3.3V output, limiting the signal swing to 0–3.3V. The piezo's output must be clamped and scaled within this range. The Teensy's 3.3V regulator can supply the TL074's ~3mA draw, but additional circuitry may require a separate regulator.

**Acoustic isolation between segments.** The four MDF segments must be vibrationally independent. The mounting system uses rubber grommets at all 12 bolt points, oversized holes to prevent bolt-to-MDF contact, and neoprene strip in the 2mm inter-segment gaps. Any residual crosstalk can be managed in software, but the physical isolation must be sufficient that each contact mic primarily captures its own segment's impulse.

## Implementation Order

**Phase 1: Bare signal path.** Build the conditioning circuit on a breadboard for one piezo channel. Verify voltage range and signal shape. Confirm clean transients across the full dynamic range.

**Phase 2: Teensy onset detection and velocity.** Implement basic onset detection and peak velocity extraction. Output simple serial messages. Validate timing and dynamic range against real playing. Expand to all four channels.

**Phase 3: Feature extraction.** Add the 256-sample window capture and FFT-based feature extraction. Use debug mode to dump raw windows alongside features. Calibrate feature scaling against different playing techniques.

**Phase 4: Synthetic impulse generator.** Build the component that converts strike descriptors into excitation signals for the existing audio pipeline. Test on the desktop with the full JUCE GUI to tune the descriptor-to-impulse mapping by ear.

**Phase 5: Resolve convolution bug. ✅ COMPLETE.** The silence for long IRs was caused by a hard 10-second cap in `ConvolutionEngine.cpp`, not a JUCE bug. Removing the cap and keeping the re-prepare-after-load pattern resolves the issue. A headless test harness (`tests/ConvolutionTest/`) confirms all IR durations from 1s to 28s produce correct output across all convolution modes and block sizes. FFTConvolver replacement is not needed.

**Phase 6: Pi deployment.** Get the JUCE engine running headless on the Pi 5. Integrate the serial reader and synthetic impulse generator. Validate audio performance and latency with the resolved convolution engine.

**Phase 7: Physical prototype assembly.** Mount MDF segments on the clothes rail frame with rubber grommet isolation. Install contact mics. Wire to Teensy and conditioning circuit. Connect to Pi. Test the complete signal path from strike to sound.

**Phase 8: Performance UI.** Implement the embedded web server and REST API. Build the browser-based preset interface. Prototype interaction patterns on the actual circular screen. Iterate on visual design and usability.

**Phase 9: Preset creation and sound design.** Use the desktop JUCE application to create a library of presets (resonator frequency sets, IR selections, effect settings). Transfer to Pi. Test in simulated performance conditions.

**Phase 10: Veneer and finish.** Take the proven MDF prototype to a woodworker for veneer application and aesthetic finishing. This only happens after the instrument is functionally complete and tested.

**Phase 11: Live performance and iteration.** Use the instrument in at least one live sound bath event. Document any issues. Address them. The project is complete when the instrument has survived real-world use and any resulting fixes have been applied.

## Risks and Uncertainties

**Contact mic behaviour on MDF.** The software was developed against laptop microphone input. Contact mics on 25mm MDF will have fundamentally different frequency response, sensitivity, and transient characteristics. The Teensy's feature extraction mitigates this (the engine receives descriptors, not raw audio), but the synthetic impulse generator's mapping from features to excitation signals will need calibration against real contact mic data. This is the single highest-risk integration point.

**Convolution IR silence bug. ✅ RESOLVED.** The silence for long IRs was caused by a hard 10-second sample cap in `ConvolutionEngine.cpp`, not a JUCE limitation. Removing the cap resolved the issue. JUCE `dsp::Convolution` handles 28-second IRs correctly in all modes (Default, NonUniform with head sizes 512–8192) and all block sizes (64–1024). CPU usage on Pi 5 still needs benchmarking but FFTConvolver replacement is no longer the assumed path.

**Pi 5 audio performance.** The full DSP chain (28 resonator voices, convolution, exciter, multiband compressor) has not been benchmarked on the Pi 5. If CPU usage exceeds budget, options include using NonUniform convolution mode (which uses larger FFT blocks for the tail, reducing CPU), reducing voice count, or simplifying the effects chain. Early benchmarking on the Pi is critical.

**Synthetic impulse quality.** The approach of converting strike descriptors to synthetic excitation signals and feeding them into the existing pipeline is architecturally clean but unproven. If the resonators don't respond convincingly to synthetic impulses — if the sound feels artificial or unresponsive compared to real audio input — the impulse generation strategy may need significant iteration. This can be tested early on the desktop before any hardware integration, which reduces the risk significantly.

**Circular screen UI usability.** The interaction pattern for the performance UI is deliberately unspecified pending prototyping on the actual hardware. There is a risk that the 5-inch circular form factor is too constrained for comfortable mid-performance use, particularly with one hand occupied by a mallet. Mitigation: prototype early with simple placeholder UIs to establish what touch targets and gestures work before investing invisual polish.

**Vibration isolation effectiveness.** The rubber grommet mounting system is well-designed on paper but untested. Real-world crosstalk between segments sharing a steel frame is an empirical question. If isolation is insufficient, software-side crosstalk cancellation is possible (subtract a scaled version of adjacent channels), but this adds complexity and latency. Testing with the raw MDF prototype on the actual frame is an early priority.

**Gain calibration for piezo conditioning.** The optimal op-amp gain depends on the specific piezo discs, their mounting method, and the MDF surface. The circuit is designed for adjustable gain (unity to 5x), but finding the right setting requires testing with the actual materials. Starting at unity gain and increasing if dynamic range is insufficient is the safe approach.

**Preset transfer workflow.** The current plan is manual file transfer (SCP or USB drive) from desktop to Pi. This is functional but high-friction for iterative sound design. If preset creation becomes a frequent activity, a more streamlined transfer mechanism (WiFi sync, shared network folder) may become worth building. Not a launch blocker, but a quality-of-life concern.