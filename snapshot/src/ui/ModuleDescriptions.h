#pragma once

#include <JuceHeader.h>
#include <unordered_map>

/**
 * Plain-language descriptions for every module and parameter.
 * Written for someone with NO knowledge of gong physics or synthesis.
 * Every description explains: what the module does, how it connects to other modules,
 * and what changing its parameters will do to the sound you hear.
 */
namespace ModuleDescriptions
{

struct ModuleInfo {
    const char* title;
    const char* subtitle;
    const char* description;
    juce::Colour colour;
};

inline const ModuleInfo& getInputModule() {
    static ModuleInfo info {
        "Input / Strike Detection",
        "Listens for when you hit the gong and measures how hard",
        "This is where sound enters the system. A microphone or line input picks up audio, "
        "and this module watches for sudden loud moments (strikes). When it detects one, it "
        "sends a signal to the Energy Accumulator saying 'a strike just happened, this hard.'\n\n"
        "In Synthetic mode, instead of a real microphone, you use MIDI to trigger virtual strikes. "
        "The Attack, Bright, HF Mix, and Tail knobs shape what that virtual strike sounds like - "
        "from a soft mallet thud to a sharp metallic click.\n\n"
        "SIGNAL FLOW: Input -> Energy Accumulator (strike energy) + Resonator Bank (audio excitation)",
        juce::Colour(0xff4caf50) // Green
    };
    return info;
}

inline const ModuleInfo& getSyntheticModule() {
    static ModuleInfo info {
        "Synthetic Impulse",
        "MIDI-driven shaped noise bursts replace the microphone",
        "Instead of a real microphone, MIDI note-on messages generate synthetic strike impulses. "
        "Velocity controls strike strength. Use the Audition button to test without a MIDI controller.",
        juce::Colour(0xff8bc34a) // Lime
    };
    return info;
}

inline const ModuleInfo& getEnergyModule() {
    static ModuleInfo info {
        "Energy Accumulator",
        "Tracks the gong's total vibration energy - this drives everything",
        "Think of this as the gong's 'excitement level.' Every strike adds energy. "
        "Energy naturally decays over time (like a gong ringing out). While energy is high, "
        "the gong sounds brighter, wider, and more alive.\n\n"
        "HOW IT WORKS: The accumulator tracks energy in 4 separate frequency bands (low, mid-low, "
        "mid-high, high). Each band feeds its own resonator. The colored rings in the visualization "
        "show each band's energy level.\n\n"
        "COUPLING: Controls how energy leaks between frequency bands. On a real gong, hitting one "
        "spot makes vibrations spread across the whole surface. Higher coupling = vibrations in one "
        "band spread to neighboring bands faster. At zero, each band is independent.\n\n"
        "BLOOM: Sets the energy threshold where the gong 'opens up.' Below bloom threshold, energy "
        "decays normally. Above it, a power-law cascade kicks in - energy amplifies itself, making "
        "the sound dramatically brighter and wider. This is how real gongs suddenly 'bloom' when "
        "hit hard enough. Lower threshold = blooms at lower energy = more dramatic response.\n\n"
        "SIGNAL FLOW: Receives strike events from Input -> Sends per-band energy levels to all 4 Resonators",
        juce::Colour(0xffff9800) // Orange
    };
    return info;
}

inline const ModuleInfo& getResonatorModule() {
    static ModuleInfo info {
        "Resonator Bank",
        "4 tuned filters that create the actual gong tone you hear",
        "This is where the sound is actually made. Each of the 4 resonators is a bandpass filter "
        "tuned to a specific frequency. When audio from the Input excites them, they ring at their "
        "tuned pitch - like striking a tuning fork.\n\n"
        "Each resonator has 7 'spread voices' - slightly detuned copies panned across the stereo field. "
        "This creates the characteristic shimmering, wide sound of a real gong.\n\n"
        "ENERGY MODULATION: The Energy Accumulator controls how each resonator behaves moment-to-moment:\n"
        "- Bright mod: Higher energy -> sharper, more resonant filter (brighter sound)\n"
        "- Spread mod: Higher energy -> louder spread voices (wider, more shimmering)\n"
        "- Detune mod: Higher energy -> more detuning between voices (more beating/chorus)\n"
        "- Pan mod: Higher energy -> wider stereo spread\n\n"
        "The orange arcs on the Brightness, Spread, and Pan Width knobs show how energy is currently "
        "pushing these values beyond their base setting.\n\n"
        "TEMPLATES: Pre-configured frequency ratios based on real gong measurements (Chinese opera gong, "
        "Gamelan, Tibetan bowl, etc.).\n\n"
        "SIGNAL FLOW: Receives excitation audio from Input -> Outputs tuned gong tone to Convolution Reverb",
        juce::Colour(0xff2196f3) // Blue
    };
    return info;
}

inline const ModuleInfo& getCombinationModule() {
    static ModuleInfo info {
        "Combination Tones",
        "New frequencies that emerge when resonators interact",
        "When two strong resonator modes vibrate simultaneously, the metal surface creates new "
        "frequencies at their sum and difference. For example, if resonators at 200Hz and 330Hz "
        "are both strong, you'll hear new tones at 530Hz (sum) and 130Hz (difference).\n\n"
        "This only activates when energy is above the threshold - you won't hear combination tones "
        "on gentle strikes, only when the gong is really singing.\n\n"
        "Mix controls the volume of these generated tones. Threshold sets how much energy is needed "
        "before they appear.\n\n"
        "SIGNAL FLOW: Reads from Resonator Bank frequencies + Energy levels -> Adds tones to the audio stream",
        juce::Colour(0xff9c27b0) // Purple
    };
    return info;
}

inline const ModuleInfo& getCrashModule() {
    static ModuleInfo info {
        "Crash Noise",
        "Chaotic broadband noise at extreme energy levels",
        "Hit a real gong hard enough and the surface vibrations become chaotic - you hear a wash "
        "of noise underneath the tonal content. This module simulates that.\n\n"
        "Threshold sets how much energy triggers the noise. Level controls how loud it is. "
        "The noise is bandpass-filtered to sound natural, not like static.\n\n"
        "For subtle use, set threshold high (0.8+) so it only appears on the hardest strikes. "
        "For aggressive textures, lower the threshold.\n\n"
        "SIGNAL FLOW: Reads global energy level -> Adds filtered noise to the audio stream",
        juce::Colour(0xfff44336) // Red
    };
    return info;
}

inline const ModuleInfo& getConvolutionModule() {
    static ModuleInfo info {
        "Convolution Reverb",
        "Shapes the resonator output through a recorded gong impulse response",
        "This module convolves the resonator bank's output with an impulse response (IR) - a recording "
        "of a real gong being struck. The IR captures all the complex resonance, body vibrations, and "
        "decay characteristics of that physical gong. This is what makes the synthesized output sound "
        "like a real gong rather than just filtered noise.\n\n"
        "You can load two IRs (A and B). As energy increases, the output crossfades from IR A to IR B. "
        "Use different gong recordings to morph between gong characters as you play harder.\n\n"
        "The 3-band EQ (Low, Mid, High) shapes the tone after convolution - useful for "
        "taming boomy lows or brightening the high harmonics.\n\n"
        "Mix: 0 = dry (no convolution), 1 = fully convolved. Gain adjusts the convolution output volume.\n\n"
        "SIGNAL FLOW: Receives audio from Resonator Bank + Combo Tones + Crash Noise -> "
        "Applies gong IR -> Outputs to Exciter",
        juce::Colour(0xff009688) // Teal
    };
    return info;
}

inline const ModuleInfo& getExciterModule() {
    static ModuleInfo info {
        "Harmonic Exciter",
        "Adds sparkle and presence through subtle saturation",
        "A highpass filter isolates the upper frequencies, then soft saturation generates new harmonic "
        "overtones from that material. The result is blended back in with the dry signal.\n\n"
        "HP Freq: Sets where the exciter starts working. Higher = only affects the very top end. "
        "Lower = excites a wider range of frequencies.\n"
        "Drive: How much saturation is applied. 1.0 = transparent, higher = more harmonic generation.\n"
        "Mix: How much of the excited signal to blend in. Start low (0.1-0.3) for subtle brightness.\n\n"
        "SIGNAL FLOW: Receives reverbed audio from Convolution -> Outputs to Compressor",
        juce::Colour(0xffe91e63) // Pink
    };
    return info;
}

inline const ModuleInfo& getCompressorModule() {
    static ModuleInfo info {
        "Multiband Compressor",
        "Keeps the volume consistent without killing the dynamics",
        "Splits the signal into 3 frequency bands (low/mid/high) and compresses each independently. "
        "This lets you tame boomy low frequencies without affecting the shimmer of the highs, "
        "or control harsh mids without dulling the overall sound.\n\n"
        "Threshold: Level above which compression kicks in. Lower = more compression.\n"
        "Ratio: How much gain reduction. 4:1 means every 4dB over threshold becomes 1dB.\n"
        "Attack: How fast the compressor reacts. Fast = catches transients, slow = lets strikes through.\n"
        "Release: How fast it stops compressing after the signal drops. Fast = pumping effect, slow = smooth.\n\n"
        "SIGNAL FLOW: Receives audio from Exciter -> Outputs final signal to your speakers",
        juce::Colour(0xff607d8b) // Slate
    };
    return info;
}

// --- Parameter Tooltips ---
// Every tooltip explains what the parameter does AND what you'll hear when you change it.

inline const std::unordered_map<juce::String, juce::String>& getParamTooltips()
{
    static const std::unordered_map<juce::String, juce::String> tooltips = {
        // Input
        { "inputGain",      "Amplifies incoming audio. Turn up if your strikes aren't being detected. "
                            "Too high will cause false triggers from background noise." },
        { "threshold",      "How loud a sound must be to count as a 'strike.' Lower = more sensitive. "
                            "If the gong triggers from room noise, raise this." },
        { "holdoff",        "Minimum time between strikes (in milliseconds). Prevents one hit from "
                            "triggering multiple times. Increase if you get double-triggers." },

        // Synthetic impulse shaping
        { "strikeAttack",   "How sharp the virtual strike is. 0 = soft mallet with a 3ms ramp up. "
                            "255 = hard beater with an instant click. Mid-values give natural mallet feel." },
        { "strikeBright",   "Frequency content of the virtual strike (filter cutoff in Hz). Low values "
                            "create dark, muted strikes like a padded mallet. High values create bright, "
                            "metallic strikes like a hard beater." },
        { "strikeHF",       "Balance between filtered and raw noise in the strike. 0 = only the "
                            "filtered (tonal) component. 255 = pure white noise. Mid-values mix both." },
        { "strikeDecay",    "How long the strike impulse rings. 0 = very short 0.5ms tap. "
                            "255 = longer 5ms thud. Longer decays excite the resonators more broadly." },

        // Energy
        { "energyDecay",    "How long energy takes to fade after a strike (in milliseconds). "
                            "Longer = the gong stays 'excited' longer, meaning brightness and width "
                            "changes persist. Shorter = quick return to base sound." },
        { "injection",      "How much energy each strike adds. Higher = fewer strikes needed to reach "
                            "full energy. At max, one strong hit saturates the system." },
        { "power",          "Shapes the velocity-to-energy curve. At 1.0, energy is proportional to hit "
                            "strength. Above 1.0, soft hits add very little energy but hard hits add a lot. "
                            "Below 1.0, even soft hits add significant energy." },
        { "coupling",       "How fast energy leaks between the 4 frequency bands. At 0, each band is "
                            "independent - a low strike only affects low frequencies. Higher values mean "
                            "energy spreads to all bands, like vibrations traveling across a gong's surface. "
                            "The result is a more complex, evolving tone." },
        { "bloom",          "The energy level where the gong dramatically 'opens up.' Below this threshold, "
                            "energy decays normally. Above it, a cascade effect amplifies energy - the sound "
                            "gets dramatically brighter and wider. Lower threshold = easier to trigger bloom. "
                            "This is what makes real gongs suddenly explode in sound when hit hard enough." },

        // Resonator
        { "resDecay",       "How long resonators ring after excitation (in seconds). This is the main "
                            "sustain control. Longer = the gong rings for ages. Shorter = quick, percussive." },
        { "brightness",     "Base tonal brightness before energy modulation. Controls filter Q (sharpness). "
                            "Higher = more focused, ringing tone. Lower = broader, duller. Energy will push "
                            "this higher when the gong is 'excited.'" },
        { "spreadLevel",    "Volume of the 6 detuned spread voices relative to the center voice. "
                            "0 = pure, focused tone (center voice only). 1 = full shimmer with all spread "
                            "voices. Energy can push this higher for dramatic widening on loud strikes." },
        { "panWidth",       "Stereo width of the spread voices. 0 = everything in mono center. "
                            "1 = voices spread fully across left and right. Creates the immersive, "
                            "enveloping quality of a large gong." },

        // Combination + Crash
        { "comboMix",       "Volume of the combination tones (sum/difference frequencies). Start low (0.1) "
                            "for subtle complexity, increase for more prominent intermodulation." },
        { "comboThreshold", "Minimum energy level for combination tones to appear. Higher = only on hard "
                            "strikes. Lower = always present. Realistic gongs are around 0.3." },
        { "crashThreshold", "Energy level that triggers chaotic noise. Set high (0.8+) for realistic "
                            "crash-only-on-hard-hits. Set lower for a noise-washed experimental texture." },
        { "crashLevel",     "Volume of the crash noise when active. Keep low (0.1-0.3) for realism, "
                            "or crank for industrial textures." },

        // Convolution
        { "convMix",        "Dry/wet balance. 0 = no reverb (dry). 1 = only reverb (wet). "
                            "0.3-0.5 is typical for natural-sounding placement in a space." },
        { "convGain",       "Overall reverb volume in dB. Adjust if the reverb is too loud or quiet "
                            "relative to the dry gong sound." },
        { "postLowEQ",      "Bass shelf EQ applied to the reverb only. Cut (negative) to reduce "
                            "low-end rumble in the reverb. Boost for a warmer, larger space feel." },
        { "postMidEQ",      "Mid-frequency EQ applied to the reverb only. Cut to remove boxy "
                            "room modes. Boost to bring out the reverb's body." },
        { "postHighEQ",     "Treble shelf EQ applied to the reverb only. Cut to darken the reverb "
                            "(like a far-away room). Boost to add air and presence." },

        // Exciter
        { "exciterFreq",    "Highpass cutoff for the exciter. Only frequencies above this get processed. "
                            "Higher = excites only the very top shimmer. Lower = excites more of the sound." },
        { "exciterDrive",   "Saturation intensity. 1.0 = no saturation (transparent). Higher values "
                            "generate more harmonic overtones. Start at 2.0 for subtle warmth." },
        { "exciterMix",     "How much excited signal to blend in. 0 = off. Keep below 0.4 for "
                            "subtle enhancement. Above 0.5 the effect becomes very obvious." },

        // Compressor
        { "compThresh",     "Level (in dB) above which compression starts. Lower threshold = more "
                            "compression. -12dB is moderate, -24dB is heavy." },
        { "compRatio",      "Compression strength. 2:1 is gentle leveling. 4:1 is standard. "
                            "8:1+ is heavy limiting. Higher ratios give more consistent volume." },
        { "compAttack",     "How fast the compressor reacts (in milliseconds). Fast (1-5ms) catches "
                            "every transient. Slow (20-50ms) lets the strike attack through, "
                            "then compresses the sustain." },
        { "compRelease",    "How fast compression stops after signal drops (in milliseconds). "
                            "Fast (20-50ms) can pump. Slow (100-300ms) gives smooth, transparent control." },

        // Glide
        { "glideDir",       "Pitch bend direction under energy. +1 = pitch goes up when energy is high "
                            "(hardening, like a tensioned gong). -1 = pitch goes down (softening)." },
        { "glideSens",      "How many cents of pitch bend per unit energy. 0 = no pitch change. "
                            "50-100 = subtle warble. 200 = dramatic pitch swoops." },
    };
    return tooltips;
}

inline juce::String getTooltip(const juce::String& paramId)
{
    auto& tips = getParamTooltips();
    auto it = tips.find(paramId);
    return it != tips.end() ? it->second : juce::String();
}

// --- Macro Descriptions ---
// Explain what each macro knob controls and what you'll hear

inline const char* getMacroDescription(int index)
{
    static const char* descs[] = {
        // Size
        "SIZE controls the perceived physical size of the gong.\n\n"
        "Turning up Size:\n"
        "- Increases resonator decay times (longer ringing)\n"
        "- Increases inter-band coupling (more complex resonance)\n"
        "- Scales fundamental frequency (deeper pitch)\n\n"
        "Small = quick, bright, focused. Large = deep, sustained, complex.",

        // Material
        "MATERIAL controls the gong's perceived metal type.\n\n"
        "Turning up Material:\n"
        "- Increases base brightness (sharper resonance)\n"
        "- Changes pitch glide direction (hardening vs softening)\n"
        "- Adjusts damping rate\n\n"
        "Low = soft bronze, warm and dark. High = hard steel, bright and cutting.",

        // Intensity
        "INTENSITY controls how dramatically the gong responds to strikes.\n\n"
        "Turning up Intensity:\n"
        "- Increases energy injection gain (more energy per strike)\n"
        "- Lowers bloom threshold (blooms at lower energy)\n"
        "- Lowers crash noise threshold (noise appears sooner)\n\n"
        "Low = gentle, controlled response. High = explosive, dramatic, chaotic.",

        // Space
        "SPACE controls the acoustic environment and stereo width.\n\n"
        "Turning up Space:\n"
        "- Increases convolution reverb wet mix (more room sound)\n"
        "- Increases spread voice stereo width (wider image)\n\n"
        "Low = dry, intimate, narrow. High = huge, reverberant, immersive.",
    };
    return (index >= 0 && index < 4) ? descs[index] : "";
}

} // namespace ModuleDescriptions
