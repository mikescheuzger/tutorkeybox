#include "LayeredSynth.h"
#include <algorithm>

// CORE CONCEPT: Constructor allocating polyphonic CustomSamplerVoice instances per layer and initializing filters for layers 2 & 3.
LayeredSynth::LayeredSynth(MidiState &stateToUpdate)
    : midiState(stateToUpdate) {
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    for (int v = 0; v < VOICES_PER_LAYER; ++v) {
      layers[layerIdx].synth.addVoice(new CustomSamplerVoice(&stateToUpdate));
    }
  }
  layers[2].hasFilter = true;
  layers[3].hasFilter = true;
}

// CORE CONCEPT: Sets playback sample rate across all synth layer instances and prepares per-layer filters.
void LayeredSynth::prepareToPlay(double sampleRate, int samplesPerBlock) {
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    layers[layerIdx].synth.setCurrentPlaybackSampleRate(sampleRate);
    layerFilters[layerIdx].prepare(sampleRate, samplesPerBlock, 2);
  }
}

// CORE CONCEPT: Selects the best matching CustomSamplerSound for an event, note, velocity,
// and pedal state using round-robin cycling across available sample variations.
juce::SynthesiserSound::Ptr LayeredSynth::findMatchingSound(
    int layerIdx, SampleType targetType, int note, int velInt, bool pedalDown) {
  auto &layer = layers[layerIdx];

  // 1. Collect candidate sounds matching targetType, note, and velocity
  std::vector<juce::SynthesiserSound::Ptr> candidates;
  for (int s = 0; s < layer.synth.getNumSounds(); ++s) {
    auto snd = layer.synth.getSound(s);
    if (auto *cs = dynamic_cast<const CustomSamplerSound *>(snd.get())) {
      if (cs->appliesToEvent(targetType, note, velInt)) {
        candidates.push_back(snd);
      }
    }
  }

  // Fallback: If sustain pedal is pressed but layer has no WithPedal sample for this note, fall back to Normal
  if (candidates.empty() && targetType == SampleType::WithPedal) {
    for (int s = 0; s < layer.synth.getNumSounds(); ++s) {
      auto snd = layer.synth.getSound(s);
      if (auto *cs = dynamic_cast<const CustomSamplerSound *>(snd.get())) {
        if (cs->appliesToEvent(SampleType::Normal, note, velInt)) {
          candidates.push_back(snd);
        }
      }
    }
  }

  if (candidates.empty())
    return nullptr;

  // 2. Sort candidates by roundRobinIndex for deterministic cycling
  std::sort(candidates.begin(), candidates.end(),
            [](const juce::SynthesiserSound::Ptr &a,
               const juce::SynthesiserSound::Ptr &b) {
              auto *csA = static_cast<const CustomSamplerSound *>(a.get());
              auto *csB = static_cast<const CustomSamplerSound *>(b.get());
              return csA->getRoundRobinIndex() < csB->getRoundRobinIndex();
            });

  // 3. Round-Robin index lookup and counter increment
  uint8_t &rrCounter = (targetType == SampleType::PedalDown)
                           ? pedalDownRrCounters[layerIdx]
                       : (targetType == SampleType::PedalUp)
                           ? pedalUpRrCounters[layerIdx]
                           : rrCounters[layerIdx][juce::jlimit(0, 127, note)];

  size_t selectedIdx = rrCounter % candidates.size();
  rrCounter++;

  return candidates[selectedIdx];
}

