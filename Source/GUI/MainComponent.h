#pragma once
#include "../Audio/AudioEngine.h"
#include "../Core/MidiState.h"
#include "MidiMonitorView.h"
#include <juce_gui_extra/juce_gui_extra.h>

/**
 * Main application window component holding the AudioEngine and GUI Views.
 */
class MainComponent : public juce::Component {
public:
  MainComponent(); // <-- 0 arguments
  ~MainComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  MidiState midiState;
  AudioEngine audioEngine{midiState};
  MidiMonitorView midiMonitorView{
      midiState, audioEngine}; // <-- Pass both midiState and audioEngine

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
