# Decision Log

| # | Date | Decision | Rationale | Alternatives Considered |
|---|------|----------|-----------|------------------------|
| 001 | Pre-Ledger | Teensy 4.0 as strike preprocessor, not raw audio into Pi | Keeps Pi CPU budget for DSP. Deterministic sub-7ms strike-to-descriptor latency. Avoids multi-channel USB audio interface. | Raw audio into Pi via USB audio interface |
| 002 | Pre-Ledger | Synthetic impulse injection into existing pipeline | Preserves the entire working signal chain without modification. Descriptors shape amplitude, spectral content, envelope, and damping. | Reworking the engine to accept descriptors directly |
| 003 | Pre-Ledger | Web-based performance UI (HTML/CSS/JS on embedded server) | CSS provides best tools for circular display masking and touch-fluid interface. Keeps UI decoupled from audio engine. | Native JUCE GUI on Pi |
| 004 | Pre-Ledger | Headless JUCE on Pi, full GUI on desktop | Avoids maintaining two native GUIs. Keeps Pi resources focused on audio. Desktop app is dev/sound design tool only. | Single unified GUI |
| 005 | Pre-Ledger | 25mm MDF for striking surface | Acoustically dead at musical frequencies — gives software full control. Wood veneer for aesthetics without compromising acoustic properties. | Wood, metal, acrylic |
| 006 | Pre-Ledger | Rubber grommet isolation at all mounting points | No rigid contact between segments or between segments and frame. Core constraint for four independent input channels. | Direct bolting with damping pads |
| 007 | 2026-03 | Convolution silence was a 10-second cap, not a JUCE bug | Removing the maxSamples cap resolved the issue. JUCE handles 28s IRs correctly. Re-prepare-after-load pattern is necessary and sufficient. | FFTConvolver replacement (no longer needed) |
| 008 | Pre-Ledger | Manual dry/wet mixing instead of JUCE DryWetMixer | JUCE DryWetMixer did not work correctly (details not investigated). Manual mixing works. | Debugging DryWetMixer root cause |
| 009 | Pre-Ledger | 11-byte binary strike descriptor protocol | Fixed packet size: sync byte, pad ID, velocity, attack slope, spectral centroid, HF energy ratio, decay shape, timestamp, XOR checksum. Compact and deterministic. | MIDI, OSC, variable-length protocol |
| 010 | Pre-Ledger | TL074 quad op-amp for signal conditioning | Single IC handles all four channels. Runs on Teensy's 3.3V. Total cost ~5-10 GBP. | Dedicated audio ADC, Teensy direct input |
