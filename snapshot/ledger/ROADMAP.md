# Roadmap

## Next Up

| Task | Milestone | Phase | Status | Effort |
|------|-----------|-------|--------|--------|
| 1.1.1 Inter-band energy coupling | 1.1 Core Nonlinear Dynamics | 1: Nonlinear Dynamics | Todo | Deep Focus |
| 2.1.1 Breadboard one-channel conditioning circuit | 2.1 Signal Conditioning | 2: Hardware Integration | Todo | Physical |
| 3.1.1 Cross-compile JUCE engine for Pi 5 | 3.1 Pi Audio Engine | 3: Pi Deployment | Todo | Deep Focus |

---

## Phase 1: Nonlinear Dynamics
**Status:** Todo
**Definition of Done:** The desktop audio engine produces convincing nonlinear gong behaviour — energy bloom, pitch glide, and dynamic spectral compensation are audible and tuneable. Validated by ear before hardware integration.

### 1.1 — Core Nonlinear Dynamics (Phase 1 of Implementation Brief)
**Status:** Todo
**Priority:** High
**Definition of Done:** Energy coupling, pitch glide, and spectral compensation implemented and producing audible results on desktop.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 1.1.1 | Inter-band energy coupling in EnergyAccumulator | Todo | Deep Focus | 4x4 coupling matrix, bloom delay via slew limiter, stochastic variation. See impl brief 1.1 |
| 1.1.2 | Energy-dependent pitch modulation (pitch glide) | Todo | Deep Focus | New ModTarget, per-band glide with jitter. See impl brief 1.2 |
| 1.1.3 | Dynamic post-convolution spectral compensation | Todo | Deep Focus | High-shelf filter driven by global energy. See impl brief 1.3 |
| 1.1.4 | Diagnostic additions for new parameters | Todo | Quick Win | Add coupling transfer rate + pitch offset to DiagnosticState/DiagnosticWindow |

### 1.2 — Advanced Nonlinear Features (Phases 2-4 of Implementation Brief)
**Status:** Todo
**Priority:** Normal
**Definition of Done:** Combination tones, crash noise, modal templates, and advanced energy dynamics are implemented.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 1.2.1 | Combination tone generation | Todo | Deep Focus | CombinationToneBank.cpp exists — check state |
| 1.2.2 | Crash/shimmer noise generator | Todo | Deep Focus | CrashNoiseGenerator.cpp exists — check state |
| 1.2.3 | Modal frequency templates from literature | Todo | Creative | ModalTemplate.h exists — check state |
| 1.2.4 | Amplitude-dependent decay rates | Todo | Deep Focus | Per impl brief phase 2 |
| 1.2.5 | Double decay envelope | Todo | Deep Focus | Per impl brief phase 2 |

---

## Phase 2: Hardware Integration
**Status:** Todo
**Definition of Done:** Strike capture chain (piezo → TL074 → Teensy → USB serial) is working and producing clean strike descriptors for all four channels.

### 2.1 — Signal Conditioning
**Status:** Todo
**Priority:** High
**Definition of Done:** One-channel breadboard prototype producing clean transients across full dynamic range.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 2.1.1 | Breadboard one-channel TL074 conditioning circuit | Todo | Physical | Verify voltage range and signal shape |
| 2.1.2 | Test across full dynamic range with contact mic on MDF | Todo | Physical | Depends on having MDF segment |
| 2.1.3 | Expand to four channels | Todo | Physical | |

### 2.2 — Teensy Strike Capture
**Status:** Todo
**Priority:** High
**Definition of Done:** Teensy producing valid 11-byte strike descriptors over USB serial for all four channels.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 2.2.1 | Basic onset detection and peak velocity on Teensy | Todo | Deep Focus | Single channel first |
| 2.2.2 | 256-sample window capture and FFT feature extraction | Todo | Deep Focus | CMSIS-DSP library |
| 2.2.3 | Binary descriptor packing and USB serial output at 1Mbaud | Todo | Deep Focus | 11-byte packet with XOR checksum |
| 2.2.4 | Debug mode: raw window dump alongside features | Todo | Quick Win | For offline calibration |
| 2.2.5 | Expand to all four channels | Todo | Deep Focus | Two hardware ADCs operating simultaneously |
| 2.2.6 | Calibrate feature scaling against different playing techniques | Todo | Creative | Requires all hardware assembled |

### 2.3 — Synthetic Impulse Tuning
**Status:** Todo
**Priority:** Normal
**Definition of Done:** Strike descriptors drive synthetic impulses that produce convincing resonator responses on desktop.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 2.3.1 | Test SyntheticImpulseGenerator with simulated descriptors on desktop | Todo | Deep Focus | Code exists — validate by ear |
| 2.3.2 | Tune descriptor-to-impulse mapping | Todo | Creative | Velocity→amplitude, centroid→filter, slope→envelope |
| 2.3.3 | Test with real Teensy descriptors when available | Todo | Deep Focus | Integration test |

---

