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

  // CORE CONCEPT: Returns true only for Normal and WithPedal samples — these
  // are the two types that respond to a NoteOn event within a MIDI pitch range.
  // Release, pedal-noise, and other types are dispatched separately.
  bool appliesToNote(int midiNoteNumber) override {
    if (entry.sampleType != SampleType::Normal &&
        entry.sampleType != SampleType::WithPedal)
      return false;
    return midiNoteNumber >= entry.keyLow && midiNoteNumber <= entry.keyHigh;
  }

  // CORE CONCEPT: Evaluates whether this sample sound matches a specific event type,
  // MIDI note number, and velocity, allowing exact routing for release tails and pedal noises.
  bool appliesToEvent(SampleType targetType, int midiNoteNumber, int midiVelocity) const {
    if (entry.sampleType != targetType)
      return false;
    if (targetType == SampleType::PedalDown || targetType == SampleType::PedalUp)
      return true;
    bool noteMatch = (midiNoteNumber >= entry.keyLow && midiNoteNumber <= entry.keyHigh) ||
                     (entry.rootNote == midiNoteNumber);
    bool velMatch  = (midiVelocity >= entry.velLow && midiVelocity <= entry.velHigh);
    return noteMatch && velMatch;
  }

  bool appliesToChannel(int /*midiChannel*/) override { return true; }

  const SampleEntry& getEntry() const { return entry; }
  const juce::AudioBuffer<float>& getAttackBuffer() const { return attackBuffer; }
  const juce::AudioBuffer<float>& getTailBuffer()   const { return tailBuffer; }
  double      getSourceSampleRate() const { return sourceSampleRate; }
  SampleType  getSampleType()       const { return entry.sampleType; }
  DynamicLayer getDynamic()         const { return entry.dynamic; }
  uint8_t     getRoundRobinIndex()  const { return entry.roundRobinIndex; }
  uint8_t     getRoundRobinTotal()  const { return entry.roundRobinTotal; }

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
