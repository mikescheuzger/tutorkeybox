#pragma once
#include "../Audio/AudioEngine.h"
#include "../Synth/SampleContainerReader.h"
#include "../Synth/SamplePackager.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Modular GUI Card representing a single instrument layer (0 to 3).
 * Implements juce::FileDragAndDropTarget for layer-specific sample dropping.
 */
class LayerCardComponent : public juce::Component,
                           public juce::FileDragAndDropTarget {
public:
  LayerCardComponent(int layerIndexToManage, AudioEngine &engineToControl);
  ~LayerCardComponent() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // --- Drag & Drop Interface for THIS Layer ---
  bool isInterestedInFileDrag(const juce::StringArray &files) override;
  void fileDragEnter(const juce::StringArray &files, int x, int y) override;
  void fileDragExit(const juce::StringArray &files) override;
  void filesDropped(const juce::StringArray &files, int x, int y) override;

private:
  int layerIndex;
  AudioEngine &audioEngine;

  juce::Slider volumeSlider;
  juce::ToggleButton muteButton;
  juce::String loadedInstrumentName{"Empty Layer"};
  bool isHoveredDuringDrag{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerCardComponent);
};
