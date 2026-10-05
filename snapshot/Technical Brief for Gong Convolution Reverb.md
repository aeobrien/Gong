# Technical Brief: Gong Convolution Reverb

# Resonator System

## Project Overview and Goals

This project aims to build a **standalone JUCE-based audio application** that simulates the resonant sound
of a gong through convolution reverb and modal resonators. The system will allow **MIDI triggers** (note
events) to excite resonators (simulating gong-like modes) with short audio impulses, and can also accept
**live audio input** to excite those resonators. The resonators’ output is then processed by a **convolution
reverb** using an impulse response (IR) – potentially an IR recorded from an actual gong or acoustic space –
to produce a rich, realistic gong resonance. The design should be modular, allowing multiple resonators,
insertion of EQ/filter stages, and the ability to start/stop or reconfigure components easily. Ultimately, the
app should run on macOS (during development in Xcode) and later deploy to an embedded platform (mini
PC or Raspberry Pi) for standalone operation.

**Key requirements and components:**

```
Convolution Reverb Engine: Efficient real-time convolution of audio with an IR (the “gong” or reverb
IR). This is a critical component for authentic sound.
Modal Resonators: One or more resonant filters or resonator modules that simulate the vibrational
modes of a gong or similar instruments. They should be excited by impulses (from MIDI or audio
triggers) and produce decaying tones.
Excitation Signal (Impulse): A short audio burst generated on each MIDI note-on (or derived from
audio input) to excite the resonators. This could be a single-sample impulse, a short noise burst, or a
small recorded attack sample.
Signal Chain Architecture: A flexible audio routing that connects the impulse -> resonators ->
convolution reverb, with possible EQ or additional processing at various points. We need a clear
architecture so we can expand (e.g. add resonators or filters) and control/bypass elements as
needed.
Performance and Portability: The solution must be efficient in real-time (low CPU usage and low
latency) especially if it will run on a Raspberry Pi. We should leverage existing optimized DSP libraries
and JUCE components instead of reinventing them, as long as they can integrate into a JUCE project.
Polyphony and Control: Support multiple simultaneous resonator excitations (polyphonic play via
MIDI) and possibly multiple resonator modules in parallel. MIDI notes should map to appropriate
resonator frequencies (e.g. scaling the base frequencies for different pitches). There should also be a
way to damp or stop the sound (e.g. an “all notes off” or panic to quickly silence ringing resonators
and reverb).
```
In summary, the goal is to outline how to build this _“gong resonator reverb”_ app by reusing proven code for
convolution and resonators, detailing the architecture and best practices. Below we present the **system
architecture** followed by multiple implementation options for each major component, along with
references to existing projects and libraries that can be leveraged.

### • • • • • •


## System Architecture and Signal Flow

**Signal Flow Overview:**

```
MIDI Input / Audio Input: The application will accept MIDI note events and optionally an audio
input stream.
MIDI Note-On events will trigger the creation of an excitation signal (an impulse) with an amplitude
scaled by velocity. The note’s pitch will determine resonator tuning (so that different MIDI notes can
excite different pitched resonances).
Audio Input (if present) can be another way to excite the resonators – for example, the live audio’s
transients could pass directly into the resonator section (or through a detector that triggers
impulses). In “audio mode,” the resonators act as an effect, imparting their resonance onto the
incoming audio.
Exciter (Impulse Generator): This module generates a short broadband signal to excite the
resonators. The simplest form is a one-sample impulse (a single sample of value 1.0 and zeros after)
which provides a flat spectrum to excite all frequencies. Alternatively, a short noise burst or a
filtered click can be used to model different mallet strikes (hardness/softness). For instance, a very
hard mallet might be closer to a sharp impulse (more high frequencies), while a softer mallet might
use a noise burst low-passed to excite more low-frequency modes. The design could allow selecting
different excitation types, but initially a basic impulse is sufficient.
Modal Resonators: The core of the system – this section simulates the gong’s resonant body. In
practice, it can be implemented as one or more parallel resonant filters or resonator modules
that ring out when fed the excitation. Each resonator module may consist of multiple internal
modes:
In a simple approach, each resonator could be a bank of band-pass filters tuned to specific modal
frequencies (with appropriate decay factors). The output of all filters is summed to produce the
resonator’s output.
Alternatively, more advanced resonator algorithms (like physical modeling) can be used (discussed in
detail in the Resonator Implementation section below).
If multiple distinct resonators are used (for example, simulating two gongs or two sets of modes),
their outputs can be mixed or processed separately as needed.
Polyphonic MIDI : If multiple MIDI notes are played, we can either (a) instantiate multiple resonator
voices (one per note) using JUCE’s voice allocation (similar to a synthesizer), or (b) use a single
resonator module that internally supports polyphony. A simple way is to treat each MIDI note as a
separate voice with its own resonator instance, summing all voices before convolution.
EQ/Filters (optional at various points): To shape the tone, EQ or damping filters might be inserted:
Pre-Resonator EQ : shaping the excitation signal spectrum (e.g. a high-frequency rolloff for “softer”
strikes).
Post-Resonator EQ : e.g. a tone control on the resonator output to tame harsh overtones or
emphasize certain frequencies. This could be as simple as a low-pass filter or a more complex
parametric EQ. (We can utilize JUCE’s DSP modules like dsp::IIR::Filter or biquad filters for
this, or even include an existing EQ code if needed.)
Post-Reverb EQ : a final shaping of the convolved output (common in reverb units to EQ the wet
signal). These EQ stages are not strictly required for functionality but provide flexibility in tweaking
the sound. We should design the architecture to allow inserting or bypassing these filters easily.
Convolution Reverb: This takes the summed output of the resonators and convolves it with an
Impulse Response (IR). The IR could be a recording of a gong’s natural resonance or any acoustic
```
### 1.

