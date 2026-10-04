# Gong residual-IR analysis and resynthesis protocol

## Purpose

Replace the **recorded residual gong impulse response** with a generated or resynthesized one, while leaving the existing working system intact:

```text
contact-mic strike -> tunable resonators -> residual IR/convolution -> output
```

The resonators already provide the intended pitched material. This work is only about the recording used after iZotope RX has removed the small number of dominant resonant frequencies: the non-pitched, metallic, diffuse background that makes the result still feel like a gong.

The intended outcome is a **residual-character generator** that can render several new IRs with the same perceptual role as the recorded residual, without retaining the original gong's fixed frequencies or waveform.

## Scope and non-goals

In scope:

- analyse the RX-treated residual recording(s);
- determine which properties make the residual feel gong-like;
- create progressively less sample-dependent synthetic residual IRs;
- audition those IRs through the existing Gong signal path;
- retain the ability to make several distinct, controllable residual characters.

Out of scope for this experiment:

- rebuilding the current resonator/excitation split;
- recreating a whole gong from scratch;
- recovering the removed primary modes;
- full finite-difference plate simulation;
- introducing ML before a simple DSP baseline has been evaluated.

This is a **sinusoidal/modal + residual** experiment. The main resonances are intentionally supplied elsewhere. The residual is treated as a structured stochastic texture, not as a missing modal model.

## Working hypothesis

The RX residual contains a mixture of:

1. a short impact/scatter component;
2. many weak, dense, high-order resonances that are perceived collectively rather than as pitches;
3. nonlinear sidebands or shimmer, especially at higher energy;
4. frequency-dependent decay and darkening over time;
5. stereo radiation, microphone, and possibly room character.

It is therefore not sufficient to use static filtered white noise. But it may be sufficient to match the residual's **time-varying spectrum, decay, micro-resonance density, and stereo behaviour** without reproducing its exact waveform.

## Key principles

1. **Do not change the current working instrument while testing.** Each experiment substitutes only the IR file or a parallel residual layer.
2. **Do not learn the RX holes.** The removed frequencies are tied to the source gong and RX process. Interpolate through those holes when estimating broad residual statistics.
3. **Use offline renders first.** They are faster to compare, reproducible, and avoid real-time DSP concerns.
4. **Test perceptual sufficiency before adding complexity.** Start with phase-randomized and noise-based versions; add micro-resonators only if they prove necessary.
5. **Keep every result.** A failed experiment is useful evidence about which information the residual needs.

## Files required before Experiment 1

Minimum set, all from the same original hit:

| File | Why it is needed |
|---|---|
| `original.wav` | Pre-RX gong recording, untrimmed if possible. |
| `residual-rx.wav` | The exact RX-treated residual currently used as the IR. |
| RX settings or a screenshot | Identifies the bands and width of the deliberately removed modes. |
| Current Gong preset / settings | Lets us render a controlled reference through the working instrument. |
| A short reference render | The current resonators through `residual-rx.wav`, ideally one soft and one hard strike. |

Strongly recommended:

| File / information | Why it improves the work |
|---|---|
| 5–20 matched original/residual pairs | Separates stable gong character from quirks of a single hit. |
| Different strike strengths | Reveals whether one static residual is sufficient. |
| Different strike positions or mallets | Reveals which variation is desirable versus unwanted. |
| Recorded contact mic alongside stereo air mics | Separates plate detail from radiation/room character. |
| Main frequencies removed in RX | Used only to mask/inpaint analysis regions, never to resynthesize them. |

Use WAV/AIFF at the original sample rate and bit depth. Do not normalise, denoise, dither, or otherwise alter a source file after exporting it for this study.

## Suggested experiment workspace

Once source files are supplied, keep a separate, additive workspace. Never overwrite the original IR.

```text
Gong/Research/residual-ir/
  input/
    original/
    rx-residual/
    rx-settings/
    reference-renders/
  analysis/
    plots/
    features/
    notebooks-or-scripts/
  renders/
    EXP-01-phase-randomised/
    EXP-02-banded-noise/
    EXP-03-micro-modal/
    EXP-04-profile-variants/
  logs/
    EXP-01.md
    EXP-02.md
```

The source of truth is the original files; generated IRs are disposable experimental outputs.

## Standard reference render

Before analysing anything, make a repeatable reference render using the existing, successful path.

1. Save the current Gong preset under a non-destructive experimental name.
2. Use one fixed input/strike for a **soft** reference and one for a **hard** reference.
3. Render at least 30 seconds or until the existing IR has fully decayed.
4. Save the source IR name, sample rate, wet/dry setting, output gain, and all resonator settings with the file.
5. Do not alter these settings during a given experiment batch.

This makes each candidate answer one question: *does the new residual provide the same useful background role behind the same tuned resonators?*

