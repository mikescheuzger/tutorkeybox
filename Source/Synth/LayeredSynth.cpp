#include "LayeredSynth.h"

LayeredSynth::LayeredSynth() {
  // Initialize 32 CustomSamplerVoice instances for each of the 4 layers (128
  // voices total)
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    for (int v = 0; v < VOICES_PER_LAYER; ++v) {
      layers[layerIdx].addVoice(new CustomSamplerVoice());
    }
  }
}

void LayeredSynth::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    layers[layerIdx].setCurrentPlaybackSampleRate(sampleRate);
  }
}

void LayeredSynth::addSoundToLayer(
    int layerIndex, const juce::SynthesiserSound::Ptr &newSound) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].addSound(newSound);
  }
}

void LayeredSynth::clearLayerSounds(int layerIndex) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].clearSounds();
  }
}

void LayeredSynth::setLayerVolume(int layerIndex, float gain) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layerGains[layerIndex] = juce::jlimit(0.0f, 1.0f, gain);
  }
}

void LayeredSynth::setLayerMuted(int layerIndex, bool mute) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layerMuted[layerIndex] = mute;
  }
}

void LayeredSynth::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                   const juce::MidiBuffer &midiMessages,
                                   int startSample, int numSamples) {
  // Temporary audio buffer for rendering each layer independently
  juce::AudioBuffer<float> layerBuffer(outputBuffer.getNumChannels(),
                                       numSamples);

  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    // Skip muted layers
    if (layerMuted[layerIdx] || layerGains[layerIdx] <= 0.001f)
      continue;

    layerBuffer.clear();

    // Render layer's 32 voices
    layers[layerIdx].renderNextBlock(layerBuffer, midiMessages, 0, numSamples);

    // Apply layer volume gain
    float gain = layerGains[layerIdx];
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      outputBuffer.addFrom(ch, startSample, layerBuffer, ch, 0, numSamples,
                           gain);
    }
  }
}
