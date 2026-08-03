#include "PresetManager.h"

// CORE CONCEPT: Constructor initializing 4 layers with default parameters.
PresetManager::PresetManager() {
    for (int i = 0; i < 4; ++i) {
        layers[i].volume   = 1.0f;
        layers[i].muted    = false;
        layers[i].sampleContainerFilename = "";
        layers[i].sampleContainerPath     = "";
        layers[i].octaveOffset = 0;
        layers[i].holdActive   = false;
        layers[i].hasFilter    = (i >= 2); // layers 2+3 have filter enabled
    }
}

// -- CC mapping accessors --

void PresetManager::setFaderCc(int faderIndex, int ccNumber) {
    if (faderIndex >= 0 && faderIndex < 4)
        ccMapping.faderCc[faderIndex] = ccNumber;
}
int PresetManager::getFaderCc(int faderIndex) const {
    return (faderIndex >= 0 && faderIndex < 4) ? ccMapping.faderCc[faderIndex] : -1;
}

void PresetManager::setButtonCc(int buttonIndex, int ccNumber) {
    if (buttonIndex >= 0 && buttonIndex < 3)
        ccMapping.buttonCc[buttonIndex] = ccNumber;
}
int PresetManager::getButtonCc(int buttonIndex) const {
    return (buttonIndex >= 0 && buttonIndex < 3) ? ccMapping.buttonCc[buttonIndex] : -1;
}

void PresetManager::setKnobCc(int knobIndex, int ccNumber) {
    if (knobIndex >= 0 && knobIndex < 2)
        ccMapping.knobCc[knobIndex] = ccNumber;
}
int PresetManager::getKnobCc(int knobIndex) const {
    return (knobIndex >= 0 && knobIndex < 2) ? ccMapping.knobCc[knobIndex] : -1;
}

// -- Layer preset accessors --

void PresetManager::setLayerPreset(int layerIndex, float volume, bool muted,
                                    const juce::String& containerFilename) {
    if (layerIndex >= 0 && layerIndex < 4) {
        layers[layerIndex].volume = volume;
        layers[layerIndex].muted  = muted;
        layers[layerIndex].sampleContainerFilename = containerFilename;
    }
}

void PresetManager::setLayerPresetFull(int layerIndex, const LayerPreset& preset) {
    if (layerIndex >= 0 && layerIndex < 4)
        layers[layerIndex] = preset;
}

const LayerPreset& PresetManager::getLayerPreset(int layerIndex) const {
    static LayerPreset defaultLayer;
    return (layerIndex >= 0 && layerIndex < 4) ? layers[layerIndex] : defaultLayer;
}

// -- JSON serialization --

