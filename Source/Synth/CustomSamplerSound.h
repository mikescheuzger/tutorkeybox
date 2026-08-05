#pragma once
#include "SampleHeader.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

/**
 * CustomSamplerSound holding metadata, 150ms RAM attack buffer,
 * and memory-mapped tail pointers for instant 0ms startup and glitch-free
 * sustain.
 */
class CustomSamplerSound : public juce::SynthesiserSound {
public:
  CustomSamplerSound(
      const SampleEntry &entryData,
      const juce::AudioBuffer<float> &attackBufferData,
      const std::vector<const float *> &tailChannelPointersData,
      int totalNumSamples, double sampleRateHz,
      std::shared_ptr<juce::MemoryMappedFile> mappedFileRef = nullptr)
      : entry(entryData), attackBuffer(attackBufferData),
        tailChannelPointers(tailChannelPointersData),
        tailNumSamples(totalNumSamples), sourceSampleRate(sampleRateHz),
        mappedFile(std::move(mappedFileRef)) {}

  ~CustomSamplerSound() override = default;

  bool appliesToNote(int midiNoteNumber) override {
    if (entry.isReleaseSample != 0)
      return false;
    return midiNoteNumber >= entry.keyLow && midiNoteNumber <= entry.keyHigh;
  }

  bool appliesToChannel(int /*midiChannel*/) override { return true; }

  const SampleEntry &getEntry() const { return entry; }
  const juce::AudioBuffer<float> &getAttackBuffer() const {
    return attackBuffer;
  }
  const float *const *getTailChannelPointers() const {
    return tailChannelPointers.data();
  }
  int getTailNumSamples() const { return tailNumSamples; }
  int getNumChannels() const { return (int)entry.numChannels; }
  double getSourceSampleRate() const { return sourceSampleRate; }

  bool appliesToVelocity(int midiVelocity) const {
    return midiVelocity >= entry.velLow && midiVelocity <= entry.velHigh;
  }

private:
  SampleEntry entry;
  juce::AudioBuffer<float> attackBuffer;
  std::vector<const float *> tailChannelPointers;
  int tailNumSamples{0};
  double sourceSampleRate{44100.0};
  std::shared_ptr<juce::MemoryMappedFile> mappedFile;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomSamplerSound)
};
