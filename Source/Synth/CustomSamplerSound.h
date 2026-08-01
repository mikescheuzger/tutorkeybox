#pragma once
#include "SampleHeader.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

// Custom Sampler Sound holding sample metadata & attack RAM buffer

class CustomSamplerSound : public juce::SynthesiserSound {

public:
  CustomSamplerSound(const SampleEntry &entryData,
                     juce::AudioBuffer<float> attackRamData, double sampleRate)
      : entry(entryData), attackBuffer(std::move(attackRamData)),
        sourceSampleRate(sampleRate) {}

  ~CustomSamplerSound() override = default;

  // juce::SynthesiserSound Interface

  // true if this sample covers the pressed MIDI note
  bool appliesToNote(int midiNoteNumber) override {
    return midiNoteNumber >= entry.keyLow && midiNoteNumber <= entry.keyHigh;
  }

  // true for all MIDI channels (1-16)
  bool appliesToChannel(int /*midiChannel*/) override { return true; }

  // true if velocity within this sample's velocity zone
  bool appliesToVelocity(int velocity) const {
    return velocity >= entry.velLow && velocity <= entry.velHigh;
  }

  // Accessors
  const SampleEntry &getEntry() const { return entry; }
  const juce::AudioBuffer<float> &getAttackBuffer() const {
    return attackBuffer;
  }
  double getSourceSampleRate() const { return sourceSampleRate; }
  bool isReleaseSample() const { return entry.isReleaseSample != 0; }
  uint8_t getLayerIndex() const { return entry.layerIndex; }

private:
  SampleEntry entry;
  juce::AudioBuffer<float> attackBuffer; // Attack buffer in RAM
  double sourceSampleRate;               // original sample rate of th WAV

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomSamplerSound)
};