## Analysis phase

### A. File conditioning

For each source pair:

1. Check sample rate, channel count, bit depth, duration, leading silence, peak level, and DC offset.
2. Time-align `original.wav` and `residual-rx.wav` if RX introduced latency.
3. Preserve the full original duration in a master copy. A trimmed analysis copy is acceptable if its offset is logged.
4. Derive a provisional removed signal only for inspection:

   ```text
   removed(t) = original(t) - residual(t)
   ```

   This is not guaranteed to be mathematically exact because RX processing may be nonlinear or phase-sensitive. It is still useful for locating the removed bands and checking the subtraction.

### B. Generate a residual analysis sheet

For every residual file, export the following plots and values:

- waveform and cumulative energy over time;
- spectrogram at a short window (about 20–50 ms) and a long window (about 0.5–1 s);
- spectral centroid, roll-off, flatness, and slope versus time;
- energy-versus-time in 16–32 ERB or Bark bands;
- decay curves per band, fitted with early and late exponentials where useful;
- local spectral peak density and peak-width distribution per band;
- left/right correlation and mid/side energy versus time;
- frequency-dependent stereo coherence where available.

The important output is not an exhaustive list of individual frequencies. It is a compact description of how the **texture** changes from the attack to the late tail.

### C. Remove the RX fingerprint from the statistics

The residual has intentionally missing energy around the original gong's dominant modes. Do not preserve those fixed holes in a new general-purpose residual IR.

For each analysis frame:

1. Mark the RX-removed regions from the settings/mask or the original-minus-residual comparison.
2. Estimate the broad spectral envelope from neighbouring bands in log-frequency or ERB space.
3. Inpaint those regions only in the **feature representation** used for synthesis.
4. Keep a second, non-inpainted analysis copy for comparison.

The goal is to retain the residual's material and decay character, not the old gong's negative EQ curve.

## Experiment sequence

### EXP-01 — Phase-randomized residual

**Question:** Is the residual primarily characterized by its time-varying magnitude spectrum, or is fine phase/microstructure essential?

**Method:**

1. Take `residual-rx.wav`.
2. Compute an overlap-add STFT with a suitable analysis window.
3. Preserve the magnitude in every time-frequency bin.
4. Replace phase with controlled random phase while retaining channel relationship as far as possible. Start with mid/side processing rather than independently randomizing left and right.
5. Invert the STFT to make 3–5 phase-seeded IR candidates.
6. Load each candidate into the existing convolver and render the standard soft/hard references.

**Listen for:**

- Does it retain metallic depth and a real-gong sense of air?
- Does it become a generic hiss or reverb wash?
- Is the stereo field still convincing?
- Does a candidate reveal an obvious fixed pitch from the original gong?

**Decision gate:**

- If phase-randomized candidates are close enough, the residual is largely a spectral-statistical problem. Move directly to EXP-02.
- If they lose the essential metallic quality, retain the result as evidence and add a micro-modal component in EXP-03.

### EXP-02 — Banded stochastic residual IR

**Question:** Can a generated, non-sample-based IR reproduce the role of the residual?

**Model:**

```text
white/pink noise burst
  -> 16–32 frequency bands
  -> independent time-varying amplitude envelopes
  -> stereo correlation / diffusion stage
  -> rendered stereo IR
```

**Procedure:**

1. Fit a smooth amplitude envelope for each ERB/Bark band from the inpainted residual statistics.
2. Use one or two decay sections per band: fast attack/early decay plus slower late tail.
3. Filter independent noise streams into those bands and apply the fitted envelopes.
4. Create stereo with a controlled common-versus-independent component per band; add only subtle decorrelation/diffusion at first.
5. Render at least three different random seeds with the same profile.
6. Test in the existing convolver against the standard reference renders.

**Controls to expose:**

- overall duration;
- spectral brightness / tilt;
- low-, mid-, and high-band decay multipliers;
- width / coherence;
- early scatter amount;
- seed.

**Decision gate:**

- If variants feel like plausible new gong ambiences and survive arbitrary retuning, this is the first viable residual-generator model.
- If they are too smooth, too noisy, or insufficiently metallic, proceed to EXP-03.

### EXP-03 — Micro-modal cloud

**Question:** Does the residual need many weak, unresolved resonances rather than filtered noise alone?

**Model:**

```text
banded stochastic residual
  + 50–250 quiet, short-lived micro-resonators
  + optional sparse high-frequency scatter events
```

**Procedure:**

1. From the analysis sheet, estimate distributions rather than exact peaks:

   - resonator density per ERB band;
   - gain distribution;
   - Q / bandwidth distribution;
   - decay-time distribution;
   - stereo placement/coherence distribution.