// CORE CONCEPT: Serializes preset structure (reverb, CC mappings, layer filter/envelopes) to JSON.
juce::String PresetManager::toJsonString() const {
    auto root = std::make_unique<juce::DynamicObject>();

    // Reverb
    auto revObj = std::make_unique<juce::DynamicObject>();
    revObj->setProperty("irFilePath", reverb.irFilePath);
    revObj->setProperty("mix",        reverb.mix);
    revObj->setProperty("preDelayMs", reverb.preDelayMs);
    revObj->setProperty("dampingHz",  reverb.dampingHz);
    root->setProperty("reverb", revObj.release());

    // MIDI CC Mappings
    auto ccObj = std::make_unique<juce::DynamicObject>();
    juce::Array<juce::var> fadersArr, buttonsArr, knobsArr, faderRevArr, faderCurveArr;
    for (int i = 0; i < 4; ++i) {
        fadersArr.add(ccMapping.faderCc[i]);
        faderRevArr.add(ccMapping.faderReversed[i]);
        faderCurveArr.add(ccMapping.faderCurve[i]);
    }
    for (int i = 0; i < 3; ++i) buttonsArr.add(ccMapping.buttonCc[i]);
    for (int i = 0; i < 2; ++i) knobsArr.add(ccMapping.knobCc[i]);

    ccObj->setProperty("faders",       fadersArr);
    ccObj->setProperty("faderReversed",faderRevArr);
    ccObj->setProperty("faderCurve",   faderCurveArr);
    ccObj->setProperty("buttons",      buttonsArr);
    ccObj->setProperty("knobs",        knobsArr);
    ccObj->setProperty("sustainPedal", ccMapping.sustainPedalCc);
    root->setProperty("ccMapping", ccObj.release());

    // Layers
    juce::Array<juce::var> layersArr;
    for (int i = 0; i < 4; ++i) {
        auto layerObj = std::make_unique<juce::DynamicObject>();
        layerObj->setProperty("volume",        layers[i].volume);
        layerObj->setProperty("muted",         layers[i].muted);
        layerObj->setProperty("containerFile", layers[i].sampleContainerFilename);
        layerObj->setProperty("containerPath", layers[i].sampleContainerPath);
        layerObj->setProperty("octaveOffset",  layers[i].octaveOffset);
        layerObj->setProperty("holdActive",    layers[i].holdActive);
        layerObj->setProperty("hasFilter",     layers[i].hasFilter);

        // Filter object
        auto filtObj = std::make_unique<juce::DynamicObject>();
        filtObj->setProperty("cutoffHz",   layers[i].filter.cutoffHz);
        filtObj->setProperty("resonance",  layers[i].filter.resonance);
        filtObj->setProperty("drive",      layers[i].filter.drive);
        filtObj->setProperty("filterType", layers[i].filter.filterType);
        layerObj->setProperty("filter", filtObj.release());

        // Filter Envelope object
        auto fenvObj = std::make_unique<juce::DynamicObject>();
        fenvObj->setProperty("attackMs",  layers[i].filterEnv.attackMs);
        fenvObj->setProperty("decayMs",   layers[i].filterEnv.decayMs);
        fenvObj->setProperty("sustain",   layers[i].filterEnv.sustain);
        fenvObj->setProperty("releaseMs", layers[i].filterEnv.releaseMs);
        fenvObj->setProperty("depth",     layers[i].filterEnv.depth);
        layerObj->setProperty("filterEnv", fenvObj.release());

        // Amp Envelope object
        auto aenvObj = std::make_unique<juce::DynamicObject>();
        aenvObj->setProperty("attackMs",  layers[i].ampEnv.attackMs);
        aenvObj->setProperty("decayMs",   layers[i].ampEnv.decayMs);
        aenvObj->setProperty("sustain",   layers[i].ampEnv.sustain);
        aenvObj->setProperty("releaseMs", layers[i].ampEnv.releaseMs);
        layerObj->setProperty("ampEnv", aenvObj.release());

        layersArr.add(layerObj.release());
    }
    root->setProperty("layers", layersArr);
    root->setProperty("presetVersion", 2);

    return juce::JSON::toString(juce::var(root.release()));
}

