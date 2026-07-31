#pragma once
#include "../Core/MidiState.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>
/**
 * Developer Surveillance Component showing active MIDI note, velocity, and gate
 * status.
 */
class MidiMonitorView : public juce::Component, private juce::Timer {
public:
  explicit MidiMonitorView(const MidiState &stateToMonitor);
  ~MidiMonitorView() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  void timerCallback() override;

  const MidiState &midiState;

  // Cached values to avoid unnecessary repaints
  bool lastNoteActive{false};
  int lastNoteNumber{-1};
  float lastVelocity{0.0f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiMonitorView)
};