### 2.

### 3.

### 4.

### 5.

### 6.

### 7.

### 8.

### 9.

### 10.

### 11.

### 12.

### 13.

### 14.


```
space we want to simulate. In our context, using a gong’s recorded impulse might further imprint
realistic gong characteristics onto the sound. The convolution reverb produces the final audio output
(wet signal), possibly mixed with some dry component (though likely we will use mostly the wet
output for a fully “resonated” sound).
The convolution should operate in real-time with low latency. Typically this is done with partitioned
FFT convolution to be efficient for long IRs (like a 2-5 second gong ring-out).
We must choose an implementation that is CPU-efficient , especially if deploying to Raspberry Pi.
Using a highly optimized convolution library or JUCE’s built-in partitioned convolution is crucial
(discussed below).
Audio Output: Finally, the convolved output is sent to the audio output device. If needed, a “stop”
or “panic” mechanism can fade out or reset the resonators and convolution (for example, by
clearing convolution buffers and resetting filter states) to immediately silence the system when
requested.
```
**Modularity & Control:** Each of these components (Impulse generator, Resonator(s), EQ, Convolution)
should be encapsulated so they can be individually enabled/disabled or adjusted. For example, during
development we might first get the convolution reverb working standalone (with perhaps a simple test
input) – since the user indicated _“the convolution reverb aspect is the bare minimum to get up and running.”_
Then we add the resonator excitation chain in front of it. Ensuring a modular design (possibly using a JUCE
AudioProcessorGraph or simply a well-structured processing block in code) will make it easier to insert
more resonators or filters later.

Below, we break down the **implementation options** for the two key technical sections: the convolution
engine and the resonator implementation. Each subsection references existing libraries or code that can be
leveraged, to avoid starting from scratch.

## Convolution Reverb: Implementation Options

A convolution reverb performs a convolution of the input signal with an IR (impulse response). In our app,
the input to the convolver is the sum of all resonator outputs (the “dry” resonator sound), and the IR could
be the gong’s recorded impulse or any reverb we choose. The result is a realistic reverberation or resonance
effect. The challenge is doing this efficiently in real-time. Here are options:

```
Option 1: JUCE’s built-in dsp::Convolution class – JUCE provides a convolution engine that
supports partitioned convolution with zero latency by default. It can handle stereo IRs and
even dynamically load new IRs in a thread-safe manner. Using this is straightforward in a JUCE
project:
Pros: Easy integration (part of JUCE), thread-safe IR loading, no extra dependencies. Can specify non-
uniform partition sizes or a fixed latency to reduce CPU if needed.
Cons: It may be less optimized than specialized libraries. In a known test, the JUCE stock convolution
consumed around 20% CPU for a large reverb IR (in a debug build). On a Raspberry Pi, this could
be significant, so performance is a consideration.
Best practice with JUCE Convolution: You can initialize with Convolution::NonUniform{256}
for example to use a two-part partitioning (head partition of 256 samples) which is recommended for
long IRs to save CPU. Also, prepare it with the sample rate and block size via
convolution.prepare(spec) and then load the IR (from WAV file or memory) using
loadImpulseResponse (JUCE can load from a file path or memory audio data).
```
### 15.

### 16.

### 17.

### •

```
1 2
```
### •

```
3 2
```
-
    4

### •

```
3
```

