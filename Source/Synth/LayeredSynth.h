#pragma once
#include "../Core/MidiState.h"
#include "CustomSamplerSound.h"
#include "CustomSamplerVoice.h"
#include <juce_audio_basics/juce_audio_basics.h>

class LayerSynthesiser : public juce::Synthesiser {
public:
  void triggerVoice(juce::SynthesiserVoice *voice, juce::SynthesiserSound *sound,
                    int midiChannel, int midiNoteNumber, float velocity) {
    startVoice(voice, sound, midiChannel, midiNoteNumber, velocity);
  }
};

class LayeredSynth {
public:
  explicit LayeredSynth(MidiState &stateToUpdate);
  ~LayeredSynth() = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock);
  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                       juce::MidiBuffer &midiMessages, int startSample,
                       int numSamples);

  void addSoundToLayer(int layerIndex, juce::SynthesiserSound::Ptr sound);
  void clearLayer(int layerIndex);
  void clearAllLayers();

  void setLayerVolume(int layerIndex, float gainLinear);
  void setLayerMute(int layerIndex, bool isMuted);
  void setLayerMuted(int layerIndex, bool isMuted) {
    setLayerMute(layerIndex, isMuted);
  } // Alias

private:
  MidiState &midiState;
  static constexpr int NUM_LAYERS = 4;
  static constexpr int VOICES_PER_LAYER = 32;

  struct Layer {
    LayerSynthesiser synth;
    float volumeGain{1.0f};
    bool muted{false};
  };

  std::array<Layer, NUM_LAYERS> layers;
};
