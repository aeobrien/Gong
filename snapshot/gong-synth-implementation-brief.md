# Gong Energy Synthesizer — Nonlinear Dynamics Implementation Brief

## Document Purpose

This brief specifies every modification and addition to the existing Gong Energy Synthesizer codebase required to transform it from a linear resonator-bank instrument into a perceptually convincing nonlinear gong model. Each change is grounded in the peer-reviewed acoustics literature compiled in three research reports, and is described in terms of the specific source files, classes, and data structures that must change.

The brief is organised into four implementation phases, ordered by audible impact per unit of engineering effort. Phases 1–3 can each be validated independently by ear before proceeding. Phase 4 contains higher-cost additions that are worth pursuing only once the earlier phases sound convincing.

Throughout this document, research citations use the following shorthand:

- **R1** = "Physics of gongs for modal synthesis design" (Compass report)
- **R2** = "Acoustic Behaviour of Gongs for Digital Reproduction" (Gemini report)
- **R3** = "Acoustic Behavior of Gongs for Digital Reproduction" (Deep Research report)

Primary literature references are given by author where relevant.

---

## Existing Architecture Summary

For reference, the current signal chain and module responsibilities:

```
Input (Mic or MIDI)
  → GongSynthesizer
      → Strike Detection / SyntheticImpulse
      → EnergyAccumulator (global + 4 bands)
      → ResonatorBank (4 × 7-voice bandpass)
  → ConvolutionEngine (partitioned FFT, 28s+ IRs)
  → ExciterProcessor (highpass + tanh saturation)
  → MultibandCompressor (3-band Linkwitz-Riley)
  → Master Gain
  → Output (stereo)
```

Key existing capabilities:
- **EnergyAccumulator**: Global + 4 per-band energy reservoirs with exponential decay and nonlinear saturation. Formula: `E += gain × S^power × (1 − E/E_max)`. Uses SpinLock only for coefficient recalculation.
- **SpreadVoiceResonator**: 7 bandpass voices per band (centre + 3 stereo spread pairs). Energy already modulates brightness, spread level, detune, and pan width.
- **ModulationBus**: Per-block processing. Reads sources from DiagnosticState, advances LFO phases, applies routes with curve shaping, accumulates additive offsets per target. 9 sources (InputLevel, GlobalEnergy, BandEnergy×4, OutputLevel, LFO1, LFO2) → 19 targets.
- **DiagnosticState**: Header-only shared state, all `std::atomic`, no locks on audio thread. Double-buffered modulation routes (max 32).
- **StrikeDescriptor**: velocity, attackSlope, spectralCentroid, hfEnergyRatio, decayShape, padId.
- **ConvolutionEngine**: JUCE `dsp::Convolution` (NonUniform partitioned FFT, 4096-point head). Manual dry/wet mixing.

---

## Phase 1: Core Nonlinear Dynamics

These three changes address the most significant perceptual gaps identified unanimously across all three research reports. They operate within the existing architecture and require no new DSP modules.

### 1.1 Inter-Band Energy Coupling

**The problem:** The four energy bands currently accumulate and decay independently. All three reports identify this as the single most critical gap. Real gongs exhibit delayed energy cascade from low-frequency modes into high-frequency modes — this is the physical mechanism behind the "bloom" or "shimmer" that defines gong sound. Without it, the instrument sounds like four independent resonators fading out at different rates rather than a single vibrating body.

**Physical basis:** Chaigne, Touzé & Thomas (Acoust. Sci. Technol. 26(5), 2005) showed that quadratic nonlinearity from shell curvature enables 1:2 internal resonance — the primary energy transfer mechanism. Only 5–10 strongly coupled modes drive the quasiperiodic regime. Fletcher (Acoustics Australia 40(3), 2012) described the upward energy cascade in tam-tams as producing a spectral centroid that rises over the first 1–2 seconds. The cascade front propagates to higher frequencies following t^(1/3) in the turbulent regime (Ducceschi et al., Physica D 280–281, 2014). Poirot, Bilbao & Kronland-Martinet (EURASIP J. Audio Speech Music Proc., 2024) proposed coupled resonant filter banks with an energy transfer matrix and specifically recommended stochastic variation of coupling coefficients for realism — this is the most directly applicable model. [R1 §7, R2 §4, R3 §nonlinear]

**What to modify:** `EnergyAccumulator.h/.cpp`

**Implementation detail:**

Define a 4×4 coupling matrix as a new member of EnergyAccumulator. Default coefficients (from R1, grounded in Poirot–Bilbao):

```
              To Band 1   To Band 2   To Band 3   To Band 4
From Band 1       —         0.05        0.20        0.10
From Band 2      0.02        —          0.15        0.08
From Band 3      0.01       0.03         —          0.10
From Band 4      0.01       0.02        0.03         —
```

Key asymmetry: upward transfer (low→high) dominates, with strongest path from Band 1→Band 3 (0.20) representing the primary shimmer cascade. Reverse coupling (high→low) is weak but non-zero (0.01–0.03) — this contributes to the subtle low-end swell after a hard strike that Legge & Fletcher (JASA 86(6), 1989) documented. All three reports simplified to purely upward cascade; the bidirectional model is more physically accurate.

Per audio processing block, after updating each band's energy independently via the existing decay formula, apply the coupling step:

```
For each target band j:
    transfer_in = 0.0
    For each source band i (where i ≠ j):
        if E_source[i] > coupling_threshold:
            noise_factor = 1.0 + stochastic_variation * random_uniform(-1, 1)
            transfer_in += C[i][j] * E_source[i] * noise_factor
    E[j] += transfer_in * block_duration
    E[j] = min(E[j], E_max[j])
```

`coupling_threshold` corresponds to the first bifurcation point: approximately 0.35 × E_max (R1 §7). Below this, the gong is in the linear regime and modes decay independently (Touzé & Chaigne, Acta Acustica 86(3), 2000). The `stochastic_variation` parameter should be 0.1–0.3 per the Poirot–Bilbao recommendation. Use a single `std::minstd_rand` per block (allocation-free, deterministic enough for audio-rate noise).

**Critical timing detail — the bloom delay:** The energy transfer from low to high bands must not be instantaneous. The physical cascade takes 0.5–2 seconds to develop (R1 §2, R2 §2, R3 §nonlinear). Implement this as a slew limiter (one-pole lowpass) on the transfer_in value before it reaches the target band:

```
slewed_transfer[j] += (transfer_in - slewed_transfer[j]) * (1.0 - exp(-block_duration / bloom_attack_time))
E[j] += slewed_transfer[j] * block_duration
```

`bloom_attack_time` should default to ~1.0 seconds, exposed as a parameter. For the four-phase tam-tam decay described in R1 §4 (Attack 0–50ms → Nonlinear build 50ms–2s → Mid sustain 2–20s → Late sustain 20s+), this timing aligns the synthetic bloom with the physical cascade onset. The t^(1/3) power law from Ducceschi could be approximated more precisely with a nonlinear slew shape, but a one-pole lowpass is a reasonable first approximation and can be refined later by ear.

**Thread safety:** The coupling matrix coefficients are set from the UI thread (preset changes, macro adjustments). Use the same SpinLock pattern already used for coefficient recalculation in EnergyAccumulator — brief lock during UI-initiated coefficient update, no lock during the per-block coupling calculation itself (which reads cached local copies).

**Diagnostic additions to DiagnosticState:** Add 4 `std::atomic<float>` values for `couplingTransferRate[4]` so the Energy Monitor tab in DiagnosticWindow can display the active transfer into each band.

