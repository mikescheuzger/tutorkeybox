#pragma once
#include "../Audio/AudioEngine.h"
#include "../Core/MidiState.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Developer Surveillance Monitor & 4-Layer Control Interface.
 * Implements juce::FileDragAndDropTarget for drag-and-drop sample loading.
 */
class MidiMonitorView : public juce::Component,
                        public juce::Timer,
                        public juce::FileDragAndDropTarget {
public:
  explicit MidiMonitorView(MidiState &stateToMonitor,
                           AudioEngine &engineToControl);
  ~MidiMonitorView() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

  // --- File Drag-and-Drop Target Interface ---
  bool isInterestedInFileDrag(const juce::StringArray &files) override;
  void fileDragEnter(const juce::StringArray &files, int x, int y) override;
  void fileDragExit(const juce::StringArray &files) override;
  void filesDropped(const juce::StringArray &files, int x, int y) override;

private:
  MidiState &midiState;
  AudioEngine &audioEngine;

  // GUI Controls per Layer
  std::array<juce::Slider, 4> volumeSliders;
  std::array<juce::ToggleButton, 4> muteButtons;
  std::array<juce::String, 4> layerLoadedNames{"Empty Layer", "Empty Layer",
                                               "Empty Layer", "Empty Layer"};
  int hoveredLayerIndex{-1};

  int lastNote{-1};
  float lastVel{0.0f};
  bool lastKeyState{false};
  bool lastSustainState{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiMonitorView);
};
