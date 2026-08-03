#include "PresetManager.h"

PresetManager::PresetManager() {
  for (int i = 0; i < 4; ++i) {
    layers[i].volume = 1.0f;
    layers[i].muted = false;
    layers[i].sampleContainerPath = "";
  }
}

void PresetManager::setFaderCc(int faderIndex, int ccNumber) {
  if (faderIndex >= 0 && faderIndex < 4) {
    ccMapping.faderCc[faderIndex] = ccNumber;
  }
}

int PresetManager::getFaderCc(int faderIndex) const {
  return (faderIndex >= 0 && faderIndex < 4) ? ccMapping.faderCc[faderIndex]
                                             : -1;
}

void PresetManager::setButtonCc(int buttonIndex, int ccNumber) {
  if (buttonIndex >= 0 && buttonIndex < 3) {
    ccMapping.buttonCc[buttonIndex] = ccNumber;
  }
}

int PresetManager::getButtonCc(int buttonIndex) const {
  return (buttonIndex >= 0 && buttonIndex < 3) ? ccMapping.buttonCc[buttonIndex]
                                              : -1;
}

void PresetManager::setKnobCc(int knobIndex, int ccNumber) {
  if (knobIndex >= 0 && knobIndex < 2) {
    ccMapping.knobCc[knobIndex] = ccNumber;
  }
}

int PresetManager::getKnobCc(int knobIndex) const {
  return (knobIndex >= 0 && knobIndex < 2) ? ccMapping.knobCc[knobIndex] : -1;
}

void PresetManager::setLayerPreset(int layerIndex, float volume, bool muted,
                                    const juce::String &containerPath) {
  if (layerIndex >= 0 && layerIndex < 4) {
    layers[layerIndex].volume = volume;
    layers[layerIndex].muted = muted;
    layers[layerIndex].sampleContainerPath = containerPath;
  }
}

const LayerPreset &PresetManager::getLayerPreset(int layerIndex) const {
  static LayerPreset defaultLayer;
  return (layerIndex >= 0 && layerIndex < 4) ? layers[layerIndex]
                                             : defaultLayer;
}

juce::String PresetManager::toJsonString() const {
  auto root = std::make_unique<juce::DynamicObject>();

  // MIDI CC Mappings
  auto ccObj = std::make_unique<juce::DynamicObject>();
  juce::Array<juce::var> fadersArr, buttonsArr, knobsArr;
  for (int i = 0; i < 4; ++i)
    fadersArr.add(ccMapping.faderCc[i]);
  for (int i = 0; i < 3; ++i)
    buttonsArr.add(ccMapping.buttonCc[i]);
  for (int i = 0; i < 2; ++i)
    knobsArr.add(ccMapping.knobCc[i]);

  ccObj->setProperty("faders", fadersArr);
  ccObj->setProperty("buttons", buttonsArr);
  ccObj->setProperty("knobs", knobsArr);
  ccObj->setProperty("sustainPedal", ccMapping.sustainPedalCc);

  root->setProperty("ccMapping", ccObj.release());

  // Layers
  juce::Array<juce::var> layersArr;
  for (int i = 0; i < 4; ++i) {
    auto layerObj = std::make_unique<juce::DynamicObject>();
    layerObj->setProperty("volume", layers[i].volume);
    layerObj->setProperty("muted", layers[i].muted);
    layerObj->setProperty("containerPath", layers[i].sampleContainerPath);
    layersArr.add(layerObj.release());
  }

  root->setProperty("layers", layersArr);

  return juce::JSON::toString(juce::var(root.release()));
}

bool PresetManager::loadFromJsonString(const juce::String &jsonText) {
  auto parsed = juce::JSON::parse(jsonText);
  if (!parsed.isObject())
    return false;

  auto *root = parsed.getDynamicObject();
  if (root == nullptr)
    return false;

  if (root->hasProperty("ccMapping")) {
    auto *ccObj = root->getProperty("ccMapping").getDynamicObject();
    if (ccObj != nullptr) {
      if (auto *faders = ccObj->getProperty("faders").getArray()) {
        for (int i = 0; i < juce::jmin(4, faders->size()); ++i)
          ccMapping.faderCc[i] = (int)(*faders)[i];
      }
      if (auto *buttons = ccObj->getProperty("buttons").getArray()) {
        for (int i = 0; i < juce::jmin(3, buttons->size()); ++i)
          ccMapping.buttonCc[i] = (int)(*buttons)[i];
      }
      if (auto *knobs = ccObj->getProperty("knobs").getArray()) {
        for (int i = 0; i < juce::jmin(2, knobs->size()); ++i)
          ccMapping.knobCc[i] = (int)(*knobs)[i];
      }
    }
  }

  if (root->hasProperty("layers")) {
    if (auto *layersList = root->getProperty("layers").getArray()) {
      for (int i = 0; i < juce::jmin(4, layersList->size()); ++i) {
        if (auto *layerObj = (*layersList)[i].getDynamicObject()) {
          layers[i].volume = (float)layerObj->getProperty("volume");
          layers[i].muted = (bool)layerObj->getProperty("muted");
          layers[i].sampleContainerPath =
              layerObj->getProperty("containerPath").toString();
        }
      }
    }
  }

  return true;
}

bool PresetManager::saveToFile(const juce::File &file) const {
  return file.replaceWithText(toJsonString());
}

bool PresetManager::loadFromFile(const juce::File &file) {
  if (!file.existsAsFile())
    return false;
  return loadFromJsonString(file.loadFileAsString());
}