---

### 1.2 Energy-Dependent Pitch Modulation (Pitch Glide)

**The problem:** Real gongs undergo significant pitch shifts under high-energy excitation — up to 3 semitones (300 cents) for the fundamental. A perfectly pitch-stable digital gong sounds rigid and artificial during high-energy events. This is one of the most immediately audible differences between a real gong and a static resonator bank.

**Physical basis:** Fletcher (JASA 78, 1985) derived the critical result: pitch glide direction depends on the ratio of shell thickness h to dome height H. Flat plates (h >> H) exhibit hardening nonlinearity — frequency starts above linear value at high amplitude, glides downward during free decay. Curved shells (h << H) exhibit softening — frequency glides upward. Jossic et al. (JASA 144(1), 2018) provided the most precise quantitative data: for the xiaoluo fundamental at 447 Hz, measured glide spans ~6 Hz (23 cents) at moderate amplitudes, extending to ~50 Hz (nearly 2 semitones) at highest mallet forces (95 N). The backbone curve nonlinear coefficient is C₀ = −8.1 × 10⁷ mm⁻² s⁻². Crucially, Jossic et al. showed that 1:2 internal resonances between the fundamental and modes at 859/880 Hz cause frequency beats superimposed on the smooth glide. [R1 §2, R2 §2, R3 §pitch glide]

**What to modify:** `ModulationTypes.h`, `ModulationBus.h/.cpp`, `SpreadVoiceResonator.h/.cpp`, `DiagnosticState.h`

**Implementation detail:**

Add a new ModTarget: `ResonatorPitchOffset` (or per-band variants: `Band1PitchOffset` through `Band4PitchOffset`). The ModulationBus already applies additive offsets per target per block — this is a new target in the existing routing infrastructure, not a new mechanism.

The pitch offset for each band is computed as:

```
pitch_offset_cents[band] = glide_coefficient * band_scale[band] * (E_band / E_max) + jitter[band]
```

Where:
- `glide_coefficient` is the master glide depth, positive for hardening/tam-tam (pitch starts high, falls with decay), negative for softening/opera gong (pitch starts low, rises with decay). Range: ±50 to ±300 cents. Default: +100 cents for tam-tam preset, -150 cents for opera gong preset. [R1 §7]
- `band_scale[band]` controls per-band glide depth. R3 specifically argues that pitch glide should NOT be uniform across partials — Band 1 gets the largest shift, higher bands get progressively less systematic shift but more random jitter. This is physically correct: higher-order modes have different backbone curves and can even shift in opposite directions. Recommended defaults: Band 1 = 1.0, Band 2 = 0.6, Band 3 = 0.2, Band 4 = 0.05. [R3 §pitch instability, Jossic et al. 2018]
- `jitter[band]` is random frequency modulation representing quasiperiodic regime instability. Computed per block: `jitter_amplitude * sin(jitter_phase) * (E_band / E_max)`, where `jitter_phase` advances at 5–50 Hz (randomised per block within this range). Jitter amplitude: ±0.2–1.0% of centre frequency in quasiperiodic regime (35–75% energy), ±1.0–5.0% in chaotic regime (>75% energy). [R1 §7]

**In SpreadVoiceResonator:** Add a `setPitchOffsetCents(float cents)` method that adjusts the centre frequency of all 7 voices by the offset. This should be called per block from GongSynthesizer after ModulationBus has computed the target offsets. The conversion is `frequency_multiplier = pow(2.0, cents / 1200.0)`. Apply this as a multiplier to the existing centre frequency (whether in Free Hz or Snap MIDI mode) before updating the bandpass filter coefficients. Coefficient updates per block are already happening for other modulated parameters, so this adds no new coefficient-update overhead.

**Interaction with existing modulation routes:** The pitch offset should be additive with any existing LFO→frequency modulation routes. The ModulationBus accumulates offsets per target — this naturally handles the additive combination.

---

### 1.3 Dynamic Post-Convolution Spectral Compensation

**The problem:** The convolution IR captures one static energy state of the gong body. If the IR was recorded from a loud strike, quiet MIDI notes will output a quiet version of the loud strike's spectral envelope — including high-frequency bloom content that should not be present at low energy levels. All three reports identify this as a fundamental limitation of static convolution. [R1 §7, R2 §7, R3 §static IR]

**Physical basis:** Convolution is by definition a linear, time-invariant operation and cannot represent amplitude-dependent frequency shifts, mode coupling, or energy cascade. The spectral envelope of a real gong changes dramatically with energy level. [R1 §7]

**What to modify:** `ConvolutionEngine.h/.cpp`, `GongSynthesizer.h/.cpp` (for routing energy to the filter)

**Implementation — Minimum Viable (Phase 1):**

Add a single `juce::dsp::StateVariableTPTFilter` configured as a high-shelf filter, placed immediately after the convolution output (post wet-signal, pre dry/wet mix — or post dry/wet mix, depending on whether you want the compensation to affect only the wet signal or the blended output; affecting only the wet signal is more physically correct since the dry resonator signal is already energy-dependent).

The filter is driven by the global energy level:

```
shelf_gain_dB = lerp(low_energy_cut_dB, 0.0, energy_normalised)
```

Where:
- `low_energy_cut_dB` = -12 dB (when energy is zero, cut 12 dB above the shelf frequency)
- `energy_normalised` = global energy / E_max, clamped to [0, 1]
- At full energy, the shelf is at 0 dB (IR passes through unmodified)
- Shelf frequency: ~1.5 kHz (this is the approximate boundary between "tonal body" and "bloom content")

This means at low energy, the recorded bloom in the IR is suppressed by 12 dB, and it opens up as the resonators drive more energy into the system. The result is that quiet notes sound appropriately dark and tonal, while loud strikes let the full IR bloom through.

R1 §7 suggests a three-tier approach (post-IR EQ → multi-IR crossfade → time-variant filtering). This Phase 1 implementation is the "post-IR EQ" tier. Phase 4 (§4.1) upgrades to multi-IR crossfade.

**Smoothing:** The shelf gain must be smoothed to avoid zipper noise. Apply a one-pole lowpass to `shelf_gain_dB` with a time constant of ~50ms. This is fast enough to track energy changes but slow enough to avoid per-sample filter coefficient recalculation artefacts.

**Filter implementation note:** `StateVariableTPTFilter` is the same type already used in ExciterProcessor, so no new filter type is introduced. One instance, mono (apply to each channel), updated once per block.

---

## Phase 2: Excitation Refinement

These changes improve the initial strike character and how repeated strikes interact with the energy model.

### 2.1 Velocity-Dependent Excitation Bandwidth

**The problem:** In the current architecture, MIDI velocity primarily controls amplitude. In a real gong, velocity simultaneously and nonlinearly controls amplitude, brightness (spectral bandwidth of the excitation), and the onset of nonlinear behaviour. Louder strikes are inherently brighter because harder mallet impact shortens contact time and widens excitation bandwidth.

**Physical basis:** Felt mallets obey a nonlinear Hertz contact law: F = Kδ^p where p ≈ 2.0–3.0 (Bork, Applied Acoustics 30, 1990; Chaigne & Doutaut, JASA, 1997). Contact time decreases with velocity as T ∝ v^(−(p−1)/(p+1)). For p = 2.5, doubling velocity reduces contact time by ~30%. The contact duration determines the high-frequency cutoff of excitation energy: f_cutoff ≈ 1/(2τ_contact). Ducceschi & Touzé (J. Sound Vib. 344, 2015) used interaction times of 1ms (hard stick) to 7ms (soft mallet) with forces of 120–300 N. [R1 §3, R2 §3, R3 §strike]