// CORE CONCEPT: Deserializes JSON text into internal preset structs.
bool PresetManager::loadFromJsonString(const juce::String& jsonText) {
    auto parsed = juce::JSON::parse(jsonText);
    if (!parsed.isObject()) return false;

    auto* root = parsed.getDynamicObject();
    if (!root) return false;

    // Reverb
    if (root->hasProperty("reverb")) {
        if (auto* revObj = root->getProperty("reverb").getDynamicObject()) {
            reverb.irFilePath = revObj->getProperty("irFilePath").toString();
            reverb.mix        = (float)revObj->getProperty("mix");
            reverb.preDelayMs = (float)revObj->getProperty("preDelayMs");
            reverb.dampingHz  = (float)revObj->getProperty("dampingHz");
        }
    }

    // CC mappings
    if (root->hasProperty("ccMapping")) {
        auto* ccObj = root->getProperty("ccMapping").getDynamicObject();
        if (ccObj) {
            if (auto* faders = ccObj->getProperty("faders").getArray())
                for (int i = 0; i < juce::jmin(4, faders->size()); ++i)
                    ccMapping.faderCc[i] = (int)(*faders)[i];
            if (auto* rev = ccObj->getProperty("faderReversed").getArray())
                for (int i = 0; i < juce::jmin(4, rev->size()); ++i)
                    ccMapping.faderReversed[i] = (bool)(*rev)[i];
            if (auto* curve = ccObj->getProperty("faderCurve").getArray())
                for (int i = 0; i < juce::jmin(4, curve->size()); ++i)
                    ccMapping.faderCurve[i] = (int)(*curve)[i];
            if (auto* buttons = ccObj->getProperty("buttons").getArray())
                for (int i = 0; i < juce::jmin(3, buttons->size()); ++i)
                    ccMapping.buttonCc[i] = (int)(*buttons)[i];
            if (auto* knobs = ccObj->getProperty("knobs").getArray())
                for (int i = 0; i < juce::jmin(2, knobs->size()); ++i)
                    ccMapping.knobCc[i] = (int)(*knobs)[i];
            if (ccObj->hasProperty("sustainPedal"))
                ccMapping.sustainPedalCc = (int)ccObj->getProperty("sustainPedal");
        }
    }

    // Layers
    if (root->hasProperty("layers")) {
        if (auto* layersList = root->getProperty("layers").getArray()) {
            for (int i = 0; i < juce::jmin(4, layersList->size()); ++i) {
                if (auto* layerObj = (*layersList)[i].getDynamicObject()) {
                    layers[i].volume = (float)layerObj->getProperty("volume");
                    layers[i].muted  = (bool) layerObj->getProperty("muted");

                    juce::String containerFile = layerObj->getProperty("containerFile").toString();
                    juce::String containerPath = layerObj->getProperty("containerPath").toString();

                    if (containerFile.isNotEmpty()) {
                        layers[i].sampleContainerFilename = containerFile;
                    } else if (containerPath.isNotEmpty()) {
                        layers[i].sampleContainerFilename = juce::File(containerPath).getFileName();
                        layers[i].sampleContainerPath     = containerPath;
                    }

                    if (layerObj->hasProperty("octaveOffset"))
                        layers[i].octaveOffset = (int)layerObj->getProperty("octaveOffset");
                    if (layerObj->hasProperty("holdActive"))
                        layers[i].holdActive = (bool)layerObj->getProperty("holdActive");
                    if (layerObj->hasProperty("hasFilter"))
                        layers[i].hasFilter = (bool)layerObj->getProperty("hasFilter");

                    // Filter
                    if (layerObj->hasProperty("filter")) {
                        if (auto* fObj = layerObj->getProperty("filter").getDynamicObject()) {
                            layers[i].filter.cutoffHz   = (float)fObj->getProperty("cutoffHz");
                            layers[i].filter.resonance  = (float)fObj->getProperty("resonance");
                            layers[i].filter.drive      = (float)fObj->getProperty("drive");
                            layers[i].filter.filterType = (int)  fObj->getProperty("filterType");
                        }
                    }

                    // Filter Envelope
                    if (layerObj->hasProperty("filterEnv")) {
                        if (auto* feObj = layerObj->getProperty("filterEnv").getDynamicObject()) {
                            layers[i].filterEnv.attackMs  = (float)feObj->getProperty("attackMs");
                            layers[i].filterEnv.decayMs   = (float)feObj->getProperty("decayMs");
                            layers[i].filterEnv.sustain   = (float)feObj->getProperty("sustain");
                            layers[i].filterEnv.releaseMs = (float)feObj->getProperty("releaseMs");
                            layers[i].filterEnv.depth     = (float)feObj->getProperty("depth");
                        }
                    }

                    // Amp Envelope
                    if (layerObj->hasProperty("ampEnv")) {
                        if (auto* aeObj = layerObj->getProperty("ampEnv").getDynamicObject()) {
                            layers[i].ampEnv.attackMs  = (float)aeObj->getProperty("attackMs");
                            layers[i].ampEnv.decayMs   = (float)aeObj->getProperty("decayMs");
                            layers[i].ampEnv.sustain   = (float)aeObj->getProperty("sustain");
                            layers[i].ampEnv.releaseMs = (float)aeObj->getProperty("releaseMs");
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool PresetManager::saveToFile(const juce::File& file) const {
    return file.replaceWithText(toJsonString());
}

bool PresetManager::loadFromFile(const juce::File& file) {
    if (!file.existsAsFile()) return false;
    return loadFromJsonString(file.loadFileAsString());
}
