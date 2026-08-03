#pragma once
#include "../Audio/AudioEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>

// CORE CONCEPT: Collapsible bottom drawer component providing MIDI CC mapping configuration,
// CC Learn activation buttons, fader curve selection, and reverse-mapping toggles.
class MidiMappingDrawerComponent : public juce::Component {
public:
  explicit MidiMappingDrawerComponent(AudioEngine &engine);
  ~MidiMappingDrawerComponent() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // CORE CONCEPT: Refreshes CC mapping fields from MidiRouter.
  void refreshFromEngine();

private:
  AudioEngine &audioEngine;

  juce::Label titleLabel;

  // Fader controls (Fader 0–3)
  struct FaderRow {
    juce::Label label;
    juce::TextEditor ccInput;
    juce::ComboBox curveCombo;
    juce::ToggleButton reverseToggle{"Rev"};
    juce::TextButton learnButton{"Learn"};
  };
  std::array<FaderRow, 4> faderRows;

  // Button controls (Button 0–2)
  struct ButtonRow {
    juce::Label label;
    juce::TextEditor ccInput;
    juce::TextButton learnButton{"Learn"};
  };
  std::array<ButtonRow, 3> buttonRows;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiMappingDrawerComponent)
};