Contact time and bandwidth by mallet type (from R1 §3):

| Mallet type           | Contact duration | Effective bandwidth | Spectral rolloff   |
|-----------------------|------------------|---------------------|---------------------|
| Soft felt (tam-tam)   | 8–20 ms          | 50–200 Hz           | −12 to −18 dB/oct  |
| Hard felt             | 3–8 ms           | 200–800 Hz          | −6 to −12 dB/oct   |
| Yarn-wound            | 2–5 ms           | 300–1500 Hz         | −6 to −12 dB/oct   |
| Wooden beater         | 0.5–2 ms         | 500–4000 Hz         | −3 to −6 dB/oct    |
| Brass/metal           | 0.1–0.5 ms       | 1000–10000+ Hz      | −3 dB/oct           |

**What to modify:** `SyntheticImpulseGenerator.h/.cpp` (MIDI mode), `ImpulseGenerator.h/.cpp` (audio input mode), `MidiControllerMock.h/.cpp`, `StrikeDescriptor.h`

**Implementation detail:**

In `StrikeDescriptor`, the `spectralCentroid` and `hfEnergyRatio` fields already exist but are currently set by MIDI CC mapping in MidiControllerMock. Change the mapping so that velocity itself contributes to these fields, not just CC values:

```
// Excitation amplitude (nonlinear — mode recruitment at high velocity)
amplitude = pow(velocity / 127.0, 1.8)

// Contact time (shorter = brighter)
contact_time_ms = T_max * pow(velocity / 127.0, -0.4)
// T_max depends on mallet hardness setting:
// Soft felt: 15ms, Hard felt: 5ms, Wood: 1.5ms, Metal: 0.3ms

// Excitation bandwidth cutoff
f_cutoff = max(f_base, 1000.0 / (2.0 * contact_time_ms))
// f_base = minimum cutoff (50 Hz for soft felt, 500 Hz for metal)

// Map to StrikeDescriptor fields
strike.spectralCentroid = f_cutoff
strike.hfEnergyRatio = 1.0 - (contact_time_ms / T_max)  // 0 = all lows, 1 = broadband
```

In `SyntheticImpulseGenerator`, the existing spectral filtering of the noise burst should use `strike.spectralCentroid` as the cutoff frequency of the excitation lowpass filter. The rolloff slope should be controlled by a "mallet hardness" parameter: −12 dB/oct for soft felt, −6 dB/oct for hard felt, −3 dB/oct for wood/metal (per the table above). This is a filter order/slope change, not just a cutoff change.

The `attackSlope` field in StrikeDescriptor should also be velocity-dependent: faster attack at higher velocity (reflecting shorter contact time). Map as: `attack_samples = sampleRate * contact_time_ms / 1000.0`.

**New parameter in MidiControllerMock:** Add a `malletHardness` parameter (0.0–1.0) that interpolates between soft felt and metal mallet characteristics. This should be mappable to a MIDI CC (suggest CC 74, Brightness, which is semantically appropriate). The parameter controls:
- T_max (maximum contact time)
- f_base (minimum excitation bandwidth)
- Rolloff slope (filter order)
- The amplitude exponent (soft mallets: 1.5, hard mallets: 2.0 — reflecting increased nonlinear mode recruitment with rigid strikers)

**For audio input mode (ImpulseGenerator):** The velocity/amplitude detection already exists. Apply the same contact_time → f_cutoff mapping to the excitation filter, using the detected strike velocity as the input. The mallet hardness would need to be a user parameter since it can't be detected from audio alone.

---

### 2.2 Strike Position as Mode-Family Weighting

**The problem:** Where a gong is struck changes which modes are excited. Centre strikes emphasise axisymmetric modes (tonal, fundamental-heavy). Edge strikes emphasise modes with nodal diameters (bright, inharmonic). The current architecture treats all strikes as identical in terms of band energy distribution.

**Physical basis:** Centre strikes strongly excite axisymmetric modes (0,n) — antinodes at the centre, most tonal sound. Mid-radius excites mixed modes (1,1), (2,1). Edge strikes preferentially activate modes with nodal diameters (2,0), (3,0) — bright, inharmonic. On bossed gongs, boss strikes maximise pitch definition; off-boss produces wash. [R1 §1, R2 §3, R3 §strike position]

**What to modify:** `StrikeDescriptor.h`, `GongSynthesizer.h/.cpp` (energy injection routing)

**Implementation detail:**

Add a `strikePosition` field to StrikeDescriptor: 0.0 = dead centre, 1.0 = edge. Map this to a MIDI CC (suggest CC 16 or a spare CC).

In GongSynthesizer, where strike energy is injected into the 4 band accumulators, apply position-dependent weighting:

```
// Centre strike: almost all energy into Bands 1–2
// Edge strike: energy shifted toward Bands 3–4
float centre_weight = 1.0 - strikePosition;  // 1.0 at centre, 0.0 at edge
float edge_weight = strikePosition;

band_injection[0] = strike_energy * (0.6 * centre_weight + 0.1 * edge_weight)
band_injection[1] = strike_energy * (0.3 * centre_weight + 0.2 * edge_weight)
band_injection[2] = strike_energy * (0.08 * centre_weight + 0.35 * edge_weight)
band_injection[3] = strike_energy * (0.02 * centre_weight + 0.35 * edge_weight)
```

These weights ensure that a centre strike delivers most energy to the fundamental/body bands, while an edge strike bypasses the low end and directly excites the shimmer/wash bands. The weights should normalise to 1.0 at any position.

For the Digital Gong hardware instrument, the `padId` field in StrikeDescriptor could map different piezo zones to different strikePosition values, giving spatial control over the physical surface.

---

### 2.3 Roll Priming Variable

**The problem:** A sustained roll on a gong is acoustically distinct from a single high-velocity strike. The current energy accumulator naturally sums energy from repeated strikes, but it doesn't capture the physical phenomenon of "priming" — where pre-excitation lowers the time it takes for high-frequency shimmer to develop.

**Physical basis:** Rossing & Fletcher's tam-tam study reports that priming (soft roll before striking) appears to lower HF buildup times in the 4 kHz and 8 kHz bands. This is R3's unique and valuable contribution — rolls don't just add energy; they alter the coupling dynamics. The physical reasoning: repeated small strikes maintain the plate in a state of continuous low-amplitude vibration, keeping mode coupling "warm" so that new energy injections cascade upward more quickly. Additionally, phase randomisation from multiple strikes creates more noise-like spectral content than a single coherent impact. [R1 §7, R2 §3, R3 §priming]

**What to modify:** `EnergyAccumulator.h/.cpp`, `GongSynthesizer.h/.cpp`

**Implementation detail:**

Add a new state variable to EnergyAccumulator: `primedness` (float, 0.0–1.0). This is distinct from energy level — it tracks recent strike density rather than total accumulated energy.

```
// On each strike event:
primedness = min(1.0, primedness + priming_increment)
// priming_increment ≈ 0.15 per strike (tunable)

// Per block decay:
primedness *= exp(-block_duration / priming_decay_time)
// priming_decay_time ≈ 3.0–5.0 seconds
```

Primedness modulates two things:

1. **Bloom attack time (from §1.1):** The slew limiter attack time on inter-band energy transfer is shortened by primedness:
   ```
   effective_bloom_attack = bloom_attack_time * (1.0 - 0.7 * primedness)
   ```
   At full primedness (sustained roll), the bloom develops ~3× faster than from a cold single strike.

2. **Noise injection:** Add a small noise component to the excitation that scales with strike rate. Track inter-strike interval:
   ```
   // On strike:
   inter_strike_interval = time_since_last_strike
   noise_injection = max(0, 1.0 - inter_strike_interval / 0.5) * noise_amount
   // Short intervals (fast roll) → more noise; intervals > 500ms → no extra noise
   ```
   This noise is added to the excitation signal in SyntheticImpulseGenerator/ImpulseGenerator, representing phase incoherence from overlapping decays of multiple strikes.

**Diagnostic addition:** Add `primedness` as a new `std::atomic<float>` in DiagnosticState, displayable in the Energy Monitor tab alongside the band energy bars.

---

## Phase 3: Preset Architecture and Gong Type Parameters

These changes create a user-facing parameter structure that maps to physically distinct gong types, using the quantitative distinctions established in the research.

### 3.1 Nonlinear Onset Model (Dual Sigmoid)

**The problem:** The transition from tonal resonance to chaotic wash in a real gong is not a smooth gradient — it involves two distinct bifurcations with different characters. The current energy-to-parameter mappings likely use linear or simple power curves, which miss the threshold-like behaviour that all three reports identify.

**Physical basis:** The Ruelle–Takens scenario confirmed by Lyapunov exponent analysis (Touzé & Chaigne, Acta Acustica 86(3), 2000) establishes three regimes separated by two bifurcations:
1. Linear (w/h < 0.1–0.3): only directly excited modes present
2. Quasiperiodic (first bifurcation, w/h ≈ 0.3–1.0): combination tones appear, pitch glide begins
3. Chaotic (second bifurcation, w/h > 1.0–2.0): broadband spectrum, modes dissolve into noise

A shallow gong study reports "catastrophic change in behavior" at critical amplitudes with strong hysteresis (R3, citing Legge & Fletcher). The transition shows bifurcation-like thresholds, not smooth curves. [R1 §2, R2 §7, R3 §thresholds]

**What to modify:** `ModulationBus.h/.cpp` (curve shaping), `ModulationTypes.h` (new curve type), `DiagnosticState.h`

**Implementation detail:**

Add a new CurveType to ModulationTypes: `DualSigmoid`. This curve is defined by two sigmoid transitions with independent thresholds and steepness:

```
float dualSigmoid(float energy_normalised) {
    // First bifurcation: quasiperiodic onset at ~35% energy
    float sig1 = 1.0 / (1.0 + exp(-k1 * (energy_normalised - threshold1)));
    // Second bifurcation: chaotic onset at ~75% energy
    float sig2 = 1.0 / (1.0 + exp(-k2 * (energy_normalised - threshold2)));

    // Blend: 0–35% → 0, 35–75% → ramps to ~0.5, 75–100% → ramps to 1.0
    return 0.5 * sig1 + 0.5 * sig2;
}
```

Default parameters:
- `threshold1` = 0.35, `k1` = 10 (first bifurcation — moderate steepness)
- `threshold2` = 0.75, `k2` = 14 (second bifurcation — steeper, more sudden)

R1 §7 recommends k ≈ 8–12 for the first sigmoid, and suggests the steepness should be a design control: lower k for deep/thick gongs (gradual transition), higher k for thin/shallow gongs (sharper bifurcation).

This curve type should be available for any modulation route, but it is particularly intended for:
- Energy → resonator detune spread (cluster width blows out at second bifurcation)
- Energy → noise injection level (crash onset at second bifurcation)
- Energy → inter-band coupling gain (coupling activates at first bifurcation)

**Hysteresis:** R3 specifically flags that real gongs exhibit hysteresis — the exit threshold is lower than the entry threshold. Implement this as separate ascending/descending thresholds in the sigmoid:

```
// Track whether we're in ascending or descending energy
if (energy_normalised > threshold_enter) in_nonlinear = true;
if (energy_normalised < threshold_exit) in_nonlinear = false;
// threshold_exit = threshold_enter - hysteresis_width (e.g., 0.05–0.10)
```

This prevents rapid switching between regimes when energy hovers near the threshold. The hysteresis width should be a parameter (default 0.08 for first bifurcation, 0.05 for second).

**Add to DiagnosticState:** A `nonlinearRegime` atomic (0 = linear, 1 = quasiperiodic, 2 = chaotic) so the diagnostic window can display current regime state.

---

### 3.2 Resonator Frequency Ratio Presets (Modal Templates)

**The problem:** The 4 bands × 7 voices need to be tuned to frequency ratios that sound like specific gong types rather than arbitrary resonators. The research provides concrete modal data for three distinct gong geometries.

**Physical basis:** McLachlan (Acoustics Australia 25(3), 1997) measured a flat steel gong with ratios 1.00 : 1.67 : 1.98 : 2.93 : 3.91 : 4.85. The same study found a cast bronze bossed gong at 1.00 : 2.00 : 2.99 : 3.72 : 3.99 : 4.71. Krueger (JASA 128(1), 2010) confirmed a 1.96:1 ratio between first two axisymmetric modes of a Balinese gong ageng. Gamelan gongs have deliberately split mode pairs at ±1–3 Hz for ombak beating. [R1 §1, R2 §1, R3 §modal structure]

**What to modify:** `PresetManager.h/.cpp`, new preset data structure or configuration

**Implementation detail:**

Define three modal ratio templates that set the relative frequencies for each band and voice:

**Template A: Flat Tam-Tam** (extreme inharmonicity, dense upper spectrum)

```
Band 1 (Foundation): f₀ × [1.000, 0.997, 1.003, 0.994, 1.006, 0.991, 1.009]
    — tight cluster around fundamental with ±1–3 Hz twin-mode splitting
Band 2 (Body): f₀ × [1.670, 1.980, 2.930, 1.665, 1.975, 2.925, 2.940]
    — inharmonic anchors from McLachlan flat gong data, with ±2–5 Hz doublets
Band 3 (Shimmer): f₀ × [3.910, 4.850, 5.200, 3.920, 4.840, 5.210, 4.200]
    — dense inharmonic cluster, fast-decaying
Band 4 (Wash): f₀ × [7.300, 8.100, 9.500, 10.200, 7.350, 8.050, 11.000]
    — quasi-random cluster for turbulent/noise character
```

**Template B: Bossed Gong (Chau/Gamelan)** (near-harmonic low partials, tight tuning)

```
Band 1 (Foundation): f₀ × [1.000, 0.998, 1.002, 0.996, 1.004, 0.993, 1.007]
    — tight cluster, deliberate ombak beating at 2–5 Hz from split pairs
Band 2 (Body): f₀ × [2.000, 2.990, 3.720, 1.995, 2.985, 3.715, 3.990]
    — near-integer ratios from McLachlan bronze gong data
Band 3 (Shimmer): f₀ × [4.710, 5.700, 6.200, 4.720, 5.690, 6.210, 5.000]
    — still somewhat structured, less random than tam-tam
Band 4 (Wash): f₀ × [7.800, 8.500, 9.200, 10.100, 7.810, 8.490, 9.800]
    — high-frequency wash, less prominent than tam-tam Band 4
```

**Template C: Symphonic/Shallow Cap** (intermediate — stronger fundamental region than tam-tam, but retains inharmonic character)

