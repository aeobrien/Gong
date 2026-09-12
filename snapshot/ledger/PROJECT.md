# Gong — Ledger

> Programmable digital gong instrument: physical MDF striking surfaces with contact mics feed software-defined resonators, convolution reverb, and effects on a Raspberry Pi 5, with a circular touch screen for live preset control.

## Status

**Lane:** personal
**Phase:** Mid-prototype — audio engine functional on desktop, hardware integration and Pi deployment not yet started
**Last updated:** 2026-04-04

## Subsystems

| Subsystem | Status | Doc |
|-----------|--------|-----|
| Audio Engine (JUCE/C++) | Functional on desktop | [Codebase Overview](../Gong%20—%20Codebase%20Overview%20Gong.md), [HANDOVER](../HANDOVER.md) |
| Resonator Bank | Working — 4x7 voice bandpass | Part of audio engine |
| Energy Accumulator | Working — leaky integrator model | Part of audio engine |
| Convolution Engine | Working — 28s+ IRs confirmed | Part of audio engine |
| Nonlinear Dynamics | Specified, not implemented | [Implementation Brief](../gong-synth-implementation-brief.md) |
| Strike Capture (Teensy 4.0) | Designed, not built | [Technical Brief](../Gong%20—%20Technical%20Brief.md) |
| Signal Conditioning (TL074) | Designed, not built | [Technical Brief](../Gong%20—%20Technical%20Brief.md) |
| Synthetic Impulse Generator | Code exists (`SyntheticImpulseGenerator.cpp`) | Part of audio engine |
| Performance UI (Web) | Early implementation (`web/`, `PerformanceServer.cpp`) | — |
| Desktop GUI (JUCE) | Working — dev/sound design tool | `src/ui/`, `MainComponent.cpp` |
| Modulation Bus | Working — 9 sources, 19 targets | `ModulationBus.cpp` |
| Preset System | Early — `PresetManager.cpp`, one default preset | `presets/` |
| Physical Assembly | Fully specified, not built | [Mounting Assembly Plan](../Gong%20—%20Mounting%20Assembly%20Technical%20Plan.md) |
| Circular Display (Waveshare 5") | Not yet acquired/integrated | — |

## Key Decisions

See [decisions/LOG.md](decisions/LOG.md) for the full decision log.

## Linked Projects

| Project | Relationship | Notes |
|---------|-------------|-------|
| DronemakerClonev3 | related-to | Both Pi-based instruments for improv music; potential shared hardware platform |
| Moire | related-to | Both Pi-based instruments; potential shared hardware platform |

## Open Questions

1. **Contact mic behaviour on MDF** — The engine was developed against laptop mic input. Contact mics on 25mm MDF will have fundamentally different characteristics. Highest-risk integration point.
2. **Pi 5 audio performance** — Full DSP chain (28 resonator voices, convolution, exciter, compressor) not yet benchmarked on Pi 5. May need NonUniform convolution mode or voice count reduction.
3. **Synthetic impulse quality** — Will resonators respond convincingly to synthetic impulses from strike descriptors? Can be tested on desktop before hardware integration.
4. **Circular screen UI usability** — 5" circular form factor may be too constrained for mid-performance use with one hand on a mallet.
5. **Vibration isolation effectiveness** — Rubber grommet mounting system untested. Crosstalk between segments sharing a steel frame is an empirical question.
6. **Clothes rail upright tube diameter** — Needs confirming on arrival (19mm, 25mm, or 32mm). Affects crossbar sizing.
7. **Screen and Pi mounting in central cutout** — Must not create a rigid vibration path. Needs its own isolation or bracket from crossbars.
8. **Surface veneer material and application** — Affects final weight; may need pre-application before drilling MDF.
9. **Grommet stiffness/durometer** — Softer = better isolation but more wobble under striking.
10. **JUCE DryWetMixer doesn't work** — Manual dry/wet mixing implemented as workaround. Root cause not investigated.

## Notes

### Existing Documentation
The project has extensive documentation predating the Ledger:
- **[Vision Statement](../Gong%20—%20Vision%20Statement.md)** — Project intent, motivation, audience, scope, design principles, definition of done
- **[Technical Brief](../Gong%20—%20Technical%20Brief.md)** — Full architecture, tech stack, data model, integration points, constraints, 11-phase implementation order
- **[Codebase Overview](../Gong%20—%20Codebase%20Overview%20Gong.md)** — Module inventory, tech stack, current state summary
- **[HANDOVER.md](../HANDOVER.md)** — Developer handover with build instructions and what's working
- **[Implementation Brief](../gong-synth-implementation-brief.md)** — Detailed spec for nonlinear dynamics (4 phases: energy coupling, pitch glide, spectral compensation, advanced features)
- **[Mounting Assembly Plan](../Gong%20—%20Mounting%20Assembly%20Technical%20Plan.md)** — Complete physical build spec with BOM, dimensions, assembly procedure
- **[Hybrid Resonator+IR Design Brief](../Hybrid%20Resonator+IR%20Percussion%20Synthesizer%20–%20Design%20Brief.pdf)** — PDF design brief

### Code Structure
Flat `src/` layout with ~40 source/header files. Key additions beyond the HANDOVER doc: `CombinationToneBank`, `CrashNoiseGenerator`, `DiagnosticWindow`, `MacroParameters`, `MidiControllerMock`, `ModalTemplate`, `ModulationBus`, `PerformanceServer`, `PresetManager`, `StrikeDescriptor`, `SyntheticImpulseGenerator`, `TestSignalGenerator`, plus a `src/ui/` directory with custom GUI components (knobs, meters, energy ring, resonator grid, patch cables, module panels).

### Research
The `Research/` directory exists but is currently empty. Three research reports (R1, R2, R3) are referenced in the implementation brief but stored elsewhere.
