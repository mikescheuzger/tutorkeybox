#pragma once
#include <juce_core/juce_core.h>

struct MidiCcMapping {
  int faderCc[4]{1, 2, 3, 4};  // Default CCs for Faders 1-4
  int buttonCc[3]{5, 6, 7};  // Default CCs for Buttons 1-3
  int knobCc[2]{8, 9};       // Default CCs for Knobs 1-2
  int sustainPedalCc{64};    // Default Sustain Pedal CC
};

struct LayerPreset {
  float volume{1.0f};
  bool muted{false};
  juce::String sampleContainerPath;
};

class PresetManager {
public:
  PresetManager();
  ~PresetManager() = default;

  void setFaderCc(int faderIndex, int ccNumber);
  int getFaderCc(int faderIndex) const;

  void setButtonCc(int buttonIndex, int ccNumber);
  int getButtonCc(int buttonIndex) const;

  void setKnobCc(int knobIndex, int ccNumber);
  int getKnobCc(int knobIndex) const;

  void setLayerPreset(int layerIndex, float volume, bool muted,
                      const juce::String &containerPath);
  const LayerPreset &getLayerPreset(int layerIndex) const;

  juce::String toJsonString() const;
  bool loadFromJsonString(const juce::String &jsonText);

  bool saveToFile(const juce::File &file) const;
  bool loadFromFile(const juce::File &file);

private:
  MidiCcMapping ccMapping;
  std::array<LayerPreset, 4> layers;
};
