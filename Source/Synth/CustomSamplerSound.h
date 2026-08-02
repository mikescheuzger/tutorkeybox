#pragma once
#include "SampleHeader.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <memory>

/**
 * CustomSamplerSound holding metadata, 150ms RAM attack buffer,
 * and a memory-mapped file reference for zero-copy disk tail streaming.
 */
class CustomSamplerSound : public juce::SynthesiserSound {
public:
  CustomSamplerSound(
      const SampleEntry &entryData,
      const juce::AudioBuffer<float> &attackBufferData, double sampleRateHz,
      std::shared_ptr<juce::MemoryMappedFile> memoryMap = nullptr)
      : entry(entryData), attackBuffer(attackBufferData),
        sourceSampleRate(sampleRateHz), mappedFile(memoryMap) {}

  ~CustomSamplerSound() override = default;

  bool appliesToNote(int midiNoteNumber) override {
    // Release samples NEVER play on Note On!
    if (entry.isReleaseSample != 0)
      return false;
    return midiNoteNumber >= entry.keyLow && midiNoteNumber <= entry.keyHigh;
  }

  bool appliesToChannel(int /*midiChannel*/) override { return true; }

  const SampleEntry &getEntry() const { return entry; }
  const juce::AudioBuffer<float> &getAttackBuffer() const {
    return attackBuffer;
  }
  double getSourceSampleRate() const { return sourceSampleRate; }
  std::shared_ptr<juce::MemoryMappedFile> getMappedFile() const {
    return mappedFile;
  }

private:
  SampleEntry entry;
  juce::AudioBuffer<float> attackBuffer;
  double sourceSampleRate;
  std::shared_ptr<juce::MemoryMappedFile> mappedFile;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomSamplerSound)
};
