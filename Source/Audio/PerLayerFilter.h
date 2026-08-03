#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

// CORE CONCEPT: Zero-delay feedback (TPT) filter processor with harmonic drive saturation,
// resonance Q control, and switchable filter modes (Lowpass, Bandpass, Highpass).
class PerLayerFilter {
public:
  PerLayerFilter();
  ~PerLayerFilter() = default;

  // CORE CONCEPT: Prepares TPT filter processor for specified sample rate and block size.
  void prepare(double sampleRate, int samplesPerBlock, int numChannels);

  // CORE CONCEPT: Processes audio block in-place with saturation drive and TPT filter.
  void process(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);

  // CORE CONCEPT: Sets filter cutoff frequency in Hz (20 Hz to 20000 Hz).
  void setCutoffFrequency(float cutoffHz);
  float getCutoffFrequency() const { return cutoffFrequencyHz; }

  // CORE CONCEPT: Sets filter resonance Q factor (0.1 to 20.0).
  void setResonance(float newQ);
  float getResonance() const { return resonanceQ; }

  // CORE CONCEPT: Sets harmonic drive pre-filter saturation amount (0.0 to 1.0).
  void setDrive(float newDrive);
  float getDrive() const { return driveAmount; }

  // CORE CONCEPT: Sets filter mode (0 = Lowpass, 1 = Bandpass, 2 = Highpass).
  void setFilterType(int typeIndex);
  int getFilterType() const { return filterTypeIndex; }

private:
  float cutoffFrequencyHz{5000.0f};
  float resonanceQ{0.707f};
  float driveAmount{0.0f};
  int filterTypeIndex{0};

  juce::dsp::StateVariableTPTFilter<float> filter;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerLayerFilter)
};
