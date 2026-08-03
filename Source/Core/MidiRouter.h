#pragma once
#include "../Synth/LayeredSynth.h"
#include "PresetManager.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

// CORE CONCEPT: Enum representing all mappable hardware controller targets.
enum class MidiControlTarget {
  Fader0_MasterVol = 0,  // Fader 0: Piano master volume (layers 0 & 1)
  Fader1_Crossfade = 1,  // Fader 1: Power-law crossfade Layer 0 <-> Layer 1
  Fader2_MacroA    = 2,  // Fader 2: Macro A (multi-param control for layers 2 & 3)
  Fader3_MacroB    = 3,  // Fader 3: Macro B (multi-param control for layers 2 & 3)
  Button0_OctDown  = 4,  // Button 0: Octave Down for layers 2 & 3
  Button1_OctUp    = 5,  // Button 1: Octave Up for layers 2 & 3
  Button2_Hold     = 6,  // Button 2: HOLD toggle for layers 2 & 3
  Count            = 7
};

// CORE CONCEPT: Real-time MIDI CC router and mapping engine. Dispatches CC messages
// to target parameters (master volume, crossfade, macros, octave, hold), applies curve
// transformations and reverse mappings, and supports CC Learn.
class MidiRouter {
public:
  explicit MidiRouter(LayeredSynth &synthToControl);
  ~MidiRouter() = default;

  // CORE CONCEPT: Processes incoming MIDI CC message and dispatches control changes to synth engine.
  void processMidiCc(const juce::MidiMessage &message);

  // CORE CONCEPT: Enables or disables CC Learn mode for a specific control target.
  void setCcLearnTarget(int targetIndex);

  // CORE CONCEPT: Gets currently active CC Learn target index (-1 if inactive).
  int getCcLearnTarget() const { return learnTargetIndex; }

  // CORE CONCEPT: Updates CC mapping configuration from PresetManager.
  void updateFromPreset(const MidiCcMapping &mapping);

  // CORE CONCEPT: Returns active MidiCcMapping configuration.
  const MidiCcMapping &getMapping() const { return currentMapping; }

  // CORE CONCEPT: Evaluates a 0.0–1.0 normalized control input through the configured curve (Linear, Log, Exp, S-Curve).
  static float applyFaderCurve(float normVal, int curveType, bool reversed);

private:
  LayeredSynth &synth;
  MidiCcMapping currentMapping;

  int learnTargetIndex{-1};
  float masterVolume{1.0f};
  float crossfadePosition{0.5f}; // 0.0 = Layer 0 only, 1.0 = Layer 1 only

  // Helper to re-calculate combined layer volumes for Layer 0 & 1 based on masterVolume and crossfadePosition
  void updatePianoLayerVolumes();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRouter)
};