// CORE CONCEPT: Audio render loop parsing real-time MIDI events, dispatching multi-pool
// note/release/pedal sounds, applying octave offsets/hold modes, and rendering mixed layer audio.
void LayeredSynth::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                   juce::MidiBuffer &midiMessages,
                                   int startSample, int numSamples) {
  bool currentPedalDown = midiState.isSustainPedalDown.load(std::memory_order_relaxed);

  // 1. Process MIDI buffer events (NoteOn, NoteOff, Pedal CC64)
  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();

    if (msg.isController() && msg.getControllerNumber() == 64) {
      bool newPedalState = (msg.getControllerValue() >= 64);
      if (newPedalState != lastPedalState) {
        lastPedalState = newPedalState;
        midiState.setSustainPedal(newPedalState);

        SampleType noiseType = newPedalState ? SampleType::PedalDown : SampleType::PedalUp;

        // Trigger pedal noise across active layers
        for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
          auto &layer = layers[layerIdx];
          if (layer.muted) continue;

          auto noiseSound = findMatchingSound(layerIdx, noiseType, 60, msg.getControllerValue(), newPedalState);
          if (noiseSound != nullptr) {
            juce::SynthesiserVoice *voiceToUse = nullptr;
            for (int v = 0; v < layer.synth.getNumVoices(); ++v) {
              auto *vPtr = layer.synth.getVoice(v);
              if (!vPtr->isVoiceActive()) {
                voiceToUse = vPtr;
                break;
              }
            }
            if (voiceToUse == nullptr && layer.synth.getNumVoices() > 0)
              voiceToUse = layer.synth.getVoice(0);

            if (voiceToUse != nullptr) {
              layer.synth.triggerVoice(voiceToUse, noiseSound.get(),
                                       msg.getChannel(), 60,
                                       msg.getControllerValue() / 127.0f);
            }
          }
        }
      }
    } else if (msg.isNoteOn()) {
      int note = msg.getNoteNumber();
      int velInt = juce::roundToInt(msg.getFloatVelocity() * 127.0f);

      for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
        auto &layer = layers[layerIdx];
        if (layer.muted) continue;
        if (layer.holdActive) continue; // HOLD mode blocks new NoteOn events

        int effectiveNote = juce::jlimit(0, 127, note + layer.octaveOffset * 12);
        SampleType targetType = currentPedalDown ? SampleType::WithPedal : SampleType::Normal;

        auto matchingSound = findMatchingSound(layerIdx, targetType, effectiveNote, velInt, currentPedalDown);

        if (matchingSound != nullptr) {
          juce::SynthesiserVoice *voiceToUse = nullptr;
          for (int v = 0; v < layer.synth.getNumVoices(); ++v) {
            auto *vPtr = layer.synth.getVoice(v);
            if (!vPtr->isVoiceActive()) {
              voiceToUse = vPtr;
              break;
            }
          }
          if (voiceToUse == nullptr && layer.synth.getNumVoices() > 0)
            voiceToUse = layer.synth.getVoice(0);

          if (voiceToUse != nullptr) {
            layer.synth.triggerVoice(voiceToUse, matchingSound.get(),
                                     msg.getChannel(), effectiveNote,
                                     msg.getFloatVelocity());
          }
        }
      }
    } else if (msg.isNoteOff()) {
      int note = msg.getNoteNumber();

      for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
        auto &layer = layers[layerIdx];
        if (layer.muted) continue;
        if (layer.holdActive) continue; // HOLD mode keeps notes sounding, ignores NoteOff

        int effectiveNote = juce::jlimit(0, 127, note + layer.octaveOffset * 12);

        // Check for ReleaseTail sound
        auto releaseSound = findMatchingSound(layerIdx, SampleType::ReleaseTail, effectiveNote, 127, currentPedalDown);
        if (releaseSound != nullptr) {
          juce::SynthesiserVoice *voiceToUse = nullptr;
          for (int v = 0; v < layer.synth.getNumVoices(); ++v) {
            auto *vPtr = layer.synth.getVoice(v);
            if (!vPtr->isVoiceActive()) {
              voiceToUse = vPtr;
              break;
            }
          }
          if (voiceToUse == nullptr && layer.synth.getNumVoices() > 0)
            voiceToUse = layer.synth.getVoice(0);

          if (voiceToUse != nullptr) {
            layer.synth.triggerVoice(voiceToUse, releaseSound.get(),
                                     msg.getChannel(), effectiveNote,
                                     msg.getFloatVelocity());
          }
        }

        // Send noteOff to active voices playing effectiveNote
        layer.synth.noteOff(msg.getChannel(), effectiveNote, msg.getFloatVelocity(), true);
      }
    }
  }

  // 2. Render layer synths into audio buffer
  juce::AudioBuffer<float> tempBuffer(outputBuffer.getNumChannels(), numSamples);

  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    auto &layer = layers[layerIdx];
    if (layer.muted) continue;

    tempBuffer.clear();

    // Render synth buffer (other non NoteOn/Off CCs processed natively)
    juce::MidiBuffer emptyMidi;
    layer.synth.renderNextBlock(tempBuffer, emptyMidi, 0, numSamples);

    // Apply per-layer TPT filter (layers 2 & 3)
    if (layer.hasFilter) {
      layerFilters[layerIdx].process(tempBuffer, 0, numSamples);
    }

    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      outputBuffer.addFrom(ch, startSample, tempBuffer, ch, 0, numSamples,
                           layer.volumeGain);
    }
  }
}

// CORE CONCEPT: Adds a sample sound object to the specified layer.
void LayeredSynth::addSoundToLayer(int layerIndex,
                                   juce::SynthesiserSound::Ptr sound) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].synth.addSound(sound);
  }
}

// CORE CONCEPT: Clears all loaded sounds from a single layer and resets its RR counters.
void LayeredSynth::clearLayer(int layerIndex) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].synth.clearSounds();
    rrCounters[layerIndex].fill(0);
    pedalDownRrCounters[layerIndex] = 0;
    pedalUpRrCounters[layerIndex] = 0;
  }
}

// CORE CONCEPT: Clears all loaded sounds across all synth layers and resets all RR counters.
void LayeredSynth::clearAllLayers() {
  for (int layerIdx = 0; layerIdx < NUM_LAYERS; ++layerIdx) {
    clearLayer(layerIdx);
  }
}

// CORE CONCEPT: Adjusts linear output volume gain for a specific layer.
void LayeredSynth::setLayerVolume(int layerIndex, float gainLinear) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].volumeGain = gainLinear;
  }
}

// CORE CONCEPT: Mutes or unmutes a specific layer.
void LayeredSynth::setLayerMute(int layerIndex, bool isMuted) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].muted = isMuted;
  }
}

// CORE CONCEPT: Sets octave transposition offset (-2 to +2 octaves) for a layer.
void LayeredSynth::setLayerOctaveOffset(int layerIndex, int octaveOffset) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].octaveOffset = juce::jlimit(-2, 2, octaveOffset);
  }
}

// CORE CONCEPT: Retrieves current octave transposition offset for a layer.
int LayeredSynth::getLayerOctaveOffset(int layerIndex) const {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    return layers[layerIndex].octaveOffset;
  }
  return 0;
}

// CORE CONCEPT: Sets hold mode toggle for a layer (blocks new NoteOns, holds existing notes).
void LayeredSynth::setLayerHold(int layerIndex, bool holdActive) {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    layers[layerIndex].holdActive = holdActive;
  }
}

// CORE CONCEPT: Retrieves hold mode state for a layer.
bool LayeredSynth::getLayerHold(int layerIndex) const {
  if (layerIndex >= 0 && layerIndex < NUM_LAYERS) {
    return layers[layerIndex].holdActive;
  }
  return false;
}