## Phase 3: Pi Deployment
**Status:** Todo
**Definition of Done:** JUCE audio engine running headless on Pi 5 with serial input, convolution, and acceptable latency.

### 3.1 — Pi Audio Engine
**Status:** Todo
**Priority:** High
**Definition of Done:** Headless JUCE engine running on Pi 5 with full DSP chain at 48kHz/256 samples.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 3.1.1 | Cross-compile JUCE engine for Pi 5 (or native compile as fallback) | Todo | Deep Focus | CMake headless target |
| 3.1.2 | Benchmark full DSP chain on Pi 5 | Todo | Deep Focus | Critical — may need NonUniform convolution or voice reduction |
| 3.1.3 | Configure ALSA or JACK audio output | Todo | Deep Focus | JACK preferred for latency, target 256 samples at 48kHz |
| 3.1.4 | Integrate serial reader thread for Teensy communication | Todo | Deep Focus | Lock-free ring buffer |
| 3.1.5 | End-to-end latency measurement (strike to sound) | Todo | Deep Focus | Target: sub-20ms total |

### 3.2 — Performance UI on Pi
**Status:** Todo
**Priority:** Normal
**Definition of Done:** Circular screen shows working touch interface for preset switching.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 3.2.1 | Get Waveshare 5" circular display working on Pi 5 | Todo | Deep Focus | |
| 3.2.2 | Configure Chromium kiosk mode pointing at localhost | Todo | Quick Win | |
| 3.2.3 | Implement PerformanceServer REST API (list/activate/state) | Todo | Deep Focus | PerformanceServer.cpp exists — check state |
| 3.2.4 | Build touch-optimised preset switching UI | Todo | Creative | web/ has early implementation |
| 3.2.5 | Prototype interaction patterns on actual hardware | Todo | Creative | Circular form factor constraints |

---

## Phase 4: Physical Build
**Status:** Todo
**Definition of Done:** Four MDF segments mounted on clothes rail frame with full vibration isolation, contact mics installed, all electronics connected, complete signal path working.

### 4.1 — Frame Assembly
**Status:** Todo
**Priority:** High
**Definition of Done:** Clothes rail with four crossbars installed, anti-rotation bolts in place.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 4.1.1 | Acquire clothes rail and confirm upright tube diameter | Todo | Physical | Resolves open question 6 |
| 4.1.2 | Cut four crossbars from 10mm round bar | Todo | Physical | Length depends on upright spacing |
| 4.1.3 | Drill uprights at four crossbar heights | Todo | Physical | CB1-CB4 per mounting plan |
| 4.1.4 | Install crossbars with anti-rotation through-bolts | Todo | Physical | M5 bolts at each junction |

### 4.2 — Segment Mounting
**Status:** Todo
**Priority:** High
**Definition of Done:** Four MDF segments suspended with rubber grommet isolation, neoprene gap strips applied.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 4.2.1 | Drill oversized (10mm) mounting holes in MDF segments | Todo | Physical | 12 holes total, 3 per segment |
| 4.2.2 | Drill 6.5mm holes in crossbars at bolt positions | Todo | Physical | Per mounting plan section 3.4 |
| 4.2.3 | Mount segments with M6 bolts and rubber grommets | Todo | Physical | No rigid MDF-to-steel contact |
| 4.2.4 | Apply neoprene strip to segment edges | Todo | Physical | Fill 2mm inter-segment gaps |
| 4.2.5 | Verify vibration isolation between segments | Todo | Physical | Tap test — each mic should only hear its own segment |
| 4.2.6 | Dry-fit Pi clearance at CB2/CB3 inner bolts | Todo | Physical | May need 90mm bolts |

### 4.3 — Electronics Installation
**Status:** Todo
**Priority:** Normal
**Definition of Done:** Contact mics, Teensy, Pi, and screen mounted and connected.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 4.3.1 | Mount contact mics at centroid positions | Todo | Physical | Per mounting plan section 3.5 |
| 4.3.2 | Mount TL074 conditioning circuit | Todo | Physical | |
| 4.3.3 | Wire piezos to conditioning circuit to Teensy | Todo | Physical | |
| 4.3.4 | Mount Pi and screen in central cutout | Todo | Physical | Needs isolation — resolves open question 7 |
| 4.3.5 | Connect Teensy to Pi via USB | Todo | Physical | |
| 4.3.6 | Full signal path integration test | Todo | Physical | Strike all four segments, verify independent responses |

---

## Phase 5: Sound Design and Presets
**Status:** Todo
**Definition of Done:** Library of presets covering a range of gong voicings, tested in simulated performance conditions.

### 5.1 — Preset Library
**Status:** Todo
**Priority:** Normal
**Definition of Done:** At least 5-10 usable presets for live performance.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 5.1.1 | Design resonator frequency sets for different gong characters | Todo | Creative | Harmonic, inharmonic, transitional |
| 5.1.2 | Select and prepare convolution IRs | Todo | Creative | Remove resonant frequencies, keep broadband noise |
| 5.1.3 | Tune effect chain per preset | Todo | Creative | Exciter, compressor settings |
| 5.1.4 | Establish preset transfer workflow (desktop → Pi) | Todo | Administrative | SCP or USB drive initially |
| 5.1.5 | Test presets in simulated performance conditions | Todo | Creative | |

