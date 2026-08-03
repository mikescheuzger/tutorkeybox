#include "ConvolutionReverbEngine.h"

// CORE CONCEPT: Constructor setting up lowpass damping filter type.
ConvolutionReverbEngine::ConvolutionReverbEngine() {
  dampingFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
  dampingFilter.setCutoffFrequency(dampingCutoffHz);
}

// CORE CONCEPT: Prepares convolution engine, pre-delay line, and damping filter for audio processing.
void ConvolutionReverbEngine::prepare(double sampleRate, int samplesPerBlock, int numChannels) {
  currentSampleRate = sampleRate;

  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = (uint32_t)samplesPerBlock;
  spec.numChannels = (uint32_t)numChannels;

  convolution.prepare(spec);
  preDelayLine.prepare(spec);
  preDelayLine.setMaximumDelayInSamples((int)(sampleRate * 0.2)); // 200 ms max pre-delay
  preDelayLine.setDelay((float)(preDelayMs * 0.001 * sampleRate));

  dampingFilter.prepare(spec);
  dampingFilter.setCutoffFrequency(dampingCutoffHz);

  wetBuffer.setSize(numChannels, samplesPerBlock);
}

// CORE CONCEPT: Asynchronously loads impulse response WAV file into the convolution DSP engine.
bool ConvolutionReverbEngine::loadImpulseResponse(const juce::File &irFile) {
  if (!irFile.existsAsFile())
    return false;

  currentIrFile = irFile;
  convolution.loadImpulseResponse(irFile,
                                  juce::dsp::Convolution::Stereo::yes,
                                  juce::dsp::Convolution::Trim::yes,
                                  0);
  juce::Logger::writeToLog("ConvolutionReverbEngine: Loaded IR -> " + irFile.getFileName());
  return true;
}

// CORE CONCEPT: Sets dry/wet mix ratio (0.0 to 1.0).
void ConvolutionReverbEngine::setMix(float newMix) {
  mixRatio = juce::jlimit(0.0f, 1.0f, newMix);
}

// CORE CONCEPT: Sets pre-delay time in milliseconds and updates delay line samples.
void ConvolutionReverbEngine::setPreDelayMs(float delayMs) {
  preDelayMs = juce::jlimit(0.0f, 100.0f, delayMs);
  if (currentSampleRate > 0.0) {
    preDelayLine.setDelay((float)(preDelayMs * 0.001 * currentSampleRate));
  }
}

// CORE CONCEPT: Sets post-convolution high-frequency damping lowpass cutoff frequency in Hz.
void ConvolutionReverbEngine::setDampingHz(float dampingHz) {
  dampingCutoffHz = juce::jlimit(500.0f, 20000.0f, dampingHz);
  dampingFilter.setCutoffFrequency(dampingCutoffHz);
}

// CORE CONCEPT: Main real-time audio block processing: renders wet reverb path with pre-delay,
// convolution, and damping lowpass filter, then blends wet/dry mix into output buffer.
void ConvolutionReverbEngine::process(juce::AudioBuffer<float> &buffer) {
  if (mixRatio <= 0.001f)
    return;

  const int numSamples = buffer.getNumSamples();
  const int numChannels = buffer.getNumChannels();

  wetBuffer.setSize(numChannels, numSamples, false, false, true);
  for (int ch = 0; ch < numChannels; ++ch) {
    wetBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
  }

  juce::dsp::AudioBlock<float> block(wetBuffer);
  juce::dsp::ProcessContextReplacing<float> context(block);

  // 1. Pre-delay
  preDelayLine.process(context);

  // 2. Convolution IR
  convolution.process(context);

  // 3. Damping LPF
  dampingFilter.process(context);

  // 4. Blend dry/wet into original buffer
  float dryGain = std::cos(mixRatio * juce::MathConstants<float>::halfPi);
  float wetGain = std::sin(mixRatio * juce::MathConstants<float>::halfPi);

  for (int ch = 0; ch < numChannels; ++ch) {
    buffer.applyGain(ch, 0, numSamples, dryGain);
    buffer.addFrom(ch, 0, wetBuffer, ch, 0, numSamples, wetGain);
  }
}