```
Band 1 (Foundation): f₀ × [1.000, 0.998, 1.002, 0.995, 1.005, 0.992, 1.008]
Band 2 (Body): f₀ × [1.460, 1.950, 2.920, 1.455, 1.945, 2.910, 2.500]
    — from McLachlan steel-with-boss data, intermediate between harmonic and inharmonic
Band 3 (Shimmer): f₀ × [3.730, 4.500, 5.100, 3.740, 4.490, 5.110, 4.000]
Band 4 (Wash): f₀ × [6.500, 7.200, 8.800, 9.600, 6.510, 7.190, 10.200]
```

The 7 voices within each band represent: 1 centre voice, 3 pairs of twin-mode splits. The detuning within pairs (±0.002–0.010 of the base ratio) produces the beating that all three reports identify as essential for shimmer and ombak. R1 §5 states that hand-hammered gongs have ±0.5–3 Hz detuning between split pairs; machine-made gongs have near-zero splitting. This maps directly to the "hand-worked" parameter in §3.3.

These templates are stored in PresetManager as JSON arrays of frequency ratios, loadable per preset. The user-facing control is a "Gong Type" selector (tam-tam / bossed / symphonic) that loads the corresponding template and adjusts associated parameters (pitch glide sign, bifurcation thresholds, decay times).

---

### 3.3 Macro Parameter System

**The problem:** The synthesis engine now has many parameters. Exposing them all individually makes presets difficult to create and the instrument unintuitive. The research suggests a small number of high-level controls that coherently adjust multiple underlying parameters based on physical gong properties.

**Physical basis:** R2 §5 proposes specific macro structures. R1 §5 provides quantitative material property differences. R3 §physical argues that geometry dominates over material for nonlinear behaviour. All three reports converge on the idea that gong types can be parameterised by a small number of physical variables.

**What to modify:** `MainComponent.h/.cpp` (UI), `PresetManager.h/.cpp`, `GongSynthesizer.h/.cpp` (macro application logic)

**Implementation detail — five macro parameters:**

**Macro 1: Geometry (0.0 = flat tam-tam → 0.5 = shallow cap → 1.0 = deep boss)**

Controls:
- Modal ratio template: interpolates between Template A (0.0), Template C (0.5), and Template B (1.0) from §3.2. For intermediate values, linearly interpolate the frequency ratios.
- Pitch glide sign and magnitude: At 0.0 (flat), glide_coefficient = +200 cents (hardening, downward glide on decay). At 1.0 (bossed), glide_coefficient = -150 cents (softening, upward glide). At 0.5 (shallow cap), glide_coefficient = +50 cents (reduced hardening). [R1 §2, Fletcher 1985]
- Bifurcation threshold steepness: At 0.0 (thin flat gong), k1 = 12, k2 = 16 (sharp transitions). At 1.0 (stiff bossed gong), k1 = 6, k2 = 10 (gradual transitions). [R1 §7]
- Band 4 relative gain: At 0.0 (tam-tam), +3 dB. At 1.0 (bossed), -6 dB. Flat gongs have more high-frequency crash energy; bossed gongs are more focused.

**Macro 2: Size (0.0 = 60cm → 1.0 = 120cm)**

Controls primarily decay characteristics, using the T60 table from R1 §4:

| Size  | Band 1 T60    | Band 2 T60     | Band 3 T60    | Band 4 T60   |
|-------|---------------|----------------|---------------|--------------|
| 60cm  | 15–30 s       | 8–15 s         | 3–6 s         | 1–3 s        |
| 80cm  | 25–50 s       | 12–25 s        | 5–10 s        | 2–4 s        |
| 100cm | 40–90 s       | 20–40 s        | 8–15 s        | 3–6 s        |
| 120cm | 60–120+ s     | 30–60 s        | 10–20 s       | 4–8 s        |

Also controls: fundamental frequency range (larger gongs = lower fundamental), pitch glide magnitude (R1 §7: max shift 50–300 cents for small gongs, 10–50 cents for large tam-tams).

**Macro 3: Alloy (0.0 = B20 bronze → 1.0 = nickel-silver)**

Controls:
- All decay times: multiply by 0.6 at nickel-silver end. B20 has lower internal damping and longer sustain. [R1 §5, R2 §5]
- Band 3/4 relative gain: B20 → higher (more HF wash). Nickel-silver → lower (more controlled overtones). [R2 §5]
- Bifurcation threshold: B20 → lower threshold (easier to drive into chaos). Nickel-silver → higher threshold (more controlled, orderly transition). [R2 §5]
- Intra-band detune spread: B20 → wider (complex tension patterns in cast bronze). Nickel-silver → tighter.

**Macro 4: Hand-Worked (0.0 = machine-made → 1.0 = heavily hammered)**

Controls a single primary parameter: the random detuning between twin-mode pairs within each band. [R1 §5, R3 §physical]
- At 0.0: paired voices are detuned by ±0.1 Hz (near-zero beating, machine-made character)
- At 1.0: paired voices are detuned by ±1.5–3.0 Hz (strong ombak beating, hand-hammered character)
- Gamelan gong presets should have this set to 1.0, with specific ombak targets of 2–5 Hz from Krueger's measurements

Secondary effect: at higher hand-worked values, increase the stochastic_variation parameter in the coupling matrix (§1.1) from 0.1 to 0.3, reflecting the greater asymmetry in hand-hammered instruments that produces more irregular energy redistribution.

**Macro 5: Rim (0.0 = no rim / wind gong → 1.0 = deep turned-over rim / tam-tam)**

Controls:
- Band 1 decay time: multiplied by 0.2 at "no rim" (wind gongs decay extremely fast in the low band). At "deep rim", full T60 from the size table. [R2 §5, McLachlan FEA results showing rim depth dramatically raises frequencies of modes with nodal diameters]
- Inter-band cascade speed: at "no rim", the bloom attack time (§1.1) is shortened by 75% (wind gongs crash almost instantly). At "deep rim", full bloom delay. [R2 §5]
- Relative frequency balance between nodal-diameter and nodal-circle mode families: the rim selectively stiffens modes with nodal diameters. This maps to the frequency spacing between certain voice pairs within each band. [R1 §1, McLachlan FEA]

---

### 3.4 Per-Band Decay Time Configuration

**The problem:** Decay times in the current architecture may not reflect the strongly frequency-dependent damping of real gongs. All three reports emphasise that the fundamental can ring for 30–120 seconds while high-frequency modes die within 1–3 seconds.

**Physical basis:** The Ducceschi–Touzé damping model assigns per-mode coefficients: σ_n = α₀ + α₁ωₙ + α₂ωₙ², combining frequency-independent, viscoelastic, and radiation terms (Chaigne & Lambourg, JASA 109(4), 2001). High-frequency modes decay faster through three mechanisms: thermoelastic losses (increase with frequency), viscoelastic losses (material-dependent), and radiation losses (increase as ~f²). [R1 §4, R2 §4]

The four-phase temporal evolution from R1 §4: Phase 1 (Attack, 0–50ms) → Phase 2 (Nonlinear build, 50ms–2s) → Phase 3 (Mid sustain, 2–20s) → Phase 4 (Late sustain, 20s–minutes). For soft strikes, Phase 2 is largely absent.

**What to modify:** `EnergyAccumulator.h/.cpp` (per-band decay rate parameters)

**Implementation detail:**

The per-band decay rates should follow the inter-band ratio from R1 §4:
- Band 2 τ ≈ 0.3–0.5 × Band 1 τ
- Band 3 τ ≈ 0.1–0.2 × Band 1 τ
- Band 4 τ ≈ 0.05–0.1 × Band 1 τ

