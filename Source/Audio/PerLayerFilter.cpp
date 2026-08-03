#include "PerLayerFilter.h"
#include <cmath>

// CORE CONCEPT: Constructor initializing TPT filter type, cutoff, and Q factor.
PerLayerFilter::PerLayerFilter() {
  filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
  filter.setCutoffFrequency(cutoffFrequencyHz);
  filter.setResonance(resonanceQ);
}

// CORE CONCEPT: Prepares DSP filter spec for current sample rate and channel count.
void PerLayerFilter::prepare(double sampleRate, int samplesPerBlock, int numChannels) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = (uint32_t)samplesPerBlock;
  spec.numChannels = (uint32_t)numChannels;

  filter.prepare(spec);
  filter.setCutoffFrequency(cutoffFrequencyHz);
  filter.setResonance(resonanceQ);
}

// CORE CONCEPT: Sets filter cutoff frequency in Hz clamped to valid audible range.
void PerLayerFilter::setCutoffFrequency(float cutoffHz) {
  cutoffFrequencyHz = juce::jlimit(20.0f, 20000.0f, cutoffHz);
  filter.setCutoffFrequency(cutoffFrequencyHz);
}

// CORE CONCEPT: Sets filter resonance Q factor.
void PerLayerFilter::setResonance(float newQ) {
  resonanceQ = juce::jlimit(0.1f, 20.0f, newQ);
  filter.setResonance(resonanceQ);
}

// CORE CONCEPT: Sets pre-filter drive saturation amount (0.0 to 1.0).
void PerLayerFilter::setDrive(float newDrive) {
  driveAmount = juce::jlimit(0.0f, 1.0f, newDrive);
}

// CORE CONCEPT: Sets filter type (0 = Lowpass, 1 = Bandpass, 2 = Highpass).
void PerLayerFilter::setFilterType(int typeIndex) {
  filterTypeIndex = juce::jlimit(0, 2, typeIndex);
  switch (filterTypeIndex) {
    case 1:
      filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
      break;
    case 2:
      filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
      break;
    case 0:
    default:
      filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
      break;
  }
}

// CORE CONCEPT: Audio processing block applying drive saturation (tanh) and TPT filter processing.
void PerLayerFilter::process(juce::AudioBuffer<float> &buffer, int startSample, int numSamples) {
  const int numChannels = buffer.getNumChannels();

  // Apply drive saturation if enabled
  if (driveAmount > 0.001f) {
    float driveGain = 1.0f + driveAmount * 4.0f; // Up to +12dB drive
    for (int ch = 0; ch < numChannels; ++ch) {
      float *ptr = buffer.getWritePointer(ch, startSample);
      for (int i = 0; i < numSamples; ++i) {
        float sample = ptr[i] * driveGain;
        ptr[i] = std::tanh(sample); // Harmonic saturation
      }
    }
  }

  // TPT Filter processing
  juce::dsp::AudioBlock<float> block(buffer.getArrayOfWritePointers(), (size_t)numChannels, (size_t)startSample, (size_t)numSamples);
  juce::dsp::ProcessContextReplacing<float> context(block);
  filter.process(context);
}
