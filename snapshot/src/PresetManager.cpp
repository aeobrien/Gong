#include "PresetManager.h"
#include "GongSynthesizer.h"
#include "ConvolutionEngine.h"
#include "ExciterProcessor.h"
#include "MultibandCompressor.h"
#include "DiagnosticState.h"

PresetManager::PresetManager()
{
}

void PresetManager::setPresetsDirectory(const juce::File& directory)
{
    presetsDirectory = directory;
    if (!presetsDirectory.exists())
        presetsDirectory.createDirectory();
}

void PresetManager::setIRDirectory(const juce::File& directory)
{
    irDirectory = directory;
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;

    if (presetsDirectory.exists())
    {
        auto files = presetsDirectory.findChildFiles(juce::File::findFiles, false, "*.json");
        files.sort();
        for (auto& f : files)
            names.add(f.getFileNameWithoutExtension());
    }

    return names;
}

bool PresetManager::savePreset(const juce::String& name,
                               const GongSynthesizer& synth,
                               const ConvolutionEngine& conv,
                               const ExciterProcessor& exciter,
                               const MultibandCompressor& comp,
                               float masterGain,
                               DiagnosticState* diagState)
{
    if (!presetsDirectory.exists())
        return false;

    auto json = serializeToJson(synth, conv, exciter, comp, masterGain, name, diagState);
    auto file = presetsDirectory.getChildFile(name + ".json");

    auto jsonString = juce::JSON::toString(json);
    if (file.replaceWithText(jsonString))
    {
        currentPresetName = name;
        return true;
    }

    return false;
}

bool PresetManager::loadPreset(const juce::String& name,
                               GongSynthesizer& synth,
                               ConvolutionEngine& conv,
                               ExciterProcessor& exciter,
                               MultibandCompressor& comp,
                               float& masterGain,
                               DiagnosticState* diagState)
{
    auto file = presetsDirectory.getChildFile(name + ".json");
    if (!file.existsAsFile())
        return false;

    auto jsonString = file.loadFileAsString();
    auto json = juce::JSON::parse(jsonString);

    if (json.isVoid())
        return false;

    if (deserializeFromJson(json, synth, conv, exciter, comp, masterGain, diagState))
    {
        currentPresetName = name;
        return true;
    }

    return false;
}