The absolute values are set by the Size macro (§3.3 Macro 2). The ratios are relatively constant across gong sizes, with bossed gongs having shorter overall decay than equivalently sized tam-tams (ratio ≈ 0.6–0.8).

Additionally, implement amplitude-dependent decay acceleration: in the nonlinear regime, effective damping increases because the energy cascade pumps energy to high frequencies where radiation damping is inherently stronger. Model this as:

```
effective_decay_rate[band] = base_decay_rate[band] * (1.0 + nonlinear_factor * amplitude_decay_coefficient)
```

Where `nonlinear_factor` is the output of the dual sigmoid from §3.1 and `amplitude_decay_coefficient` ≈ 0.5–1.5 (higher for high-frequency bands). This creates the asymmetric spectral centroid trajectory: centroid rises (cascade builds) then falls (high-frequency modes decay faster under increased damping). [R1 §2]

**Nonlinear build phase for Bands 3 and 4:** For loud strikes (velocity > 80/127), Bands 3 and 4 should exhibit an energy *increase* for 0.5–2 seconds before beginning exponential decay, with build duration proportional to strike energy. This is already handled by the inter-band coupling (§1.1), but the per-band decay should not fight the coupling — ensure that the coupling transfer rate can exceed the band's own decay rate during the build phase.

---

### 3.5 Damping/Muting Model

**The problem:** Performance practice requires physical damping (hand on surface). The current architecture likely implements damping as a uniform gain reduction. Real damping is strongly frequency-dependent — the hand covers a significant fraction of a wavelength for high-frequency modes but not for low-frequency modes.

**Physical basis:** Hand contact primarily damps high-frequency modes (short wavelengths). Low modes with large wavelengths persist. A 60cm gong can be stopped in 1–3 seconds with firm contact; a 100+ cm tam-tam requires body-muting and 3–10+ seconds. [R1 §4, R3 §decay]

**What to modify:** `EnergyAccumulator.h/.cpp`, `MainComponent.h/.cpp` (panic button / damping control)

**Implementation detail:**

Add a `damping` parameter (0.0 = free ring, 1.0 = fully muted). When active, multiply per-band decay times by frequency-dependent factors:

```
Band 1 (lows):  τ_damped = τ_free × lerp(1.0, 0.3, damping)   // lows persist
Band 2 (mids):  τ_damped = τ_free × lerp(1.0, 0.15, damping)
Band 3 (highs): τ_damped = τ_free × lerp(1.0, 0.05, damping)  // highs damped fast
Band 4 (air):   τ_damped = τ_free × lerp(1.0, 0.02, damping)  // nearly instant
```

The existing panic button could be reimplemented as `damping = 1.0` with a quick ramp rather than an instantaneous zero-out, which would sound more physical.

Map damping to MIDI CC 64 (sustain pedal, inverted: pedal down = free ring, pedal up = damped) or to aftertouch for expressive control.

---

## Phase 4: Advanced / Higher-Cost Additions

These additions provide significant quality improvements but require more engineering effort or new DSP infrastructure.

### 4.1 Multi-IR Crossfade

**The problem:** The single post-convolution EQ from §1.3 is a coarse approximation. The spectral character of a real gong body changes in complex ways with energy — not just a simple high-shelf. Different IR captures at different energy states contain different spectral envelopes, different spatial characteristics, and different decay profiles.

**Physical basis:** R1 §7 recommends three-tier IRs: 0–30% energy → clean/tonal IR; 30–60% → coloured/broad IR with +3–6 dB above 2 kHz; 60–100% → diffuse IR with +6–12 dB above 1 kHz and reduced resonance Q. The precedent is established in commercial products: MeldaProduction's MConvolutionMB and Liquidsonics' Fusion-IR technology. [R1 §7]

**What to modify:** `ConvolutionEngine.h/.cpp` (major modification)

**Implementation detail:**

Two approaches, in order of increasing fidelity:

**Approach A: Dual-IR with crossfade (recommended)**

Load two IRs: a "gentle" IR (captured from a soft strike, or the existing IR with aggressive highpass filtering to remove bloom) and a "full" IR (the existing 30s capture). Run two parallel convolution engines. Crossfade their outputs based on global energy level:

```
gentle_gain = cos(energy_normalised * PI / 2)  // equal-power crossfade
full_gain = sin(energy_normalised * PI / 2)
output = gentle_IR_output * gentle_gain + full_IR_output * full_gain
```

This doubles the convolution CPU cost but provides much more realistic body resonance evolution than the single-IR + EQ approach. The NonUniform partitioned FFT already handles 28s IRs within budget, so two instances may be feasible depending on overall CPU headroom.

**Approach B: Time-variant filtered IR (lower CPU)**

Instead of running two convolution engines, pre-compute filtered variants of the single IR at different energy levels and interpolate filter coefficients in real time. Apply a multi-band parametric EQ to the convolution output with energy-driven parameters. This is essentially a more sophisticated version of §1.3 with 3–4 filter bands instead of one shelf:

```
Low shelf (200 Hz): gain = lerp(0, +3, energy_normalised)
Mid peak (800 Hz): Q = lerp(5.0, 1.0, energy_normalised)  // Q narrows with energy
High shelf (2 kHz): gain = lerp(-12, 0, energy_normalised)
Air shelf (6 kHz): gain = lerp(-18, -3, energy_normalised)
```

This replaces the Phase 1 single shelf with a more nuanced spectral shaping that approximates different IR energy states without running multiple convolutions.

**Which approach to choose:** If CPU budget allows, Approach A provides noticeably better results because different energy-state IRs capture different spatial and temporal characteristics that EQ cannot synthesise. If CPU is tight, Approach B is a significant improvement over the single shelf from §1.3.

**IR preparation guidance:** Record two versions of the same gong sustain: (1) a gentle tap, lightly damped to capture the tonal body resonance without bloom; (2) a firm strike that captures the full nonlinear bloom and spatial development. Process both to remove the primary resonant frequencies (as already done for the existing IR). The difference between these two IRs IS the nonlinear spectral evolution that the crossfade reintroduces dynamically.

---

### 4.2 Broadband Crash/Noise Injection Layer

**The problem:** Above the second bifurcation threshold (~75% energy), real gongs transition into a regime where discrete modal peaks dissolve into a broadband noise continuum. This is the characteristic "crash" of a tam-tam. Resonator banks fundamentally cannot produce this — 28 bandpass filters, no matter how widely detuned, still produce 28 discrete peaks, not a continuous spectrum.

**Physical basis:** In the chaotic regime, the correlation dimension converges to 4–6, confirming deterministic chaos (Touzé & Chaigne, Acta Acustica 86(3), 2000). Discrete modal peaks dissolve into noise-like continuum. The velocity power spectral density follows a power law with exponent approximately −0.5 for undamped plates, steepening to roughly −1.0 with realistic damping (Humbert et al., Europhys. Lett. 102, 2013). [R1 §2, R2 §2, R3 §nonlinear]

**What to modify:** New class `CrashNoiseGenerator.h/.cpp`, `GongSynthesizer.h/.cpp` (mixing), `DiagnosticState.h`

**Implementation detail:**

Create a new lightweight DSP module: `CrashNoiseGenerator`. This generates shaped broadband noise that represents the chaotic regime content above the second bifurcation.