---

## Phase 6: Finish and Performance
**Status:** Todo
**Definition of Done:** Veneered instrument used in at least one live sound bath event, any resulting issues addressed.

### 6.1 — Veneer and Aesthetics
**Status:** Todo
**Priority:** Low
**Definition of Done:** Instrument is visually striking and performance-ready.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 6.1.1 | Select veneer material | Todo | Creative | |
| 6.1.2 | Apply veneer to MDF segments | Todo | Physical | Professional woodworker |
| 6.1.3 | Final aesthetic finishing | Todo | Physical | |

### 6.2 — Live Performance
**Status:** Todo
**Priority:** Normal
**Definition of Done:** Instrument has survived real-world use and any fixes have been applied.

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 6.2.1 | First live sound bath performance | Todo | Creative | Definition of Done for whole project |
| 6.2.2 | Document issues from live use | Todo | Administrative | |
| 6.2.3 | Address issues | Todo | Deep Focus | |

---

## Phase 0: Completed Work
**Status:** Done

### 0.1 — Core Audio Engine
**Status:** Done

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 0.1.1 | GongSynthesizer with 4x7 resonator bank | Done | Deep Focus | |
| 0.1.2 | EnergyAccumulator with leaky integrators | Done | Deep Focus | |
| 0.1.3 | SpreadVoiceResonator with energy modulation | Done | Deep Focus | |
| 0.1.4 | ConvolutionEngine with full-length IR support | Done | Deep Focus | 28s IRs confirmed working |
| 0.1.5 | ExciterProcessor (harmonic exciter) | Done | Deep Focus | |
| 0.1.6 | MultibandCompressor (3-band Linkwitz-Riley) | Done | Deep Focus | |
| 0.1.7 | ImpulseGenerator (multiple excitation modes) | Done | Deep Focus | |
| 0.1.8 | ModulationBus (9 sources, 19 targets) | Done | Deep Focus | |

### 0.2 — Convolution Bug Resolution
**Status:** Done

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 0.2.1 | Identify root cause of silence for long IRs | Done | Deep Focus | Was a hard 10-second cap, not a JUCE bug |
| 0.2.2 | Remove maxSamples cap | Done | Quick Win | |
| 0.2.3 | Build headless test harness | Done | Deep Focus | tests/ConvolutionTest/ — all durations pass |

### 0.3 — Desktop GUI
**Status:** Done

| # | Task | Status | Effort | Notes |
|---|------|--------|--------|-------|
| 0.3.1 | MIDI device selection and parameter controls | Done | Deep Focus | |
| 0.3.2 | IR waveform visualization | Done | Deep Focus | |
| 0.3.3 | Custom UI components (knobs, meters, energy ring, patch cables) | Done | Creative | src/ui/ directory |
| 0.3.4 | Diagnostic window | Done | Deep Focus | |

---

## Dependencies

| Item | Depends On | Status |
|------|-----------|--------|
| 2.2.1 Teensy onset detection | 2.1.1 Conditioning circuit | Unmet |
| 2.3.3 Test with real descriptors | 2.2.3 Binary descriptor output | Unmet |
| 3.1.4 Serial reader integration | 2.2.3 Binary descriptor output | Unmet |
| 3.2.1 Circular display on Pi | 3.1.1 Pi compilation | Unmet |
| 4.1.2 Cut crossbars | 4.1.1 Confirm tube diameter | Unmet |
| 4.2.1 Drill MDF | 4.1.4 Frame complete | Unmet |
| 4.3.6 Full integration test | 4.3.1-4.3.5 All electronics installed | Unmet |
| 5.1.5 Test presets | 4.3.6 Full integration | Unmet |
| 6.1.2 Apply veneer | 4.2.5 Isolation verified | Unmet |
| 6.2.1 Live performance | 5.1.5 Presets tested | Unmet |

---

## Reference

### Status Values
| Status | Meaning |
|--------|---------|
| Todo | Not yet started |
| In Progress | Actively being worked on |
| Blocked: [reason] | Cannot proceed — reason is one of: poorly-defined, too-large, missing-info, missing-resource, decision-required |
| Waiting | User's part done, waiting on external input |
| Done | Complete |
| Dropped | Deliberately abandoned |

### Effort Types
| Type | Description |
|------|-------------|
| Deep Focus | Sustained concentration, problem-solving, design work |
| Creative | Open-ended, generative, exploratory |
| Administrative | Organising, documenting, updating, filing |
| Communication | Discussions, reviews, feedback |
| Physical | Hands-on work, building, soldering |
| Quick Win | Small, low-effort, momentum-building |

### Priority
High / Normal / Low — milestones only. Tasks inherit from their milestone unless overridden.
