#pragma once
#include "../Audio/AudioEngine.h"
#include "../Core/MidiState.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Surveillance Header displaying real-time MIDI status and System/Audio
 * Telemetry Gauges.
 */
class TelemetryHeaderView : public juce::Component, public juce::Timer {
public:
  explicit TelemetryHeaderView(MidiState &stateToMonitor,
                               AudioEngine &engineToMonitor);
  ~TelemetryHeaderView() override;

  void paint(juce::Graphics &g) override;
  void timerCallback() override;

private:
  MidiState &midiState;
  AudioEngine &audioEngine;

  double audioCpuUsage{0.0};
  double systemCpuUsage{0.0};

  int lastNote{-1};
  float lastVel{0.0f};
  bool lastSustainState{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TelemetryHeaderView);
};
