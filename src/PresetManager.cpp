#include "PresetManager.h"
#include "GongSynthesizer.h"
#include "ConvolutionEngine.h"
#include "ExciterProcessor.h"
#include "MultibandCompressor.h"

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
                               float masterGain)
{
    if (!presetsDirectory.exists())
        return false;

    auto json = serializeToJson(synth, conv, exciter, comp, masterGain, name);
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
                               float& masterGain)
{
    auto file = presetsDirectory.getChildFile(name + ".json");
    if (!file.existsAsFile())
        return false;

    auto jsonString = file.loadFileAsString();
    auto json = juce::JSON::parse(jsonString);

    if (json.isVoid())
        return false;

    if (deserializeFromJson(json, synth, conv, exciter, comp, masterGain))
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
                                          const juce::String& name) const
{
    auto* root = new juce::DynamicObject();

    root->setProperty("version", 1);
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
    root->setProperty("energy", juce::var(energy));

    // Convolution
    auto* convObj = new juce::DynamicObject();
    convObj->setProperty("irFilename", conv.getIRFileName());
    convObj->setProperty("mix", static_cast<double>(conv.getWetDryMix()));
    convObj->setProperty("gainDb", static_cast<double>(conv.getOutputGainDb()));
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

    return juce::var(root);
}

bool PresetManager::deserializeFromJson(const juce::var& json,
                                         GongSynthesizer& synth,
                                         ConvolutionEngine& conv,
                                         ExciterProcessor& exciter,
                                         MultibandCompressor& comp,
                                         float& masterGain)
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

    return true;
}
