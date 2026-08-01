#pragma once
#include "CustomSamplerSound.h"
#include "CustomSamplerVoice.h"
#include <juce_audio_basics/juce_audio_basics.h>

/**
 * 4-Layer Polyphonic Synthesizer Engine (128 total voices).
 */
class LayeredSynth {
public:
  static constexpr int NUM_LAYERS = 4;
  static constexpr int VOICES_PER_LAYER = 32;

  LayeredSynth();
  ~LayeredSynth() = default;

  // Prepare sample rate & buffer size for all 4 layers
  void prepareToPlay(double sampleRate, int samplesPerBlock);

  // Add a sound sample to a specific layer (0 - 3)
  void addSoundToLayer(int layerIndex,
                       const juce::SynthesiserSound::Ptr &newSound);

  // Clear all sounds from a specific layer
  void clearLayerSounds(int layerIndex);

  // Layer Controls: Volume (0.0 to 1.0) and Mute
  void setLayerVolume(int layerIndex, float gain);
  void setLayerMuted(int layerIndex, bool mute);

  // Real-Time Audio Rendering (Renders all 4 layers and mixes audio)
  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                       const juce::MidiBuffer &midiMessages, int startSample,
                       int numSamples);

private:
  std::array<juce::Synthesiser, NUM_LAYERS> layers;
  std::array<float, NUM_LAYERS> layerGains{1.0f, 1.0f, 1.0f, 1.0f};
  std::array<bool, NUM_LAYERS> layerMuted{false, false, false, false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayeredSynth)
};