```
If initial simplicity is priority, you might start with JUCE’s dsp::Convolution for development,
then optimize later if needed.
```
```
Option 2: Use an optimized FFT convolution library (KlangFalter / FFTConvolver) – An excellent
open-source solution is the FFTConvolver library by HiFi-LoFi. This is the convolution engine
extracted from the open-source plugin KlangFalter. It uses partitioned FFT convolution (with uniform
partition sizes by default) and is highly optimized in C++ (with optional SSE and support for non-
uniform partition via a “two-stage” convolver). This library is MIT-licensed and light-weight (no
big dependencies; it even includes its own FFT implementation).
```
```
Pros: High performance – as reported by the developer of REEV-R (another convolution plugin),
using the KlangFalter FFT convolution library reduced CPU usage dramatically (20% down to <1% in
their case, compared to JUCE’s built-in convolution). This makes it very suitable for low-power
hardware. It’s also real-time safe and has no added latency (if using zero-latency configuration).
Cons: Slightly more integration work: you’ll need to include the library’s source in your project (a
few .h/.cpp files) and manage the convolution process manually (calling its process function each
audio block).
Integration: FFTConvolver provides classes like FFTConvolver (for uniform partition) and
TwoStageFFTConvolver (for non-uniform). You initialize it with setup() providing the IR data
and desired block size. Then each audio callback, call process() with your input buffer to get the
convolved output. The library supports any length IR (internally chooses partition sizes).
Example usage is shown in REEV-R plugin: REEV-R credits using the KlangFalter FFT convolution and
notes that JUCE’s stock convolution was much less efficient. The library itself is on GitHub for
reference.
```
```
Given we aim for eventual Raspberry Pi deployment, this optimized approach is highly
recommended once basic functionality is verified.
```
```
Option 3: Other convolution libraries or approaches – A few other possibilities:
```
```
The IRS toolkit or Libsndfile + FFT : We could manually perform convolution by taking an FFT of the
IR and doing block-by-block FFT multiply (this is essentially what the above libraries do). However,
writing this from scratch isn’t necessary given the above options.
Hardware/Accelerated solutions: On Raspberry Pi (ARM), NEON optimizations can accelerate FFT.
FFTConvolver uses plain C++ (and SSE for x86); it might not have NEON by default. If needed, one
could integrate an FFT library like KissFFT or Apple’s vDSP (for macOS) to help, but again,
FFTConvolver already comes with an FFT (and one could extend it with NEON optimizations if
absolutely necessary).
GPU convolution is overkill here and not easily supported on JUCE without custom code; likely not
needed.
```
**Recommendation:** _Start with JUCE’s dsp::Convolution for simplicity (quick to get working) and then
consider switching to FFTConvolver for efficiency, especially for deployment._ The coding LLM can refer to the
FFTConvolver repo for implementation details. The REEV-R project’s README explicitly praises the
performance gains of the KlangFalter/FFTConvolver approach , indicating it “was the only library [the
author] found that perfectly fits this plugin.”

### • • 5 6 • 4 • • •

```
4 5
```
### • • • • • 5 4


Additionally, _KlangFalter_ itself is an open-source convolution plugin (GPL-licensed) whose code could be
referenced. _REEV-R_ (by Tiago) is another open-source convolution reverb that builds on KlangFalter’s
engine and adds IR loading, EQ, etc. These can serve as code references for how to integrate
convolution in a JUCE plugin context: - KlangFalter GitHub: **HiFi-LoFi/KlangFalter** – convolution plugin using
JUCE (look for how it initializes the convolution engine). - REEV-R GitHub: **tiagolr/reevr** – shows usage of
the FFTConvolver library in a JUCE project, plus how they implemented IR file management and parametric
EQ on the IR.

Finally, regarding **IR management** : We should allow loading a custom IR (especially on desktop, for testing
different gong or reverb impulses). JUCE Convolution can load from file path easily. With FFTConvolver, you’d
need to load the WAV (e.g. via AudioFormatReader), get the IR samples, and pass them to FFTConvolver’s
setup. For a “gong” IR, if not provided, we might record one or use an available IR (OpenAIR library has
some instrument IRs ). For now, a placeholder (like a short burst or any reverb IR) can be used to test the
pipeline.

## Resonator Implementation: Options for Modal Resonators (Gong

## Simulation)

Creating the **resonator** component is perhaps the most complex part, as it involves simulating the physics
(or at least the spectral behavior) of a gong. A gong produces a rich, inharmonic spectrum of decaying
partials. We have a few strategies to implement this digitally, and importantly, there are existing projects
and libraries to draw from:

```
Option A: Parallel Resonant Filters (Manual Modal Synthesis) – Implement the gong as a sum of
resonant band-pass filters, each representing a vibrational mode. Each filter will ring at a certain
frequency with a certain decay time when struck by an impulse. Technically, a second-order band-
pass or band-stop filter can serve as a resonator for a single frequency.
For example, we can create N biquad filters with high Q (narrow bandwidth) tuned to frequencies f1,
f2, ..., fN (these would be derived from known gong partial frequencies). When an impulse passes
through, each filter will produce a decaying sinusoid at its frequency. The decay rate is controlled by
the filter’s resonance (pole radius). A radius slightly less than 1.0 (or an equivalently high Q factor)
yields a slow decay, whereas lower Q (more damping) decays faster.
We can draw on the modal synthesis technique : For a simple implementation, 4-8 modes might
already give a gong-like sound. There are known modal frequency ratios for instruments (see
Csound’s appendix for modal ratios – e.g., a small bowl might have modes at frequency
ratios 1 : 2.78 : 5.18 ... etc for inharmonic partials).
Implementation: JUCE’s dsp::IIR::Coefficients can create band-pass filters given a center
frequency and Q. We could also directly implement a two-pole filter equation. For consistent decay
regardless of sample rate, one might adjust filter gain or use a normalized loop. The JUCE forum has
discussions on biquad resonators (placing poles on the unit circle at desired frequencies with a
certain radius). Key insight: The biquad coefficients for a resonator at frequency ω0 with
decay can be set by poles = radius * e^(±i ω0). The radius (slightly <1) gives decay, and ω0 = 2π *
(freq/sampleRate). The feedforward (zeros) can be simple (even just [1,0,0] as in that forum post for
an all-pole resonator).
We would sum the output of all these filters. Optionally, each mode can have a weighting (some
modes are louder than others in a real gong). We might start equal and adjust by ear.
```
```
7
8 9
```
```
7
```
```
8 9
```
```
10
```
### •

