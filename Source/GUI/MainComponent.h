#pragma once
#include "../Audio/AudioEngine.h"
#include "../Core/LibraryManager.h"
#include "../Core/MidiState.h"
#include "../Core/PresetManager.h"
#include "ChannelStripComponent.h"
#include "DeployControlBar.h"
#include "HardwareSelectorBar.h"
#include "MidiMappingDrawer.h"
#include "TelemetryHeaderView.h"
#include <juce_gui_basics/juce_gui_basics.h>

// CORE CONCEPT: Master application GUI container arranging top control bar, telemetry header,
// DAW channel strips (0–3), and collapsible MIDI mapping drawer.
class MainComponent : public juce::Component {
public:
  MainComponent();
  ~MainComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  MidiState midiState;
  AudioEngine audioEngine{midiState};

  LibraryManager libraryManager;
  PresetManager presetManager;

  // Modular Sub-Views
  DeployControlBar deployBar{presetManager};
  TelemetryHeaderView telemetryHeader{midiState, audioEngine};
  HardwareSelectorBar hardwareBar{audioEngine};

  // DAW Channel Strips (0–3)
  ChannelStripComponent channelStrip0{audioEngine, libraryManager, 0};
  ChannelStripComponent channelStrip1{audioEngine, libraryManager, 1};
  ChannelStripComponent channelStrip2{audioEngine, libraryManager, 2};
  ChannelStripComponent channelStrip3{audioEngine, libraryManager, 3};

  // MIDI CC Mapping Drawer
  MidiMappingDrawerComponent midiDrawer{audioEngine};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
