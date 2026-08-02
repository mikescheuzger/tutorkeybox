#pragma once
#include "../Audio/AudioEngine.h"
#include "../Core/MidiState.h"
#include "HardwareSelectorBar.h"
#include "LayerCardComponent.h"
#include "TelemetryHeaderView.h"
#include <juce_gui_extra/juce_gui_extra.h>

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

  // Modular Sub-Views
  TelemetryHeaderView telemetryHeader{midiState, audioEngine};
  HardwareSelectorBar hardwareBar{audioEngine};

  LayerCardComponent layerCard0{0, audioEngine};
  LayerCardComponent layerCard1{1, audioEngine};
  LayerCardComponent layerCard2{2, audioEngine};
  LayerCardComponent layerCard3{3, audioEngine};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