2. Randomly draw a cloud of weak modes from those distributions for each rendered IR.
3. Keep individual gains low enough that no isolated mode is perceived as a primary pitch.
4. Randomize frequencies and phases per seed; do not keep the original gong's fixed micro-peaks.
5. Add the cloud beneath the EXP-02 noise layer and render a small family of candidates.

**Listen for:**

- metallic grain rather than broadband hiss;
- a sense of depth without an identifiable original gong note;
- variety between seeds without random unpleasant whistles;
- compatibility with several unrelated resonator tunings.

**Decision gate:**

- Keep this layer only if blind or repeated listening shows it materially improves the result over EXP-02.

### EXP-04 — Profile variants and source independence

**Question:** Can one analysis yield a controllable family rather than a single imitation?

Start with the best model from EXP-02 or EXP-03 and make systematic variants:

| Parameter | Low end | High end |
|---|---|---|
| Brightness | dark, low-centred texture | bright, extended metallic air |
| Density | sparse/smoky | granular/shimmering |
| Tail | short/contained | long/blooming |
| Width | focused | diffuse/wide |
| Scatter | smooth | fractured/active |
| Seed | repeatable | new material per render |

Render a small grid, not a giant library: 6–12 candidates is enough for first listening. The desired result is that they sound related in material, not copied from the same recorded gong.

### EXP-05 — Energy dependence, only if necessary

**Question:** Does one static synthetic IR work for both soft and hard strikes?

If the answer is no, do not immediately redesign the audio engine. First render 2–3 residual profiles:

- low-energy: darker, sparser, shorter;
- medium-energy: nominal;
- high-energy: brighter, denser, more scatter, possibly longer high-frequency persistence.

Use the existing dual-IR crossfade mechanism to test energy-driven morphing. A live parallel residual generator is only justified if this static-bank test proves insufficient.

## Test protocol

For every candidate IR, use the same test matrix:

| Test | Purpose |
|---|---|
| Soft strike, original tuning | Match the known successful reference. |
| Hard strike, original tuning | Reveal harshness and shimmer behaviour. |
| Soft strike, unrelated tuning | Check that no original gong pitch remains. |
| Hard strike, unrelated tuning | Check that the texture stays metallic when pushed. |
| At least two random seeds | Check that variation is musical rather than arbitrary. |
| Mono fold-down | Identify phase tricks that fail outside stereo. |

Use blind filenames whenever possible. Rate each render 1–5 for:

- gong-like metallic background;
- independence from the original gong's pitch;
- compatibility with arbitrary tuning;
- stereo depth;
- harshness / fatigue;
- desire to keep using it musically.

## Success criteria

The first successful model does not need to be acoustically identical to the original residual. It should:

1. sit behind the current resonators with the same useful metallic, spacious role;
2. avoid imposing an audible fixed pitch from the source gong;
3. produce several musically coherent variants from a small set of controls and seeds;
4. remain convincing at more than one tuning and strike strength;
5. be renderable as a WAV IR without changing the working real-time audio path.

## Experiment log template

Create one file per experiment, for example `logs/EXP-02.md`:

```markdown
# EXP-02 — Banded stochastic residual

## Hypothesis

## Inputs

## Analysis settings

## Synthesis settings

## Candidate files

## Listening setup

## Results

## Decision

## Next experiment
```

Always record the random seed, sample rate, duration, and exact profile settings for a rendered candidate.

## First working session

When the minimum file set is available, proceed in this order:

1. Verify and catalogue the source files.
2. Make the standard current-system reference renders.
3. Generate the analysis sheet for `residual-rx.wav`.
4. Run EXP-01 with 3–5 phase-randomized candidates.
5. Listen and make the first decision gate before writing a full resynthesis engine.

That first phase test prevents us from spending time on a sophisticated model before knowing whether the residual's identity survives loss of exact waveform phase.

## Longer-term direction

If the model works, its output should become a compact **residual character profile**, rather than a collection of samples:

```text
profile = {
  band_energy_envelopes,
  decay_distribution,
  spectral_tilt_over_time,
  micro_modal_density,
  stereo_coherence,
  energy_variants,
  random_seed
}
```

The current convolver can load rendered profile outputs as IRs. Later, if the musical benefit warrants it, the same profile can drive a dynamic residual layer in parallel with the convolver.

## Related background

The ResearchVault physical-modelling landscape recommends a practical hybrid direction for percussion and explicitly identifies a structured physical core plus a learned residual as the direction of travel. It is useful context, but this protocol deliberately begins with simple, audibly testable DSP rather than a neural or full-physics model:

- [ResearchVault: physical-modelling synthesis landscape](/Users/aidan/Dropbox/ResearchVault/dossiers/2026-06-06-physical-modelling-synth-landscape/dossier.md:398)

