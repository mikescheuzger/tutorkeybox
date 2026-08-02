#pragma once
#include "SampleHeader.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <memory>

/**
 * CustomSamplerSound holding metadata, 150ms RAM attack buffer,
 * and decoded float tail buffer for 100% glitch-free full-length sustain.
 */
class CustomSamplerSound : public juce::SynthesiserSound {
public:
  CustomSamplerSound(const SampleEntry &entryData,
                     const juce::AudioBuffer<float> &attackBufferData,
                     const juce::AudioBuffer<float> &tailBufferData,
                     double sampleRateHz)
      : entry(entryData), attackBuffer(attackBufferData),
        tailBuffer(tailBufferData), sourceSampleRate(sampleRateHz) {}

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
  const juce::AudioBuffer<float> &getTailBuffer() const { return tailBuffer; }
  double getSourceSampleRate() const { return sourceSampleRate; }

  bool appliesToVelocity(int midiVelocity) const {
    return midiVelocity >= entry.velLow && midiVelocity <= entry.velHigh;
  }

private:
  SampleEntry entry;
  juce::AudioBuffer<float> attackBuffer;
  juce::AudioBuffer<float> tailBuffer;
  double sourceSampleRate;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomSamplerSound)
};
