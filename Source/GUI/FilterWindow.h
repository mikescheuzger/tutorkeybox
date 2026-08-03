#pragma once
#include "../Audio/AudioEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>

// CORE CONCEPT: Floating editor component for configuring per-layer TPT filter parameters
// (cutoff, resonance, drive, filter type) and filter envelope ADSR modulation depth for Layers 2 & 3.
class FilterWindowComponent : public juce::Component {
public:
  FilterWindowComponent(AudioEngine &engine, int layerIndex);
  ~FilterWindowComponent() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  AudioEngine &audioEngine;
  int layerIdx{2};

  juce::Label titleLabel;

  juce::GroupComponent filterGroup{"fGroup", "TPT FILTER SETTINGS"};
  juce::Label typeLabel{"tLbl", "Filter Type:"};
  juce::ComboBox typeComboBox;

  juce::Slider cutoffSlider;
  juce::Slider resonanceSlider;
  juce::Slider driveSlider;

  juce::Label cutoffLabel{"cLbl", "Cutoff (Hz)"};
  juce::Label resonanceLabel{"qLbl", "Resonance Q"};
  juce::Label driveLabel{"drvLbl", "Drive Saturation"};

  juce::GroupComponent fenvGroup{"feGroup", "FILTER ENVELOPE (ADSR)"};
  juce::Slider attackSlider;
  juce::Slider decaySlider;
  juce::Slider sustainSlider;
  juce::Slider releaseSlider;
  juce::Slider depthSlider;

  juce::Label attackLabel{"faLbl", "Att (ms)"};
  juce::Label decayLabel{"fdLbl", "Dec (ms)"};
  juce::Label sustainLabel{"fsLbl", "Sus"};
  juce::Label releaseLabel{"frLbl", "Rel (ms)"};
  juce::Label depthLabel{"fdepLbl", "Env Depth"};

  void setupSlider(juce::Slider &slider, juce::Label &label, float minVal, float maxVal, float defaultVal, const juce::String &suffix, bool isLog = false);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterWindowComponent)
};

// CORE CONCEPT: Floating window container for FilterWindowComponent.
class FilterWindow : public juce::DocumentWindow {
public:
  FilterWindow(AudioEngine &engine, int layerIndex);
  ~FilterWindow() override = default;

  void closeButtonPressed() override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterWindow)
};
