#include "LayeredSynth.h"

LayeredSynth::LayeredSynth(MidiState &stateToUpdate)
    : midiState(stateToUpdate) {
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    for (int v = 0; v < VOICES_PER_LAYER; ++v) {
      layers[layerIdx].synth.addVoice(new CustomSamplerVoice(&stateToUpdate));
    }
  }
}

void LayeredSynth::prepareToPlay(double sampleRate, int samplesPerBlock) {
  int safeBlockSize = juce::jmax(512, samplesPerBlock);
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    layers[layerIdx].synth.setCurrentPlaybackSampleRate(sampleRate);
    // Optimization A: Pre-allocate tempBuffer capacity ONCE
    layers[layerIdx].tempBuffer.setSize(2, safeBlockSize, false, false, true);
  }
}

void LayeredSynth::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                   juce::MidiBuffer &midiMessages,
                                   int startSample, int numSamples) {
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    auto &layer = layers[layerIdx];
    if (layer.muted)
      continue;

    juce::MidiBuffer nonNoteOnMessages;

    for (const auto metadata : midiMessages) {
      auto msg = metadata.getMessage();
      if (msg.isNoteOn()) {
        int note = juce::jlimit(0, 127, msg.getNoteNumber());
        int velInt = juce::jlimit(
            0, 127, juce::roundToInt(msg.getFloatVelocity() * 127.0f));

        // Optimization B: O(1) Instant Direct Sound Lookup Grid Access!
        juce::SynthesiserSound::Ptr matchingSound =
            layer.soundLookupGrid[note][velInt];

        if (matchingSound != nullptr) {
          juce::SynthesiserVoice *voiceToUse = nullptr;
          for (int v = 0; v < layer.synth.getNumVoices(); ++v) {
            auto *vPtr = layer.synth.getVoice(v);
            if (!vPtr->isVoiceActive()) {
              voiceToUse = vPtr;
              break;
            }
          }
          if (voiceToUse == nullptr && layer.synth.getNumVoices() > 0) {
            voiceToUse = layer.synth.getVoice(0); // Voice stealing
          }

          if (voiceToUse != nullptr) {
            layer.synth.triggerVoice(voiceToUse, matchingSound.get(),
                                     msg.getChannel(), note,
                                     msg.getFloatVelocity());
          }
        }
      } else {
        nonNoteOnMessages.addEvent(msg, metadata.samplePosition);
      }
    }

    // Optimization A: Reuse pre-allocated tempBuffer without heap malloc
    layer.tempBuffer.clear(0, numSamples);
    layer.synth.renderNextBlock(layer.tempBuffer, nonNoteOnMessages, 0,
                                numSamples);

    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      outputBuffer.addFrom(ch, startSample, layer.tempBuffer, ch, 0, numSamples,
                           layer.volumeGain);
    }
  }
}

void LayeredSynth::addSoundToLayer(int layerIndex,
                                   juce::SynthesiserSound::Ptr sound) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    auto &layer = layers[layerIndex];
    layer.synth.addSound(sound);

    // Optimization B: Populate O(1) Sound Lookup Grid for fast note triggering
    if (auto *cs = dynamic_cast<CustomSamplerSound *>(sound.get())) {
      const auto &entry = cs->getEntry();
      int kLow = juce::jlimit(0, 127, (int)entry.keyLow);
      int kHigh = juce::jlimit(0, 127, (int)entry.keyHigh);
      int vLow = juce::jlimit(0, 127, (int)entry.velLow);
      int vHigh = juce::jlimit(0, 127, (int)entry.velHigh);

      for (int n = kLow; n <= kHigh; ++n) {
        for (int v = vLow; v <= vHigh; ++v) {
          layer.soundLookupGrid[n][v] = sound;
        }
      }
    }
  }
}

void LayeredSynth::clearLayer(int layerIndex) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    auto &layer = layers[layerIndex];
    layer.synth.clearSounds();
    for (int n = 0; n < 128; ++n) {
      for (int v = 0; v < 128; ++v) {
        layer.soundLookupGrid[n][v] = nullptr;
      }
    }
  }
}

void LayeredSynth::clearAllLayers() {
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    clearLayer(layerIdx);
  }
}

void LayeredSynth::setLayerVolume(int layerIndex, float gainLinear) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].volumeGain = gainLinear;
  }
}

void LayeredSynth::setLayerMute(int layerIndex, bool isMuted) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].muted = isMuted;
  }
}