### •

```
11 12
```
-

```
13 14
```
### •

```
11 12
```
```
15
```
-


```
Pros: Straightforward conceptually, easy to customize frequencies and decays. No external
dependency (just using JUCE or basic DSP).
Cons: Fine-tuning is required to get a realistic sound. We need to predefine mode frequencies and
decays (maybe based on literature or analysis of a real gong). Also, if we want to support different
pitches via MIDI, we’d need to scale these frequencies per note, which for inharmonic partials isn’t
exact (scaling a set of inharmonic frequencies by a constant factor will preserve inharmonicity, which
is actually okay as it just sounds like a scaled version of the same gong).
Multiple Resonators vs. Polyphony: If we use this approach, one “resonator” could be defined as a
bank of filters for one note. For polyphony, either instantiate multiple banks (one per active MIDI
note) or dynamically retune one bank per note (complicated). It’s easier to have independent voices.
JUCE’s Synthesiser class could manage voices, each voice containing a filter bank that is
triggered on note-on and rings out. We must ensure performance is okay if, say, 4 notes
simultaneously => 4 * N filters active.
```
```
Reference: Perry Cook’s research on modal synthesis is relevant. In fact, Csound and STK provide
opcodes and classes (streson in Csound, Modal in STK) to do exactly this by specifying modal
frequencies and decays. We can potentially use these resources to obtain frequency ratios of a
gong-like instrument (for example, the tibetan bowl ratios in the list might serve as a starting
point for a gong’s inharmonic spectrum).
```
```
Option B: Use the Synthesis ToolKit (STK)** physical modeling classes – STK is a well-known C++
library of physical models by Cook and Scavone. It includes classes for modal synthesis and other
resonators. Notably:
```
```
ModalBar – an STK class that implements a resonant bar (or plate) with multiple modes. It has
presets for instruments like marimba, vibraphone, etc., and allows setting modal frequencies for
custom sounds. While a gong isn’t a preset, we could potentially supply a custom set of modal
frequencies. STK’s ModalBar inherits from a Modal class which essentially manages multiple
resonant modes.
BandedWG – an STK class for banded waveguide modeling, useful for inharmonic percussion (like
plates, perhaps gongs).
Resonate – a simpler STK class which is just a two-pole resonator filter (it can be used to filter an
input, similar to Option A’s individual filter approach).
Using STK in a JUCE project would mean either adding STK as a static library or copying the needed
class code. STK is open-source (mostly MIT-like license). It might be overkill to include the whole
library, but extracting ModalBar or Modal classes is possible.
Pros: Leverages well-tested code for resonances. ModalBar already handles multiple modes and
their decays, and it has methods to strike (excite) and damp.
Cons: Integration effort (resolving dependencies, e.g. STK may use its own sample rate global or
require initialization). Also, STK’s sound may not be exactly a gong without tuning; we’d need to
tweak modal frequencies.
Reference: The STK documentation and headers (e.g., ModalBar.h describes it as “resonant bar
instrument” ). While ModalBar is about bars, a gong is more like a 2D plate – however, ModalBar
could possibly approximate if given many modes. STK’s Mesh2D class simulates a 2D membrane
(which could model a thin plate/gong), but that is a more computational finite-difference approach.
```
```
If using STK, an example approach is to use one instance per note (STK has noteOn/noteOff
functions). We can compile STK’s core with JUCE (ensuring the real-time thread safe usage).
```
### •

### •

### •

### •

```
16
14
```
### •

### •^17

```
18
```
- • • • • •

```
17
```
### •


