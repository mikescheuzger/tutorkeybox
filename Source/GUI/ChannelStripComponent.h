#pragma once
#include "../Audio/AudioEngine.h"
#include "../Core/LibraryManager.h"
#include "FilterWindow.h"
#include "ReverbWindow.h"
#include "SamplerWindow.h"
#include <juce_gui_basics/juce_gui_basics.h>

// CORE CONCEPT: DAW-style mixer channel strip representing one synth layer with interactive sampler slot button,
// FX inserts (Reverb & Filter), octave shift buttons, HOLD toggle, vertical volume fader, and mute button.
class ChannelStripComponent : public juce::Component {
public:
  ChannelStripComponent(AudioEngine &engine, LibraryManager &library, int layerIndex);
  ~ChannelStripComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // CORE CONCEPT: Refreshes channel strip volume, mute, octave, and hold states from engine.
  void refreshFromEngine();

private:
  AudioEngine &audioEngine;
  LibraryManager &libraryManager;
  int layerIdx{0};

  juce::Label headerLabel;

  // Slot 1: Instrument Sampler Slot
  juce::TextButton samplerSlotButton{"[ SAMPLER ]"};

  // Slot 2: FX Inserts
  juce::TextButton reverbFxButton{"FX: REVERB"};
  juce::TextButton filterFxButton{"FX: FILTER"};

  // Layer 2 & 3 controls
  juce::TextButton octDownButton{"OCT -"};
  juce::TextButton octUpButton{"OCT +"};
  juce::TextButton holdButton{"HOLD"};

  // Fader & Mute
  juce::Slider volumeFader;
  juce::ToggleButton muteButton{"MUTE"};

  // Active floating windows
  juce::Component::SafePointer<juce::DocumentWindow> activeSamplerWindow;
  juce::Component::SafePointer<juce::DocumentWindow> activeReverbWindow;
  juce::Component::SafePointer<juce::DocumentWindow> activeFilterWindow;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStripComponent)
};
