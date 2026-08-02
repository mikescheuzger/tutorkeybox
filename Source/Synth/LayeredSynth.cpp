#include "LayeredSynth.h"

LayeredSynth::LayeredSynth(MidiState &stateToUpdate)
    : midiState(stateToUpdate) {
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    for (int v = 0; v < VOICES_PER_LAYER; ++v) {
      layers[layerIdx].synth.addVoice(new CustomSamplerVoice(&stateToUpdate));
    }
  }
}

void LayeredSynth::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    layers[layerIdx].synth.setCurrentPlaybackSampleRate(sampleRate);
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
        int note = msg.getNoteNumber();
        int velInt = juce::roundToInt(msg.getFloatVelocity() * 127.0f);

        // Find the SINGLE matching sound for pitch AND velocity!
        juce::SynthesiserSound::Ptr matchingSound = nullptr;
        for (int s = 0; s < layer.synth.getNumSounds(); ++s) {
          auto snd = layer.synth.getSound(s);
          if (auto *cs = const_cast<CustomSamplerSound *>(
                  dynamic_cast<const CustomSamplerSound *>(snd.get()))) {
            if (cs->appliesToNote(note) && cs->appliesToVelocity(velInt)) {
              matchingSound = snd;
              break;
            }
          }
        }

        if (matchingSound != nullptr) {
          // Find a free or oldest voice on this layer synth
          juce::SynthesiserVoice *voiceToUse = nullptr;
          for (int v = 0; v < layer.synth.getNumVoices(); ++v) {
            auto *vPtr = layer.synth.getVoice(v);
            if (!vPtr->isVoiceActive()) {
              voiceToUse = vPtr;
              break;
            }
          }
          if (voiceToUse == nullptr && layer.synth.getNumVoices() > 0) {
            voiceToUse = layer.synth.getVoice(0); // Reuse voice 0 if all busy
          }

          if (voiceToUse != nullptr) {
            layer.synth.triggerVoice(voiceToUse, matchingSound.get(),
                                     msg.getChannel(), note, msg.getFloatVelocity());
          }
        }
      } else {
        nonNoteOnMessages.addEvent(msg, metadata.samplePosition);
      }
    }

    juce::AudioBuffer<float> tempBuffer(outputBuffer.getNumChannels(),
                                        numSamples);
    tempBuffer.clear();

    // Render audio and non-NoteOn messages (Note Off, Sustain Pedal, etc.)
    layer.synth.renderNextBlock(tempBuffer, nonNoteOnMessages, 0, numSamples);

    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      outputBuffer.addFrom(ch, startSample, tempBuffer, ch, 0, numSamples,
                           layer.volumeGain);
    }
  }
}

void LayeredSynth::addSoundToLayer(int layerIndex,
                                   juce::SynthesiserSound::Ptr sound) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].synth.addSound(sound);
  }
}

void LayeredSynth::clearLayer(int layerIndex) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].synth.clearSounds();
  }
}

void LayeredSynth::clearAllLayers() {
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    layers[layerIdx].synth.clearSounds();
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