```
Option C: Utilize Mutable Instruments Rings/Elements code – Mutable Instruments’ Rings module
is essentially a modal resonator that can simulate strings, tubes, and plates. In particular,
Rings’ modal resonator model (borrowed from their Elements synth) uses a bank of 60 band-pass
filters** to simulate resonant modes. This is a sophisticated implementation that handles
inharmonicity (“Structure” parameter), brightness (decay of higher modes), and has an excitation
input (the “Strum” trigger for impulses).
```
```
Pros: High-quality, proven sound – Rings is famous for realistic plucked and percussive resonances.
Its code (for the modal resonator) is MIT licensed , and could potentially be integrated. It
already supports polyphony (internally can allocate up to 4 voices by splitting the filter bank).
It also has nice features like position parameter (excitation position affecting mode amplitudes).
Cons: The code is non-trivial to integrate – it was written for an STM32 microcontroller environment.
We’d have to adapt it to our project (replace its audio IO with our own, remove hardware-specific
parts). Also, 60 filters per resonator might be somewhat heavy CPU (though Rings ran on 168 MHz
STM32 in C++ without SIMD, so a Raspberry Pi 1.5GHz should handle it easily). If we only need the
modal resonator part, we can strip out the other models (Rings has other models like sympathetic
strings, which we might not need unless we want that effect).
How Rings works: In code, Rings has a Resonator class with an array of filters f_[i]. It sets
them up in ComputeFilters() by calculating each mode’s frequency and Q based on a base
frequency and a stiffness (inharmonicity) parameter. During audio processing, it feeds the
input impulse to all filters each sample and sums the outputs, splitting odd and even modes to two
outputs (we can just sum them for mono). Notably, it adjusts the amplitude of each mode
according to an excitation position using a cosine distribution (this is a physical modeling detail to
mimic how striking at different points excites modes differently).
This level of detail might be beyond what we initially need, but it’s an excellent reference or even
source to port. We could, for example, use a simplified version: perhaps use fewer modes or fix
some parameters. But given it’s open source, one approach is to literally use Rings’ resonator code
for the core DSP, and wrap it in a JUCE-friendly class. Many have ported MI code into plugins (the
license allows it with attribution).
If we adopt Rings code, we’d feed it our impulse on trigger (Rings had an internal noise burst
generator for its exciter, which we could bypass by providing our own impulse).
```
```
References: Mutable Instruments documentation explains Rings’ resonator models. The
code on GitHub (pichenettes/eurorack) under rings/dsp is where the implementation lives (we’ve
captured a snippet above showing how the filters are computed and processed).
```
```
Option D: Other physical modeling libraries or projects – For completeness:
```
```
Faust Physical Models : The FAUST DSP language has a physical modeling library (including a modal
bar, etc.), which could be used to generate a C++ module. However, this might be unnecessary given
the above options.
Soundpipe / Csound : There are small DSP libraries like Soundpipe (in C) that have some physical
models derived from STK. Csound opcodes (like modalfreq lists or reson filters) could inspire
parameter choices.
Example projects : The FX-Mechanics plugin collection includes a waveguide synth (StrinGO by
Olivier Doaré) , which is more for strings, but it indicates interest in these techniques. Another
```
### •

```
19 20
```
### •

```
21 22
23 24
```
### •

### •^25

```
26 27
```
```
28 29
```
```
30
```
-

### •

### •^1920

```
26 28
```
### •

### •

### •

### •

```
31
```

```
project, Resonarium (open-source waveguide/resonator synth) was mentioned in search results –
possibly could have relevant approaches.
```
**Chosen approach:** It might be wise to start simpler (Option A: a few resonant filters to prove the concept,
since that’s easier to implement from scratch or via JUCE). This will demonstrate the MIDI->impulse-
>resonator->reverb chain quickly. Once that works, we can refine the resonator: - Possibly upgrade to a
larger modal bank (more filters) for richer sound. - Or integrate one of the advanced codes (STK or Rings)
for realism. - The user specifically mentioned “gong,” so focusing on inharmonic partials is key – a large
gong has a complex spectrum, so ultimately using an advanced model like Rings’ modal resonator (which
can produce inharmonic spectra by adjusting stiffness parameter) might give the best result. _For now, ensure
the architecture allows swapping out the resonator implementation easily._ For example, define a
ResonatorVoice interface that we can implement in different ways (simple biquad bank vs. Rings code).
The rest of the system (MIDI handling, convolution) can remain the same.

**Tuning and MIDI mapping:** - We likely want the resonator’s base frequency to follow the MIDI note (so you
can “play” pitches to some extent). For a given MIDI note number n , convert to frequency f = 440 *
2^((n-69)/12) (A4=69->440Hz, standard tuning). Then feed that as the fundamental frequency into the
resonator model. In Rings, this would be the “frequency” parameter (it internally populates modes up to
Nyquist). In a custom filter bank, we multiply our modal ratio list by that fundamental. Note: Because
gong partials are inharmonic, playing a musical scale may sound dissonant, but at least higher MIDI note =
higher overall pitch of the gong resonance (which is likely desired). - If needed, we might quantize or limit
the range (real gongs might not follow equal temperament, but as a creative instrument it’s okay). - **MIDI
Velocity** can scale the amplitude of the impulse (and perhaps brightness: e.g., higher velocity could use a
“harder” excitation – maybe mix in a higher-frequency content). But at minimum, velocity -> volume is
straightforward. - **Stopping/damping:** In a real physical model, a note-off could apply damping (e.g., in STK
ModalBar you can call noteOff to damp). If we implement manually, we can simulate damping by zeroing
the filter states or injecting a short inverse impulse. Simpler: on note-off, we could just reduce that voice’s
output gain or set a flag to stop processing it further (though if ringing, you have to actively silence it).
Another approach is to implement a global “damp” pedal: if a certain MIDI CC or command is received,
apply heavy damping (e.g., set all resonators’ pole radius much lower so they decay immediately). - If voices
naturally decay, maybe we don’t need an explicit note-off for sustained gongs (they ring out fully). But
having a way to stop sound (especially if the convolution tail is long) is useful. We can achieve an immediate
stop by calling reset() on the JUCE Convolution (clears IR buffer) and resetting resonator states.
However, that might cause a click; a gentler way is to fade out quickly (like over 50 ms) by multiplying output
by a descending gain if a stop is requested.