Signal chain:
```
White noise source (allocation-free, same approach as TestSignalGenerator)
  → Spectral shaping filter (pink-ish: −1.0 dB/octave slope per Humbert et al.)
  → Energy-dependent bandpass: centre frequency tracks spectral centroid
  → Amplitude envelope driven by (global energy − second_bifurcation_threshold)
  → Stereo decorrelation (independent L/R noise seeds)
```

The crash noise is mixed into the signal chain BEFORE the convolution engine, so the IR spatialises it and gives it the body resonance character of the gong. This is physically motivated — the crash is vibration of the gong body, not a separate sound source. Mixing point: after the ResonatorBank sum, before ConvolutionEngine input.

Amplitude envelope:

```
crash_level = max(0.0, sigmoid_2(energy_normalised) - 0.5) * 2.0 * crash_gain
// Where sigmoid_2 is the second sigmoid from §3.1
// This means crash is completely silent below ~70% energy,
// and ramps up through the second bifurcation
```

`crash_gain` should be a preset parameter, defaulting to ~0.3 (crash supplements but doesn't overwhelm the resonators). For tam-tam presets, set higher (~0.5). For bossed gong presets, set lower (~0.1).

The spectral shape of the crash noise should evolve with energy: at moderate crash levels (just above threshold), the noise is bandpass-limited around the mid/high range. At extreme energy, the bandwidth widens to nearly full-range, reflecting the power-law spectrum of fully developed wave turbulence.

**Hysteresis on crash exit:** Use the hysteresis system from §3.1 — the crash doesn't cut out immediately when energy drops below threshold. Instead, the exit threshold is ~5–8% lower than the entry threshold, and the crash amplitude decays over ~500ms when exiting. This prevents the crash from "chattering" on and off when energy hovers near threshold.

---

### 4.3 Combination Tone Resonators

**The problem:** In the quasiperiodic regime (between the two bifurcations), real gongs generate sum and difference tones from nonlinear mode coupling. These combination tones are distinct from the crash noise — they're discrete, identifiable frequencies that arise from the interaction of existing modes. The combination resonance rule: pΩ = aᵢωᵢ + aⱼωⱼ where |aᵢ| + |aⱼ| = 2.

**Physical basis:** Chaigne et al. (2005) identified that only 5–10 strongly coupled modes drive the quasiperiodic regime. Sum and difference frequencies (ω₁ + ω₂, ω₁ − ω₂, 2ω₁, 2ω₂) appear at the first bifurcation. Subharmonics at f/2 appear first via period doubling; f/3 and f/5 have been observed experimentally (Legge & Fletcher, JASA 86(6), 1989). Second harmonic amplitude scales as (fundamental)²; third harmonic as (fundamental)³ (Krueger, 2009). [R1 §2, R3 §mode coupling]

**What to modify:** New class `CombinationToneBank.h/.cpp`, `GongSynthesizer.h/.cpp`

**Implementation detail:**

Add a small secondary resonator bank (4–8 additional bandpass voices) whose frequencies are derived from the primary resonator frequencies. These combination tone resonators are not independently tuned — their frequencies are computed from the Band 1 and Band 2 centre frequencies:

```
combo_freqs[] = {
    f_band1 * 2.0,                    // 2nd harmonic of fundamental
    f_band1 * 3.0,                    // 3rd harmonic
    f_band1 + f_band2_centre,         // sum tone
    abs(f_band2_centre - f_band1),    // difference tone
    f_band1 * 0.5,                    // subharmonic (period doubling)
    f_band2_centre * 2.0,             // 2nd harmonic of body
}
```

These are driven not by direct excitation but by the energy in Bands 1 and 2, gated by the first bifurcation sigmoid:

```
combo_amplitude[i] = combo_gain * sigmoid_1(energy_normalised) * pow(E_source / E_max, exponent[i])
// exponent = 2.0 for 2nd harmonic, 3.0 for 3rd harmonic (per Krueger scaling law)
// exponent = 1.5 for sum/difference tones
// exponent = 2.5 for subharmonic (appears later, requires more energy)
```

The combination tone bank output is summed with the main resonator bank output before convolution.

This is a relatively low-cost addition (6–8 bandpass filters vs the existing 28) but adds the crucial "new frequencies appearing" quality that distinguishes the quasiperiodic regime from simple resonator ringing.

**For opera gong presets:** R3 specifically notes that 1:2 internal resonances between the fundamental and modes at ~2× strongly influence observed pitch-glide behaviour. The combination tone at `f_band1 * 2.0` interacts with Band 2's centre frequency, producing beating when they're close but not identical. This is a perceptually important detail for opera gong presets.

---

### 4.4 Bloom Timing Model (t^(1/3) Cascade Law)

**The problem:** The one-pole lowpass slew limiter in §1.1 is a linear approximation of the bloom timing. The physical cascade front propagates as t^(1/3), which is a different shape — it rises more slowly initially and then accelerates, whereas exponential rise (one-pole) is fastest at the start and decelerates.

**Physical basis:** Ducceschi et al. (Physica D 280–281, 2014) showed through numerical simulation that the wave turbulence cascade front propagates to higher frequencies as t^(1/3). [R1 §2]

**What to modify:** `EnergyAccumulator.h/.cpp` (replace one-pole slew with t^(1/3) envelope)

**Implementation detail:**

Replace the one-pole lowpass on inter-band transfer with a shaped envelope:

```
// Track time since energy exceeded coupling threshold
if (E_source > coupling_threshold && !cascade_active) {
    cascade_active = true;
    cascade_start_time = current_time;
}
if (E_source < coupling_threshold * (1.0 - hysteresis_width)) {
    cascade_active = false;
}

if (cascade_active) {
    float elapsed = current_time - cascade_start_time;
    // t^(1/3) rise: slow start, accelerating
    float cascade_envelope = pow(elapsed / bloom_duration, 1.0 / 3.0);
    cascade_envelope = min(1.0, cascade_envelope);
    effective_coupling = base_coupling * cascade_envelope;
}
```

Where `bloom_duration` is the time to reach full cascade (1.0–2.0 seconds for tam-tams, shorter for smaller/thinner gongs). This produces a bloom that starts gently and accelerates — matching the physical observation that the shimmer "opens up" gradually and then hits a peak. The one-pole exponential rise from §1.1 would reach 63% of maximum in one time constant, while t^(1/3) reaches 63% at (0.63)^3 = 25% of bloom_duration — a much slower start followed by faster completion.

The `bloom_duration` parameter should be modulated by the primedness variable from §2.3: `effective_bloom_duration = bloom_duration * (1.0 - 0.7 * primedness)`.

---

## Parameter Summary Table

All new parameters introduced by this brief, with their locations, ranges, and defaults:

| Parameter | Location | Range | Default (Tam-Tam) | Default (Bossed) |
|---|---|---|---|---|
| coupling_matrix[4][4] | EnergyAccumulator | 0.0–0.5 | See §1.1 table | Same × 0.6 |
| coupling_threshold | EnergyAccumulator | 0.0–1.0 | 0.35 | 0.45 |
| stochastic_variation | EnergyAccumulator | 0.0–0.5 | 0.2 | 0.15 |
| bloom_attack_time | EnergyAccumulator | 0.1–5.0 s | 1.0 s | 0.5 s |
| glide_coefficient | ModulationBus route | −300 to +300 cents | +150 cents | −100 cents |
| glide_band_scale[4] | ModulationBus | 0.0–1.0 | [1.0, 0.6, 0.2, 0.05] | [1.0, 0.5, 0.15, 0.03] |
| jitter_amplitude | ModulationBus | 0.0–5.0% | 1.0% | 0.5% |
| jitter_rate_range | ModulationBus | Hz range | 5–50 Hz | 5–30 Hz |
| post_IR_shelf_freq | ConvolutionEngine | 500–5000 Hz | 1500 Hz | 2000 Hz |
| post_IR_shelf_cut_dB | ConvolutionEngine | −24 to 0 dB | −12 dB | −8 dB |
| mallet_hardness | MidiControllerMock | 0.0–1.0 | 0.3 (soft felt) | 0.4 |
| strike_position | StrikeDescriptor | 0.0–1.0 | 0.0 (centre) | 0.0 |
| priming_increment | EnergyAccumulator | 0.0–0.5 | 0.15 | 0.15 |
| priming_decay_time | EnergyAccumulator | 1.0–10.0 s | 4.0 s | 3.0 s |
| sigmoid_threshold1 | ModulationBus/curves | 0.1–0.6 | 0.35 | 0.45 |
| sigmoid_threshold2 | ModulationBus/curves | 0.5–0.95 | 0.75 | 0.85 |
| sigmoid_k1 | ModulationBus/curves | 4–20 | 10 | 7 |
| sigmoid_k2 | ModulationBus/curves | 4–25 | 14 | 10 |
| hysteresis_width_1 | ModulationBus/curves | 0.0–0.15 | 0.08 | 0.06 |
| hysteresis_width_2 | ModulationBus/curves | 0.0–0.10 | 0.05 | 0.04 |
| hand_worked | Preset macro | 0.0–1.0 | 0.6 | 0.9 |
| twin_mode_detune_Hz | SpreadVoiceResonator | 0.0–5.0 Hz | 1.5 Hz | 2.5 Hz |
| damping | EnergyAccumulator | 0.0–1.0 | 0.0 | 0.0 |
| crash_gain | CrashNoiseGenerator | 0.0–1.0 | 0.4 | 0.1 |
| crash_spectral_slope | CrashNoiseGenerator | −2.0 to 0.0 dB/oct | −1.0 | −1.0 |
| combo_tone_gain | CombinationToneBank | 0.0–1.0 | 0.25 | 0.15 |
| bloom_duration | EnergyAccumulator | 0.2–5.0 s | 1.5 s | 0.8 s |

---

## Preset Manager Version Update

PresetManager currently uses Version 2 format (with "modulation" key). This brief introduces enough new parameters to warrant a Version 3 format. The new keys required:

```json
{
    "version": 3,
    "gongType": "tam-tam",
    "macros": {
        "geometry": 0.0,
        "size": 0.7,
        "alloy": 0.0,
        "handWorked": 0.6,
        "rim": 0.9
    },
    "coupling": {
        "matrix": [[0, 0.05, 0.20, 0.10], ...],
        "threshold": 0.35,
        "stochasticVariation": 0.2,
        "bloomAttackTime": 1.0,
        "bloomDuration": 1.5,
        "bloomShape": "t_one_third"
    },
    "pitchGlide": {
        "coefficient": 150,
        "bandScale": [1.0, 0.6, 0.2, 0.05],
        "jitterAmplitude": 1.0,
        "jitterRateMin": 5,
        "jitterRateMax": 50
    },
    "nonlinearOnset": {
        "threshold1": 0.35,
        "k1": 10,
        "threshold2": 0.75,
        "k2": 14,
        "hysteresis1": 0.08,
        "hysteresis2": 0.05
    },
    "modalTemplate": "tam-tam",
    "modalRatios": { ... },
    "excitation": {
        "malletHardness": 0.3,
        "amplitudeExponent": 1.8,
        "contactTimeMax": 15.0
    },
    "crash": {
        "enabled": true,
        "gain": 0.4,
        "spectralSlope": -1.0
    },
    "combinationTones": {
        "enabled": true,
        "gain": 0.25
    },
    "convolution": {
        "irMode": "single",
        "postShelfFreq": 1500,
        "postShelfCut": -12
    },
    "damping": {
        "bandMultipliers": [0.3, 0.15, 0.05, 0.02]
    },
    "modulation": { ... }
}
```

Backward compatibility: Version 2 presets load without any of the new keys; all new parameters fall back to defaults. This maintains the pattern already established between V1 and V2.

---

## Validation Strategy

Each phase should be validated before proceeding:

**Phase 1 validation:**
- Play a single hard MIDI strike and listen for the bloom: high-frequency content should swell 0.5–2 seconds after the strike, peak, and then decay. Compare against a recording of a real tam-tam strike.
- Play the same strike at pp and ff: the pp strike should sound dark and tonal; the ff strike should bloom and shimmer. The difference should be qualitative (different character), not just quantitative (different volume).
- Enable pitch glide and verify that the fundamental audibly shifts during loud strikes and returns to centre as the sound decays.
- Toggle inter-band coupling on/off and confirm the difference is immediately audible.

**Phase 2 validation:**
- Play a sustained MIDI roll (rapid repeated notes at moderate velocity): the sound should build into a wash over 3–5 seconds. Compare against a single loud strike at the same total energy — the roll should sound "wider" and "noisier" while the single strike should have a cleaner bloom.
- Vary mallet hardness from soft to hard on the same note: soft should produce a dark thud that blooms slowly; hard should produce an immediate bright crack.

**Phase 3 validation:**
- Switch between tam-tam and bossed gong presets: the tam-tam should sound crashy, inharmonic, and wide; the bossed gong should sound pitched, focused, and warm.
- Adjust the hand-worked macro from 0 to 1: shimmer/beating should increase smoothly.

**Phase 4 validation:**
- Enable crash noise injection and play at extreme velocity: above the second bifurcation threshold, the sound should transition from resonant to noise-like. Below threshold, no noise should be audible.
- Compare single-IR + EQ against dual-IR crossfade: the dual-IR version should have more convincing spatial evolution and body character changes with energy.

---

## Key Literature References

- Chaigne, Touzé & Thomas, "Nonlinear vibrations and chaos in gongs and cymbals," Acoust. Sci. Technol. 26(5), 2005
- Fletcher, "Nonlinear frequency shifts in quasispherical-cap shells: Pitch glide in Chinese gongs," JASA 78, 1985
- Fletcher, "The sound of music: Order from complexity," Acoustics Australia 40(3), 2012
- Jossic et al., "Effects of internal resonances in the pitch glide of Chinese gongs," JASA 144(1), 2018
- Krueger, "Acoustical and vibrometry analysis of a large Balinese gamelan gong," JASA 128(1), 2010
- Legge & Fletcher, "Nonlinearity, chaos, and the sound of shallow gongs," JASA 86(6), 1989
- McLachlan, "Finite element analysis and gong acoustics," Acoustics Australia 25(3), 1997
- Poirot, Bilbao & Kronland-Martinet, coupled resonant filter banks, EURASIP J. Audio Speech Music Proc., 2024
- Touzé & Chaigne, "Lyapunov exponents from experimental time series," Acta Acustica 86(3), 2000
- Ducceschi et al., "Dynamics of the wave turbulence spectrum in vibrating plates," Physica D 280–281, 2014
- Chaigne & Lambourg, "Time-domain simulation of damped impacted plates," JASA 109(4), 2001
- Bilbao et al., "Real-time gong synthesis," DAFx23, 2023
- Humbert et al., "Wave turbulence in vibrating plates," Europhys. Lett. 102, 2013
- Ducceschi & Touzé, "Modal approach for nonlinear vibrations of damped impacted plates," J. Sound Vib. 344, 2015