juce::var PresetManager::serializeToJson(const GongSynthesizer& synth,
                                          const ConvolutionEngine& conv,
                                          const ExciterProcessor& exciter,
                                          const MultibandCompressor& comp,
                                          float masterGain,
                                          const juce::String& name,
                                          DiagnosticState* diagState) const
{
    auto* root = new juce::DynamicObject();

    root->setProperty("version", 3);
    root->setProperty("name", name);
    root->setProperty("masterGain", static_cast<double>(masterGain));

    // Resonators
    auto* resonators = new juce::DynamicObject();
    const auto& bank = synth.getResonatorBank();

    resonators->setProperty("globalDecay", static_cast<double>(bank.getResonator(0).getDecayTime()));
    resonators->setProperty("globalBrightness", static_cast<double>(bank.getResonator(0).getBaseBrightness()));
    resonators->setProperty("globalSpreadLevel", static_cast<double>(bank.getResonator(0).getSpreadLevel()));
    resonators->setProperty("globalPanWidth", static_cast<double>(bank.getResonator(0).getSpreadPanWidth()));

    juce::Array<juce::var> voices;
    for (int i = 0; i < ResonatorBank::kNumResonators; ++i)
    {
        auto* voice = new juce::DynamicObject();
        const auto& res = bank.getResonator(i);

        voice->setProperty("enabled", res.getEnabled());
        voice->setProperty("frequencyMode",
            res.getFrequencyMode() == SpreadVoiceResonator::FrequencyMode::Free ? "free" : "snap");
        voice->setProperty("frequencyHz", static_cast<double>(res.getFrequencyHz()));
        voice->setProperty("midiNote", res.getMidiNote());
        voice->setProperty("gainDb", static_cast<double>(res.getGainDb()));

        auto* energyMod = new juce::DynamicObject();
        energyMod->setProperty("brightness", static_cast<double>(res.getBrightnessEnergyAmount()));
        energyMod->setProperty("spread", static_cast<double>(res.getSpreadLevelEnergyAmount()));
        energyMod->setProperty("detune", static_cast<double>(res.getSpreadDetuneEnergyAmount()));
        energyMod->setProperty("panWidth", static_cast<double>(res.getPanWidthEnergyAmount()));
        voice->setProperty("energyMod", juce::var(energyMod));

        // v3: pitch glide
        voice->setProperty("glideDirection", static_cast<double>(res.getPitchGlideDirection()));
        voice->setProperty("glideSensitivity", static_cast<double>(res.getPitchGlideSensitivity()));
        voice->setProperty("glideSmoothing", static_cast<double>(res.getPitchGlideSmoothing()));

        voices.add(juce::var(voice));
    }
    resonators->setProperty("voices", voices);
    root->setProperty("resonators", juce::var(resonators));

    // Energy
    auto* energy = new juce::DynamicObject();
    const auto& ea = synth.getEnergyAccumulator();
    energy->setProperty("decayMs", static_cast<double>(ea.getGlobalDecayMs()));
    energy->setProperty("injectionGain", static_cast<double>(ea.getInjectionGain()));
    energy->setProperty("injectionPower", static_cast<double>(ea.getInjectionPower()));
    // v3: nonlinear energy parameters
    energy->setProperty("couplingRate", static_cast<double>(ea.getCouplingRate()));
    energy->setProperty("bloomThreshold", static_cast<double>(ea.getBloomThreshold()));
    energy->setProperty("bloomRate", static_cast<double>(ea.getBloomRate()));
    energy->setProperty("dampRate", static_cast<double>(ea.getDampRate()));
    energy->setProperty("rollPrimeLevel", static_cast<double>(ea.getRollPrimeLevel()));
    // Per-band decay times
    juce::Array<juce::var> bandDecays;
    for (int i = 0; i < EnergyAccumulator::kNumBands; ++i)
        bandDecays.add(static_cast<double>(ea.getBandDecayMs(i)));
    energy->setProperty("bandDecayMs", bandDecays);
    root->setProperty("energy", juce::var(energy));

    // Convolution
    auto* convObj = new juce::DynamicObject();
    convObj->setProperty("irFilename", conv.getIRFileName());
    convObj->setProperty("irFilenameB", conv.getIRFileNameB());
    convObj->setProperty("mix", static_cast<double>(conv.getWetDryMix()));
    convObj->setProperty("gainDb", static_cast<double>(conv.getOutputGainDb()));
    // v3: post-conv EQ
    convObj->setProperty("postConvLowGainDb", static_cast<double>(conv.getPostConvLowGainDb()));
    convObj->setProperty("postConvMidGainDb", static_cast<double>(conv.getPostConvMidGainDb()));
    convObj->setProperty("postConvHighGainDb", static_cast<double>(conv.getPostConvHighGainDb()));
    root->setProperty("convolution", juce::var(convObj));

    // Exciter
    auto* exciterObj = new juce::DynamicObject();
    exciterObj->setProperty("enabled", exciter.getEnabled());
    exciterObj->setProperty("highpassFreqHz", static_cast<double>(exciter.getHighpassFrequency()));
    exciterObj->setProperty("drive", static_cast<double>(exciter.getSaturationDrive()));
    exciterObj->setProperty("mix", static_cast<double>(exciter.getDryWetMix()));
    root->setProperty("exciter", juce::var(exciterObj));

    // Compressor
    auto* compObj = new juce::DynamicObject();
    compObj->setProperty("enabled", comp.getEnabled());
    // Use band 0 settings as representative (UI sets all bands the same)
    const auto& bandSettings = comp.getBandSettings(0);
    compObj->setProperty("thresholdDb", static_cast<double>(bandSettings.threshold));
    compObj->setProperty("ratio", static_cast<double>(bandSettings.ratio));
    compObj->setProperty("attackMs", static_cast<double>(bandSettings.attackMs));
    compObj->setProperty("releaseMs", static_cast<double>(bandSettings.releaseMs));
    root->setProperty("compressor", juce::var(compObj));

    // Input settings
    auto* input = new juce::DynamicObject();
    input->setProperty("gain", static_cast<double>(synth.getInputGain()));
    input->setProperty("strikeThreshold", static_cast<double>(synth.getStrikeThreshold()));
    input->setProperty("strikeHoldoffMs", static_cast<double>(synth.getStrikeHoldoffMs()));
    root->setProperty("input", juce::var(input));

    // Modulation routes (version 2)
    if (diagState != nullptr)
    {
        auto* modObj = new juce::DynamicObject();

        juce::Array<juce::var> routeArray;
        int routeCount = 0;
        const ModRoute* routes = diagState->getActiveRoutes(routeCount);
        for (int i = 0; i < routeCount; ++i)
        {
            auto* r = new juce::DynamicObject();
            r->setProperty("source", juce::String(modSourceToString(routes[i].source)));
            r->setProperty("target", juce::String(modTargetToString(routes[i].target)));
            r->setProperty("amount", static_cast<double>(routes[i].amount));
            r->setProperty("curve", juce::String(curveTypeToString(routes[i].curve)));
            r->setProperty("enabled", routes[i].enabled);
            routeArray.add(juce::var(r));
        }
        modObj->setProperty("routes", routeArray);

        auto* lfo1Obj = new juce::DynamicObject();
        lfo1Obj->setProperty("rate", static_cast<double>(diagState->lfo1Rate.load(std::memory_order_relaxed)));
        static const char* shapeNames[] = { "sine", "triangle", "saw", "square" };
        int shape1 = diagState->lfo1Shape.load(std::memory_order_relaxed);
        lfo1Obj->setProperty("shape", juce::String(shapeNames[juce::jlimit(0, 3, shape1)]));
        modObj->setProperty("lfo1", juce::var(lfo1Obj));

        auto* lfo2Obj = new juce::DynamicObject();
        lfo2Obj->setProperty("rate", static_cast<double>(diagState->lfo2Rate.load(std::memory_order_relaxed)));
        int shape2 = diagState->lfo2Shape.load(std::memory_order_relaxed);
        lfo2Obj->setProperty("shape", juce::String(shapeNames[juce::jlimit(0, 3, shape2)]));
        modObj->setProperty("lfo2", juce::var(lfo2Obj));

        root->setProperty("modulation", juce::var(modObj));
    }

    return juce::var(root);
}