In summary, the resonator can initially be a simple parallel filter bank. As a concrete minimal example: -
Suppose we start with 3 modes: frequencies [1x, 2.7x, 5.8x] of a base frequency (based on a bowl/gong
ratio). At MIDI A4 (440 Hz base), those would be 440, ~1188, ~2552 Hz. We set Q such that each rings for
perhaps 2 seconds (this might be trial-and-error, e.g. Q=100 or pole radius 0.999). - Generate an impulse on
note-on, feed these filters, sum output -> convolution reverb (with a gong IR or even a generic hall to start).
We should hear a metallic ringing with a reverb tail. - Then we refine by adding more modes, adjusting
decay, etc., or swapping in a more advanced model.

**References for resonator code & research:** - JUCE Forum “Stabilize Volume of BP Filters for Modal
Synthesis” might have tips on maintaining consistent output when tuning filters (ensuring no huge gain
spikes). - Csound’s streson and mode opcodes as mentioned, which use lists of frequencies/decays

```
32
```
```
33
```
```
34
```
```
14
```
```
35
```

(the Appendix E table provides example ratios). - STK’s Modal class code or the documentation (part of
STK) for how they implement different presets (they likely have fixed ratios for things like a ‘Chinese gong’
preset in some form; if not, at least a structure to load modal data). - Mutable Instruments’ Elements had a
resonator + exciter + reverb in one instrument (interestingly, exactly our chain: “Elements = Exciter +
Resonator + Reverb” ). Rings is the standalone resonator from Elements. So essentially, our project is
conceptually similar to **Elements** (if we combine our impulse exciter, resonator, and convolution reverb). It’s
encouraging that such a combination is known to work well. We might glean that a bit of reverb is actually
part of the instrument’s character (Elements had an integrated reverb in the module’s output). In our case,
we explicitly design the convolution reverb for that.

## MIDI and Audio Trigger Handling

This section covers how we translate inputs into the excitations properly and manage voices:

```
MIDI handling: We will likely use JUCE’s AudioAppComponent or AudioProcessor framework. If
using an AudioProcessor (like making this as a plugin initially or a standalone via the Standalone
Plugin wrapper), JUCE provides MIDI message handling each block. We can parse the MIDI buffer for
note on/off.
On Note On : Determine which resonator voice to use (if using Synthesiser, it does that for us; or
manage a small array of voices manually). Trigger the voice: this could mean resetting its internal
state (so it starts fresh) and giving it the new frequency and amplitude. For a filter bank approach,
that means recalculating filter coefficients for the target frequency (if each voice has its own bank).
On Note Off : For a gong-like instrument, we might ignore note-offs to let the sound decay naturally
(unless we explicitly want a dampening). Possibly treat a MIDI All Notes Off or a sustain pedal
message as a signal to damp everything.
Polyphony : If we go with JUCE’s Synthesiser class, we’ll create a custom SynthesiserVoice
that contains our resonator (and maybe even the convolution, though convolution is better done
globally as an effect). Usually, reverb is not per-voice but a global send effect. So we might
implement voices that output to a mix buffer which then goes through convolution at the end.
```
```
Another approach is to have a single resonator that handles polyphony internally (like Rings does
with up to 4 note polyphony). That is more complicated to manage but more efficient than spinning
up separate 60-filter banks. Rings does voice allocation internally (each new note triggers allocation
of modes to either all in one, or splits modes between voices) , but replicating that might be
complex. Using multiple simpler voices may be easier for the coding LLM to implement.
```
```
Audio input mode: If audio input is used, two possibilities:
```
```
Direct convolution: We could simply convolve the live audio with a gong IR, but that wouldn’t
involve resonators – it would be just an IR reverb effect.
Through resonators: Feed the input into the resonator filters, then into convolution. This would
make the input sound take on the gong’s resonant characteristics. This is effectively a resonator
effect as seen in Rings (Rings has an audio input jack to ring its filter bank with external sounds ).
We should implement a mode where if audio input is present (and maybe a certain toggle is on), the
resonator input comes from the live audio (possibly plus an impulse when a transient is detected).
```
```
13
```
```
36 37
```
### •

### •

### •

### •

### •

```
38
```
### •

### •

### •

```
39
```

