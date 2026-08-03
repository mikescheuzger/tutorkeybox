#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

// CORE CONCEPT: High-quality impulse response convolution reverb engine with dry/wet mixing,
// pre-delay buffer, and high-frequency damping lowpass filter.
class ConvolutionReverbEngine {
public:
  ConvolutionReverbEngine();
  ~ConvolutionReverbEngine() = default;

  // CORE CONCEPT: Prepares DSP processors (convolution, damping filter, pre-delay) for the specified sample rate and block size.
  void prepare(double sampleRate, int samplesPerBlock, int numChannels);

  // CORE CONCEPT: Loads an impulse response WAV file asynchronously into the convolution engine.
  bool loadImpulseResponse(const juce::File &irFile);

  // CORE CONCEPT: Processes stereo audio block replacing output with dry/wet reverb mix.
  void process(juce::AudioBuffer<float> &buffer);

  // CORE CONCEPT: Sets dry/wet mix ratio (0.0 = completely dry, 1.0 = completely wet).
  void setMix(float newMix);
  float getMix() const { return mixRatio; }

  // CORE CONCEPT: Sets pre-delay time in milliseconds (0 to 100 ms).
  void setPreDelayMs(float delayMs);
  float getPreDelayMs() const { return preDelayMs; }

  // CORE CONCEPT: Sets post-convolution high-frequency damping cutoff in Hz (1000 Hz to 20000 Hz).
  void setDampingHz(float dampingHz);
  float getDampingHz() const { return dampingCutoffHz; }

  // CORE CONCEPT: Returns path of currently loaded IR file.
  juce::String getLoadedIrPath() const { return currentIrFile.getFullPathName(); }

private:
  double currentSampleRate{44100.0};
  float mixRatio{0.3f};
  float preDelayMs{10.0f};
  float dampingCutoffHz{8000.0f};
  juce::File currentIrFile;

  juce::dsp::Convolution convolution;
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayLine{44100};
  juce::dsp::StateVariableTPTFilter<float> dampingFilter;

  juce::AudioBuffer<float> wetBuffer;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConvolutionReverbEngine)
};