bool PresetManager::deserializeFromJson(const juce::var& json,
                                         GongSynthesizer& synth,
                                         ConvolutionEngine& conv,
                                         ExciterProcessor& exciter,
                                         MultibandCompressor& comp,
                                         float& masterGain,
                                         DiagnosticState* diagState)
{
    if (!json.isObject())
        return false;

    auto* root = json.getDynamicObject();
    if (root == nullptr)
        return false;

    // Version check
    int version = root->getProperty("version");
    if (version < 1)
        return false;

    // Master gain
    if (root->hasProperty("masterGain"))
        masterGain = static_cast<float>(static_cast<double>(root->getProperty("masterGain")));

    // Input
    auto inputVar = root->getProperty("input");
    if (inputVar.isObject())
    {
        auto* input = inputVar.getDynamicObject();
        if (input->hasProperty("gain"))
            synth.setInputGain(static_cast<float>(static_cast<double>(input->getProperty("gain"))));
        if (input->hasProperty("strikeThreshold"))
            synth.setStrikeThreshold(static_cast<float>(static_cast<double>(input->getProperty("strikeThreshold"))));
        if (input->hasProperty("strikeHoldoffMs"))
            synth.setStrikeHoldoffMs(static_cast<float>(static_cast<double>(input->getProperty("strikeHoldoffMs"))));
    }

    // Resonators
    auto resVar = root->getProperty("resonators");
    if (resVar.isObject())
    {
        auto* resonators = resVar.getDynamicObject();
        auto& bank = synth.getResonatorBank();

        if (resonators->hasProperty("globalDecay"))
            synth.setGlobalDecayTime(static_cast<float>(static_cast<double>(resonators->getProperty("globalDecay"))));
        if (resonators->hasProperty("globalBrightness"))
            synth.setGlobalBrightness(static_cast<float>(static_cast<double>(resonators->getProperty("globalBrightness"))));
        if (resonators->hasProperty("globalSpreadLevel"))
            synth.setGlobalSpreadLevel(static_cast<float>(static_cast<double>(resonators->getProperty("globalSpreadLevel"))));
        if (resonators->hasProperty("globalPanWidth"))
            synth.setGlobalSpreadPanWidth(static_cast<float>(static_cast<double>(resonators->getProperty("globalPanWidth"))));

        auto voicesVar = resonators->getProperty("voices");
        if (voicesVar.isArray())
        {
            auto* voices = voicesVar.getArray();
            for (int i = 0; i < juce::jmin(voices->size(), ResonatorBank::kNumResonators); ++i)
            {
                auto voiceVar = (*voices)[i];
                if (!voiceVar.isObject()) continue;

                auto* voice = voiceVar.getDynamicObject();
                auto& res = bank.getResonator(i);

                if (voice->hasProperty("enabled"))
                    res.setEnabled(static_cast<bool>(voice->getProperty("enabled")));

                if (voice->hasProperty("frequencyMode"))
                {
                    juce::String mode = voice->getProperty("frequencyMode").toString();
                    res.setFrequencyMode(mode == "snap"
                        ? SpreadVoiceResonator::FrequencyMode::Snap
                        : SpreadVoiceResonator::FrequencyMode::Free);
                }

                if (voice->hasProperty("frequencyHz"))
                    res.setFrequencyHz(static_cast<float>(static_cast<double>(voice->getProperty("frequencyHz"))));
                if (voice->hasProperty("midiNote"))
                    res.setMidiNote(static_cast<int>(voice->getProperty("midiNote")));
                if (voice->hasProperty("gainDb"))
                    res.setGainDb(static_cast<float>(static_cast<double>(voice->getProperty("gainDb"))));

                auto emVar = voice->getProperty("energyMod");
                if (emVar.isObject())
                {
                    auto* em = emVar.getDynamicObject();
                    if (em->hasProperty("brightness"))
                        res.setBrightnessEnergyAmount(static_cast<float>(static_cast<double>(em->getProperty("brightness"))));
                    if (em->hasProperty("spread"))
                        res.setSpreadLevelEnergyAmount(static_cast<float>(static_cast<double>(em->getProperty("spread"))));
                    if (em->hasProperty("detune"))
                        res.setSpreadDetuneEnergyAmount(static_cast<float>(static_cast<double>(em->getProperty("detune"))));
                    if (em->hasProperty("panWidth"))
                        res.setPanWidthEnergyAmount(static_cast<float>(static_cast<double>(em->getProperty("panWidth"))));
                }

                // v3: pitch glide
                if (voice->hasProperty("glideDirection"))
                    res.setPitchGlideDirection(static_cast<float>(static_cast<double>(voice->getProperty("glideDirection"))));
                if (voice->hasProperty("glideSensitivity"))
                    res.setPitchGlideSensitivity(static_cast<float>(static_cast<double>(voice->getProperty("glideSensitivity"))));
                if (voice->hasProperty("glideSmoothing"))
                    res.setPitchGlideSmoothing(static_cast<float>(static_cast<double>(voice->getProperty("glideSmoothing"))));
            }
        }
    }

    // Energy
    auto energyVar = root->getProperty("energy");
    if (energyVar.isObject())
    {
        auto* energy = energyVar.getDynamicObject();
        auto& ea = synth.getEnergyAccumulator();

        if (energy->hasProperty("decayMs"))
            ea.setGlobalDecayMs(static_cast<float>(static_cast<double>(energy->getProperty("decayMs"))));
        if (energy->hasProperty("injectionGain"))
            ea.setInjectionGain(static_cast<float>(static_cast<double>(energy->getProperty("injectionGain"))));
        if (energy->hasProperty("injectionPower"))
            ea.setInjectionPower(static_cast<float>(static_cast<double>(energy->getProperty("injectionPower"))));
        // v3 fields
        if (energy->hasProperty("couplingRate"))
            ea.setCouplingRate(static_cast<float>(static_cast<double>(energy->getProperty("couplingRate"))));
        if (energy->hasProperty("bloomThreshold"))
            ea.setBloomThreshold(static_cast<float>(static_cast<double>(energy->getProperty("bloomThreshold"))));
        if (energy->hasProperty("bloomRate"))
            ea.setBloomRate(static_cast<float>(static_cast<double>(energy->getProperty("bloomRate"))));
        if (energy->hasProperty("dampRate"))
            ea.setDampRate(static_cast<float>(static_cast<double>(energy->getProperty("dampRate"))));
        if (energy->hasProperty("rollPrimeLevel"))
            ea.setRollPrimeLevel(static_cast<float>(static_cast<double>(energy->getProperty("rollPrimeLevel"))));
        if (energy->hasProperty("bandDecayMs"))
        {
            auto bdVar = energy->getProperty("bandDecayMs");
            if (bdVar.isArray())
            {
                auto* bdArr = bdVar.getArray();
                for (int i = 0; i < juce::jmin(bdArr->size(), EnergyAccumulator::kNumBands); ++i)
                    ea.setBandDecayMs(i, static_cast<float>(static_cast<double>((*bdArr)[i])));
            }
        }
    }

    // Convolution
    auto convVar = root->getProperty("convolution");
    if (convVar.isObject())
    {
        auto* convObj = convVar.getDynamicObject();

        if (convObj->hasProperty("mix"))
            conv.setWetDryMix(static_cast<float>(static_cast<double>(convObj->getProperty("mix"))));
        if (convObj->hasProperty("gainDb"))
            conv.setOutputGainDb(static_cast<float>(static_cast<double>(convObj->getProperty("gainDb"))));

        // Load IR file if specified
        if (convObj->hasProperty("irFilename"))
        {
            juce::String irFilename = convObj->getProperty("irFilename").toString();
            if (irFilename.isNotEmpty() && irDirectory.exists())
            {
                auto irFile = irDirectory.getChildFile(irFilename);
                if (irFile.existsAsFile())
                    conv.loadImpulseResponse(irFile);
            }
        }
        // v3: Load IR B
        if (convObj->hasProperty("irFilenameB"))
        {
            juce::String irFnB = convObj->getProperty("irFilenameB").toString();
            if (irFnB.isNotEmpty() && irDirectory.exists())
            {
                auto irFileB = irDirectory.getChildFile(irFnB);
                if (irFileB.existsAsFile())
                    conv.loadImpulseResponseB(irFileB);
            }
        }
        // v3: Post-conv EQ
        if (convObj->hasProperty("postConvLowGainDb"))
            conv.setPostConvLowGainDb(static_cast<float>(static_cast<double>(convObj->getProperty("postConvLowGainDb"))));
        if (convObj->hasProperty("postConvMidGainDb"))
            conv.setPostConvMidGainDb(static_cast<float>(static_cast<double>(convObj->getProperty("postConvMidGainDb"))));
        if (convObj->hasProperty("postConvHighGainDb"))
            conv.setPostConvHighGainDb(static_cast<float>(static_cast<double>(convObj->getProperty("postConvHighGainDb"))));
    }

    // Exciter
    auto exciterVar = root->getProperty("exciter");
    if (exciterVar.isObject())
    {
        auto* exciterObj = exciterVar.getDynamicObject();

        if (exciterObj->hasProperty("enabled"))
            exciter.setEnabled(static_cast<bool>(exciterObj->getProperty("enabled")));
        if (exciterObj->hasProperty("highpassFreqHz"))
            exciter.setHighpassFrequency(static_cast<float>(static_cast<double>(exciterObj->getProperty("highpassFreqHz"))));
        if (exciterObj->hasProperty("drive"))
            exciter.setSaturationDrive(static_cast<float>(static_cast<double>(exciterObj->getProperty("drive"))));
        if (exciterObj->hasProperty("mix"))
            exciter.setDryWetMix(static_cast<float>(static_cast<double>(exciterObj->getProperty("mix"))));
    }

    // Compressor
    auto compVar = root->getProperty("compressor");
    if (compVar.isObject())
    {
        auto* compObj = compVar.getDynamicObject();

        if (compObj->hasProperty("enabled"))
            comp.setEnabled(static_cast<bool>(compObj->getProperty("enabled")));
        if (compObj->hasProperty("thresholdDb"))
            comp.setAllThresholds(static_cast<float>(static_cast<double>(compObj->getProperty("thresholdDb"))));
        if (compObj->hasProperty("ratio"))
            comp.setAllRatios(static_cast<float>(static_cast<double>(compObj->getProperty("ratio"))));
        if (compObj->hasProperty("attackMs"))
            comp.setAllAttacks(static_cast<float>(static_cast<double>(compObj->getProperty("attackMs"))));
        if (compObj->hasProperty("releaseMs"))
            comp.setAllReleases(static_cast<float>(static_cast<double>(compObj->getProperty("releaseMs"))));
    }

    // Modulation routes (version 2+)
    if (diagState != nullptr)
    {
        auto modVar = root->getProperty("modulation");
        if (modVar.isObject())
        {
            auto* modObj = modVar.getDynamicObject();

            // Routes
            auto routesVar = modObj->getProperty("routes");
            if (routesVar.isArray())
            {
                auto* routeArr = routesVar.getArray();
                std::vector<ModRoute> routes;
                for (int i = 0; i < routeArr->size() && i < DiagnosticState::kMaxRoutes; ++i)
                {
                    auto rVar = (*routeArr)[i];
                    if (!rVar.isObject()) continue;
                    auto* rObj = rVar.getDynamicObject();
                    ModRoute route;
                    if (rObj->hasProperty("source"))
                        route.source = modSourceFromString(rObj->getProperty("source").toString().toRawUTF8());
                    if (rObj->hasProperty("target"))
                        route.target = modTargetFromString(rObj->getProperty("target").toString().toRawUTF8());
                    if (rObj->hasProperty("amount"))
                        route.amount = static_cast<float>(static_cast<double>(rObj->getProperty("amount")));
                    if (rObj->hasProperty("curve"))
                        route.curve = curveTypeFromString(rObj->getProperty("curve").toString().toRawUTF8());
                    if (rObj->hasProperty("enabled"))
                        route.enabled = static_cast<bool>(rObj->getProperty("enabled"));
                    routes.push_back(route);
                }
                diagState->setRoutes(routes.data(), static_cast<int>(routes.size()));
            }

            // LFO config
            auto lfoShapeFromString = [](const juce::String& s) -> int {
                if (s == "sine") return 0;
                if (s == "triangle") return 1;
                if (s == "saw") return 2;
                if (s == "square") return 3;
                return 0;
            };

            auto lfo1Var = modObj->getProperty("lfo1");
            if (lfo1Var.isObject())
            {
                auto* lfo1Obj = lfo1Var.getDynamicObject();
                if (lfo1Obj->hasProperty("rate"))
                    diagState->lfo1Rate.store(static_cast<float>(static_cast<double>(lfo1Obj->getProperty("rate"))), std::memory_order_relaxed);
                if (lfo1Obj->hasProperty("shape"))
                    diagState->lfo1Shape.store(lfoShapeFromString(lfo1Obj->getProperty("shape").toString()), std::memory_order_relaxed);
            }

            auto lfo2Var = modObj->getProperty("lfo2");
            if (lfo2Var.isObject())
            {
                auto* lfo2Obj = lfo2Var.getDynamicObject();
                if (lfo2Obj->hasProperty("rate"))
                    diagState->lfo2Rate.store(static_cast<float>(static_cast<double>(lfo2Obj->getProperty("rate"))), std::memory_order_relaxed);
                if (lfo2Obj->hasProperty("shape"))
                    diagState->lfo2Shape.store(lfoShapeFromString(lfo2Obj->getProperty("shape").toString()), std::memory_order_relaxed);
            }
        }
    }

    return true;
}
