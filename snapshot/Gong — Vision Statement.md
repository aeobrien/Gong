## Intent

The Digital Gong is a custom-built electronic instrument that reimagines the gong as a programmable, performable digital instrument. It uses physical striking surfaces embedded with contact microphones to capture impulses, which are processed through software-defined resonators running on a Raspberry Pi 5. The result is an instrument that preserves the physicality and gestural drama of playing a gong — the mallet, the strike, the visual spectacle — while making the sonic behaviour entirely controllable.

The instrument is designed to look gong-inspired but unmistakably new. It should be immediately apparent to anyone seeing it that this is a different kind of instrument — one that references the gong tradition without imitating it.

## Motivation

A physical gong's resonant behaviour is fixed by its manufacture — its size, alloy, profile, and how it was hammered. This limits what a performer can do in contexts like sound baths and ambient performance, where precise control over pitch, harmony, and decay would be musically valuable. The Digital Gong decouples the physical interaction from the sonic result, giving the performer control over resonator frequencies, voicing, detuning, decay characteristics, and how input maps to output.

The deeper motivation is rooted in a personal theory about why gongs work therapeutically: their inharmonic partials overwhelm the brain's pattern-matching systems in sensory-reduced environments, creating a productive disorientation that facilitates deep relaxation. A programmable gong allows deliberate exploration and exploitation of this effect — tuning to specific inharmonic relationships, or shifting to harmonic intervals, or moving between the two during a session.

There is also a practical dimension. A decent physical gong costs potentially thousands of pounds. The Digital Gong offers a more cost-effective and significantly more versatile alternative for someone building a sound bath practice.

## Audience

The primary audience is the builder — this is a personal instrument for use in the builder's own ambient sound bath events, which incorporate gongs alongside other musical and immersive sound elements. The instrument needs to satisfy one performer's workflow and aesthetic standards.

There is no current intent to produce this commercially or for other performers, though the design and documentation should be clear enough that the project could be replicated or adapted if that ever became interesting.

## Scope

**In scope:**
- The complete physical instrument: MDF core segments with wood veneer finish, mounted on a stand with full vibration isolation between segments.
- The embedded audio engine: Raspberry Pi 5 running the resonator, convolution, and effects software, accepting contact microphone input.
- The performance interface: a 5-inch circular Waveshare screen providing a dedicated performance UI for switching between impulse responses and resonator frequency presets during live use.
- The desktop development environment: the existing JUCE application, used for configuration, development, and sound design but not for live performance.
- All iterations from raw MDF prototype through to the final veneered instrument.

**Out of scope:**
- Sound bath event planning, marketing, or audience-facing materials — that is a separate project.
- Commercial production or manufacturing for others.
- Plugin formats (VST, AU, etc.) — this is a standalone instrument, not a DAW tool.
- Integration with other instruments or control surfaces beyond what the circular screen provides.

## Design Principles

**Function before form.** Get it working on raw MDF before investing in aesthetics. Every version should be playable and testable. Beauty comes last, but it does come.

**Physical honesty.** The instrument should look like what it is — a new digital instrument that references gong tradition. It should never look like it's trying to be a real gong and falling short.

**Buy over build where possible.** Use off-the-shelf solutions (stands, screens, Pi hardware) rather than custom fabrication, to reduce risk and keep the project moving. Custom work is reserved for the things that genuinely need to be custom.

**Acoustic isolation by design.** The four segments must be vibrationally independent so each contact mic captures only its own impulse. This is a core architectural constraint, not an optimisation — the instrument's expressiveness depends on having four genuinely separate input channels.

**The performer's instrument.** Design decisions should serve the experience of playing the instrument in a live setting. If something looks good on a workbench but doesn't work in performance, it's wrong.

## Definition of Done

The project is complete when all of the following are true:

- The instrument is a veneered, visually striking object that looks intentional and beautiful — something you'd confidently place as the centrepiece of a performance.
- It is mounted on a stand with full vibration isolation between all four segments.
- The Raspberry Pi 5 is running the audio engine reliably, processing contact microphone input through the resonator and effects chain with acceptable latency.
- The circular performance screen provides a working interface for switching between impulse responses and resonator frequency presets during live use.
- The instrument has been used in at least one live sound bath event with a real audience.
- Any issues discovered during that live use have been addressed.

## Mental Model

The Digital Gong is a resonance decoupler. A physical gong is a single object where the striking surface and the resonating body are the same thing — you hit it, and physics decides what happens. The Digital Gong splits that into two independent systems: a physical surface that captures the gesture, and a software engine that decides the sound. The connection between them is programmable, which means the performer's physical vocabulary (where you strike, how hard, which segment) maps onto a sonic vocabulary that can be completely reconfigured between — or during — performances.

## Ethical Considerations

The instrument draws on the tradition and cultural significance of gongs, which have deep roots in multiple Asian cultures and spiritual practices. The design should reference this lineage respectfully without appropriating specific cultural forms or sacred imagery. The physical design principle of "not a replica" serves this goal — the instrument is explicitly a new thing inspired by the gong, not an imitation of a traditional one.