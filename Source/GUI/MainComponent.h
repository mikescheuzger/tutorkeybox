#pragma once
#include "../Audio/AudioEngine.h"
#include "../Core/MidiState.h"
#include "../Core/PresetManager.h"
#include "DeployControlBar.h"
#include "HardwareSelectorBar.h"
#include "LayerCardComponent.h"
#include "TelemetryHeaderView.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Master Application Container arranging modular sub-views.
 */
class MainComponent : public juce::Component {
public:
  MainComponent();
  ~MainComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  MidiState midiState;
  AudioEngine audioEngine{midiState};

#include "../Core/PresetManager.h"
#include "DeployControlBar.h"

  PresetManager presetManager;

  // Modular Sub-Views
  DeployControlBar deployBar{presetManager};
  TelemetryHeaderView telemetryHeader{midiState, audioEngine};
  HardwareSelectorBar hardwareBar{audioEngine};

  LayerCardComponent layerCard0{0, audioEngine, presetManager};
  LayerCardComponent layerCard1{1, audioEngine, presetManager};
  LayerCardComponent layerCard2{2, audioEngine, presetManager};
  LayerCardComponent layerCard3{3, audioEngine, presetManager};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
