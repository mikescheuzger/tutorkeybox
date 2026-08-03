#pragma once
#include "../Audio/AudioEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>

// CORE CONCEPT: Floating editor component for configuring post-fader convolution reverb parameters
// (IR sample loading, dry/wet mix, pre-delay, and high-frequency damping).
class ReverbWindowComponent : public juce::Component {
public:
  explicit ReverbWindowComponent(AudioEngine &engine);
  ~ReverbWindowComponent() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  AudioEngine &audioEngine;

  juce::Label titleLabel;
  juce::Label irLabel{"irLbl", "Impulse Response:"};
  juce::ComboBox irComboBox;

  juce::GroupComponent paramsGroup{"pGroup", "REVERB PARAMETERS"};
  juce::Slider mixSlider;
  juce::Slider preDelaySlider;
  juce::Slider dampingSlider;

  juce::Label mixLabel{"mixLbl", "Dry/Wet Mix"};
  juce::Label preDelayLabel{"pdLbl", "Pre-Delay (ms)"};
  juce::Label dampingLabel{"dampLbl", "Damping (Hz)"};

  void setupSlider(juce::Slider &slider, juce::Label &label, float minVal, float maxVal, float defaultVal, const juce::String &suffix);
  void populateIrDropdown();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbWindowComponent)
};

// CORE CONCEPT: Floating window container for ReverbWindowComponent.
class ReverbWindow : public juce::DocumentWindow {
public:
  explicit ReverbWindow(AudioEngine &engine);
  ~ReverbWindow() override = default;

  void closeButtonPressed() override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbWindow)
};