```
Simpler: always allow audio input to mix in. For example, if audio input is not silent, feed it through
the resonators continuously (the filters will resonate particular frequencies present in the input). This
can create interesting resonant filter effects.
Alternatively, detect transients or onsets in audio and use those as trigger points (which is more
advanced).
```
```
We likely will focus on MIDI triggering first (since that’s a clearer control scheme) and treat audio
input handling as an extension.
```
```
Threading considerations: Both MIDI and audio input will be handled in the audio thread (MIDI via
the buffer, audio as actual samples). All DSP (resonators, convolution) should execute in the audio
callback with no blocking calls. That’s why using partitioned convolution and pre-computed filter
coeffs is necessary. If we need to load a new IR or change something heavy, use background threads
(JUCE Convolution does this via its background message queue for IR loading ).
```
```
Versatility and Stoppability: To reiterate, design the code such that:
```
```
You can add or remove resonators easily (e.g. have a list or vector of resonator objects that the
audio loop iterates over, summing their outputs).
You can bypass the convolution (for debugging or if one wants just the dry resonator sound).
Implement a “panic” function that mutes everything. This could set a flag that causes all voices to
immediately stop producing sound (e.g., by clearing their internal states or outputting zeros). Also
possibly flush the convolution buffers (though with partitioned convolution, a flush might cause a
pop – better is to ramp down volume).
Because this is an app, we could even allow reconfiguration on the fly: e.g. user can load a different
IR or select a different resonator mode. Ensuring the architecture separates the concerns (e.g.,
convolution class vs resonator class) will help with this.
```
## Additional Processing (EQ, Filters, Effects)

The user specifically mentioned adding **additional EQs at various places or multiple resonators** during
tweaking. We have touched on where EQ can fit in the chain: - After resonators, before convolution: to
shape tonal balance going into the convolution. This could be critical if the convolution IR is a gong IR – real
gong IRs might emphasize certain frequencies, and adjusting the input spectrum could avoid over-
emphasizing those. - Within resonator: if we build a large resonator from many modes, we might _implicitly_
shape brightness through the mode amplitudes. Rings, for instance, has a “Brightness” control that
dampens higher modes more. We can emulate that by an EQ or by scaling the gains of high-
frequency filters. - Post-convolution: a simple high-cut might remove unpleasant digital harshness or a low-
shelf to tame rumble, etc.

We should leverage **existing filters** for EQ. JUCE’s dsp::ProcessorChain could string together filters
easily. Or the **DSPFilters** library by Vinnie Falco (if we need more advanced filter designs) – though probably
not needed as JUCE covers basics. There’s also the example of _REEV-R_ which implemented **parametric EQs
for the IR** (meaning they let the user EQ the impulse response before convolution – an advanced
feature to alter frequency-dependent reverb time). We might not need that complexity, but it’s good to note
how others do it.

### •

### •

### •

### •

```
40 2
```
### •

### •

### •

### •

### •

```
41 42
```
```
43
```

If multiple resonators are desired (beyond polyphony), it could mean: - Running two different resonator
modules in parallel and mixing them. For example, one could simulate a small gong, another a large gong,
and both are excited together to get a complex result. This is as simple as instantiating two resonator banks
and sending the same impulse to both. - Or having resonators with different audio inputs (less likely). - Our
design can accommodate that by having an array of resonators; MIDI note events could either trigger all
(for layering) or specific ones (if we had different mappings).

## Platform Considerations and Best Practices

Finally, some notes on developing with JUCE on Mac and deploying to Raspberry Pi or a mini PC:

```
JUCE Project Structure: It’s advisable to separate the DSP code from UI code. We may not need a
heavy GUI, but a basic interface with controls for parameters (like fundamental frequency offset,
decay, reverb mix, etc.) and file loading for IR can be useful for testing. On Raspberry Pi, if running
headless, the app could run as a console or background process with maybe MIDI control only.
Building for Pi: JUCE supports Linux builds. We might compile the app using Makefile or CMake
(Projucer can export a Linux Makefile). Ensure to link any libraries properly (if using FFTConvolver,
just include source). Because Pi is ARM, any x86-specific code (SSE) should be disabled;
FFTConvolver’s SSE can be turned off (and possibly NEON on if we add it).
Performance:
Use release builds on Pi for much better performance (the reevr author noted 20% in debug vs 1%
in release with optimized convolution ).
Take advantage of JUCE’s dsp::ProcessSpec to initialize filters and convolution with the appropriate
maximum block size (this helps internal buffer allocations).
Avoid memory allocation in the audio thread. Pre-allocate everything (JUCE convolution does
background alloc, FFTConvolver expects you to allocate once in setup).
Multithreading: Convolution can benefit from multi-threading if partition sizes are large, but since
we plan on partitioning, it might be fine single-threaded. If we do need, we could run convolution in
a separate thread that processes with one block delay (complex to ensure sync), but likely
unnecessary if optimized.
On multi-core (Pi4 has 4 cores), the simplest is to let the OS distribute different voices on core if
possible (but audio callback typically runs single-threaded). We could use the AudioProcessorGraph
to split tasks but that might be overkill. Probably not needed if CPU usage is under control.
Memory: Convolution IR will take memory for FFT buffers. A 2-second stereo IR at 48kHz is ~2*
samples, which after partitioning might consume a few MB – fine on Pi. The resonator filters are
negligible memory. So no big issues there.
Best Practices for Code Design:
Modularity : Write classes for each major part. For example: ConvolverEngine (wraps either JUCE
Convolution or FFTConvolver behind a common interface so we can switch easily),
ResonatorVoice (handles one note’s resonation), ResonatorsManager or Synthesiser (to
manage voices), and maybe an ImpulseGenerator (though the impulse is so simple it could be
inline code).
Avoid blocking I/O : If reading IR from SD card on Pi, do it at startup or in a separate thread, not in
audio processing.
Testing and debugging : It might help to implement a bypass for convolution (to hear the raw
resonator output) or even a mode to bypass resonators (to test convolution alone using a test input).
This can isolate issues.
```
### • • • • 4 • • • • • • • • •


