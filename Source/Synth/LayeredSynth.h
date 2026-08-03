#pragma once
#include "../Audio/PerLayerFilter.h"
#include "../Core/MidiState.h"
#include "CustomSamplerSound.h"
#include "CustomSamplerVoice.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

// CORE CONCEPT: Wraps a JUCE Synthesiser with a helper method to trigger specific voices.
class LayerSynthesiser : public juce::Synthesiser {
public:
  // CORE CONCEPT: Triggers a voice on this synthesiser with a specific sound and MIDI attributes.
  void triggerVoice(juce::SynthesiserVoice *voice, juce::SynthesiserSound *sound,
                    int midiChannel, int midiNoteNumber, float velocity) {
    startVoice(voice, sound, midiChannel, midiNoteNumber, velocity);
  }
};

// CORE CONCEPT: 4-layer polyphonic sample playback engine supporting multi-pool sample dispatch,
// sustain pedal state routing, round-robin cycling, per-layer octave transposing, and hold modes.
class LayeredSynth {
public:
  explicit LayeredSynth(MidiState &stateToUpdate);
  ~LayeredSynth() = default;

  // CORE CONCEPT: Prepares all synth layers for playback at the specified audio sample rate.
  void prepareToPlay(double sampleRate, int samplesPerBlock);

  // CORE CONCEPT: Processes incoming MIDI buffer and renders audio output for all 4 synth layers.
  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                       juce::MidiBuffer &midiMessages, int startSample,
                       int numSamples);

  // CORE CONCEPT: Adds a decoded sample sound object to the specified layer (0–3).
  void addSoundToLayer(int layerIndex, juce::SynthesiserSound::Ptr sound);

  // CORE CONCEPT: Clears all loaded sample sounds from the specified layer.
  void clearLayer(int layerIndex);

  // CORE CONCEPT: Clears all loaded sample sounds across all 4 layers.
  void clearAllLayers();

  // CORE CONCEPT: Sets linear volume gain factor for the specified layer (0.0 to 1.0+).
  void setLayerVolume(int layerIndex, float gainLinear);

  // CORE CONCEPT: Sets mute state for the specified layer.
  void setLayerMute(int layerIndex, bool isMuted);
  void setLayerMuted(int layerIndex, bool isMuted) { setLayerMute(layerIndex, isMuted); }

  // CORE CONCEPT: Sets octave transposition offset (-2 to +2 octaves) for layers 2 & 3.
  void setLayerOctaveOffset(int layerIndex, int octaveOffset);

  // CORE CONCEPT: Gets octave transposition offset for specified layer.
  int getLayerOctaveOffset(int layerIndex) const;

  // CORE CONCEPT: Sets hold mode toggle for layers 2 & 3 (blocks new NoteOns, holds sounding notes).
  void setLayerHold(int layerIndex, bool holdActive);

  // CORE CONCEPT: Gets hold mode state for specified layer.
  bool getLayerHold(int layerIndex) const;

  // CORE CONCEPT: Returns reference to per-layer TPT filter processor for layer (0–3).
  PerLayerFilter &getLayerFilter(int layerIndex) {
    return layerFilters[juce::jlimit(0, 3, layerIndex)];
  }

private:
  // CORE CONCEPT: Selects the best matching CustomSamplerSound for an event, note, velocity, and pedal state using round-robin cycling.
  juce::SynthesiserSound::Ptr findMatchingSound(int layerIdx, SampleType targetType, int note, int velInt, bool pedalDown);

  MidiState &midiState;
  static constexpr int NUM_LAYERS = 4;
  static constexpr int VOICES_PER_LAYER = 32;

  struct Layer {
    LayerSynthesiser synth;
    float volumeGain{1.0f};
    bool muted{false};
    int octaveOffset{0};
    bool holdActive{false};
    bool hasFilter{false};
  };

  std::array<Layer, NUM_LAYERS> layers;
  std::array<PerLayerFilter, NUM_LAYERS> layerFilters;
  std::array<std::array<uint8_t, 128>, NUM_LAYERS> rrCounters{};
  std::array<uint8_t, NUM_LAYERS> pedalDownRrCounters{};
  std::array<uint8_t, NUM_LAYERS> pedalUpRrCounters{};
  bool lastPedalState{false};
};
