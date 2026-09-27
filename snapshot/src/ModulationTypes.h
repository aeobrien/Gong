#pragma once

#include <array>

enum class ModSource : int {
    InputLevel, GlobalEnergy,
    BandEnergy0, BandEnergy1, BandEnergy2, BandEnergy3,
    OutputLevel, LFO1, LFO2,
    Count
};

enum class ModTarget : int {
    ConvMix, ConvGain,
    ExciterFreq, ExciterDrive, ExciterMix, ExciterOutputGain,
    CompThreshold, CompRatio, CompAttack, CompRelease, CompOutputGain,
    SynthDecay, SynthBrightness, SynthSpreadLevel, SynthSpreadPanWidth,
    EnergyDecayMs, EnergyInjectionGain, EnergyInjectionPower,
    MasterGain,
    // Step 4: Roll prime level
    RollPrimeLevel,
    // Step 6: Pitch glide parameters
    PitchGlideDirection, PitchGlideSensitivity,
    // Step 8: Post-convolution EQ
    PostConvLowGain, PostConvMidGain, PostConvHighGain,
    Count
};

enum class CurveType : int {
    Linear, Exponential, SCurve, Logarithmic,
    DualSigmoid  // Step 7: Ruelle-Takens bifurcation model
};
static constexpr int kNumCurveTypes = 5;

struct ModRoute {
    ModSource source = ModSource::InputLevel;
    ModTarget target = ModTarget::ConvMix;
    float amount = 0.0f;
    CurveType curve = CurveType::Linear;
    bool enabled = true;
};

struct TargetSpec {
    float minVal, maxVal;
    const char* name;
};

inline const std::array<TargetSpec, static_cast<int>(ModTarget::Count)>& getTargetSpecs()
{
    static const std::array<TargetSpec, static_cast<int>(ModTarget::Count)> specs = {{
        { 0.0f, 1.0f, "Conv Mix" },
        { -24.0f, 24.0f, "Conv Gain" },
        { 200.0f, 2000.0f, "Exciter Freq" },
        { 1.0f, 5.0f, "Exciter Drive" },
        { 0.0f, 1.0f, "Exciter Mix" },
        { -24.0f, 12.0f, "Exciter Out Gain" },
        { -40.0f, 0.0f, "Comp Threshold" },
        { 1.0f, 20.0f, "Comp Ratio" },
        { 1.0f, 100.0f, "Comp Attack" },
        { 10.0f, 500.0f, "Comp Release" },
        { -24.0f, 12.0f, "Comp Out Gain" },
        { 0.5f, 15.0f, "Synth Decay" },
        { 0.0f, 1.0f, "Synth Brightness" },
        { 0.0f, 1.0f, "Synth Spread Level" },
        { 0.0f, 1.0f, "Synth Pan Width" },
        { 500.0f, 5000.0f, "Energy Decay ms" },
        { 0.1f, 3.0f, "Energy Injection" },
        { 0.5f, 3.0f, "Energy Power" },
        { 0.0f, 1.0f, "Master Gain" },
        // New targets
        { 0.0f, 1.0f, "Roll Prime Level" },
        { -1.0f, 1.0f, "Pitch Glide Dir" },
        { 0.0f, 200.0f, "Pitch Glide Sens" },
        { -12.0f, 12.0f, "PostConv Low" },
        { -12.0f, 12.0f, "PostConv Mid" },
        { -12.0f, 12.0f, "PostConv High" },
    }};
    return specs;
}

inline const char* getSourceName(ModSource s)
{
    static const char* names[] = {
        "Input Level", "Global Energy",
        "Band Energy 0", "Band Energy 1", "Band Energy 2", "Band Energy 3",
        "Output Level", "LFO 1", "LFO 2"
    };
    int i = static_cast<int>(s);
    return (i >= 0 && i < static_cast<int>(ModSource::Count)) ? names[i] : "?";
}

inline const char* getTargetName(ModTarget t)
{
    int i = static_cast<int>(t);
    auto& specs = getTargetSpecs();
    return (i >= 0 && i < static_cast<int>(ModTarget::Count)) ? specs[i].name : "?";
}

inline const char* getCurveName(CurveType c)
{
    static const char* names[] = { "Linear", "Exponential", "S-Curve", "Logarithmic", "Dual Sigmoid" };
    return names[static_cast<int>(c)];
}

// String<->enum conversion for preset serialization
inline const char* modSourceToString(ModSource s) { return getSourceName(s); }
inline const char* modTargetToString(ModTarget t) { return getTargetName(t); }
inline const char* curveTypeToString(CurveType c) { return getCurveName(c); }

inline ModSource modSourceFromString(const char* str)
{
    for (int i = 0; i < static_cast<int>(ModSource::Count); ++i)
        if (std::strcmp(getSourceName(static_cast<ModSource>(i)), str) == 0)
            return static_cast<ModSource>(i);
    return ModSource::InputLevel;
}

inline ModTarget modTargetFromString(const char* str)
{
    for (int i = 0; i < static_cast<int>(ModTarget::Count); ++i)
        if (std::strcmp(getTargetName(static_cast<ModTarget>(i)), str) == 0)
            return static_cast<ModTarget>(i);
    return ModTarget::ConvMix;
}

inline CurveType curveTypeFromString(const char* str)
{
    static const char* names[] = { "Linear", "Exponential", "S-Curve", "Logarithmic", "Dual Sigmoid" };
    for (int i = 0; i < kNumCurveTypes; ++i)
        if (std::strcmp(names[i], str) == 0)
            return static_cast<CurveType>(i);
    return CurveType::Linear;
}