```
Leveraging existing code : Don’t hesitate to copy proven code for filters or transforms. For example,
if implementing the biquad filter, one could use JUCE’s IIR (which is already optimized and can be
configured as band-pass) or copy equations from trusted sources (RBJ filters etc.). If integrating
Rings code, use the original code as much as possible to avoid introducing errors – just adapt the
interface.
```
Finally, we provide a quick summary of **existing resources** that can be referenced for implementation: -
**KlangFalter Convolution Plugin (JUCE-based)** – Example of convolution reverb plugin (GPL). -
**FFTConvolver library (MIT)** – Standalone efficient convolution engine. - **REEV-R (Convolution Reverb
with modulations)** – Open source plugin using FFTConvolver; check its libs/ and src/ for how it sets
up convolution and IR EQ. - **STK (Synthesis ToolKit)** – for modal synthesis classes like ModalBar, etc.
(STK on GitHub). - **Mutable Instruments Rings** – MIT-licensed code for advanced resonator (see
rings/dsp/resonator.{h,cc} for core algorithm). - **JUCE DSP Module** – Documentation on
dsp::Convolution (we included some reference lines above) and dsp::IIR for filters. - **JUCE Forum
and Csound References** – for hints on implementing resonators and example modal frequencies (e.g.,
Csound modal frequency ratios list).

By combining these resources with careful system design, we can build the gong convolution resonator app
efficiently without reinventing wheels. The approach ensures we meet the **“bare minimum” (standalone
convolution reverb working) first** , then incrementally add the resonator complexity, following best
practices throughout.

**Sources:**

```
JUCE Convolution class supports partitioned convolution (zero-latency by default, with optional non-
uniform partitions for efficiency).
REEV-R plugin notes that using the KlangFalter FFT convolution library yielded significant CPU
improvements vs JUCE’s stock convolution (20% CPU down to <1%) , highlighting the performance
benefit of that library.
REEV-R features list confirms it uses KlangFalter for high-performance convolution, and includes IR
manipulation and parametric EQ features – useful references if we consider similar features.
Mutable Instruments Rings documentation explains it’s a resonator without exciter , using 60
band-pass filters for the modal resonator model (with polyphony support by splitting filters among
voices). The Rings code’s resonator algorithm shows how modes are computed and
processed (frequency, Q, etc.) , which we can use as a template for our resonator
implementation.
STK’s ModalBar class (and underlying Modal class) exemplify a modal synthesis approach where
multiple resonant modes are excited by a strike – this is essentially what we need for a gong
(though with different modal frequencies). This indicates the viability of using a bank of resonators
to simulate an instrument’s timbre.
```



juce_Convolution.h
https://github.com/sonosaurus/sonobus/blob/35f1062dab196b9838a4bb529c4bf6592b7f5987/JUCE/modules/juce_dsp/
frequency/juce_Convolution.h

GitHub - tiagolr/reevr: Convolution reverb with pre and post modulation
https://github.com/tiagolr/reevr

GitHub - HiFi-LoFi/FFTConvolver: Audio convolution algorithm in C++ for real time audio processing
https://github.com/HiFi-LoFi/FFTConvolver

HiFi-LoFi/KlangFalter: Convolution audio plugin (e.g. for ... - GitHub
https://github.com/HiFi-LoFi/KlangFalter

Biquad Resonator - General JUCE discussion - JUCE
https://forum.juce.com/t/biquad-resonator/

Appendix E. Modal Frequency Ratios
https://csound.com/docs/manual/MiscModalFreq.html

ModalBar.h
https://github.com/RTcmix/RTcmix/blob/4befac70d470babb2ada24a3ae6f19f2cd5ff8e4/insts/stk/stklib/ModalBar.h

Index - Mutable Instruments Documentation
https://pichenettes.github.io/mutable-instruments-documentation/modules/rings/

eurorack/rings/rings.cc at master · pichenettes/eurorack · GitHub
https://github.com/pichenettes/eurorack/blob/master/rings/rings.cc

resonator.cc
https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/rings/dsp/resonator.cc

GitHub - odoare/StrinGO: A waveguide synthesis plugin
https://github.com/odoare/StrinGO

Resonarium: Free Open Source Waveguide Synthesizer
https://polarity.me/posts/polarity-music/2025-06-08-resonarium-open-source-and-free-waveguide-synthesizer/
