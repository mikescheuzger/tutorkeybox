#pragma once
#include "../Audio/AudioEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Top control bar with live Audio Output & MIDI Input device selector
 * dropdowns.
 */
class HardwareSelectorBar : public juce::Component {
public:
  explicit HardwareSelectorBar(AudioEngine &engineToControl);
  ~HardwareSelectorBar() override = default;

  void resized() override;
  void updateDropdowns();

private:
  AudioEngine &audioEngine;

  juce::ComboBox audioOutputSelector;
  juce::ComboBox midiInputSelector;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HardwareSelectorBar);
};
