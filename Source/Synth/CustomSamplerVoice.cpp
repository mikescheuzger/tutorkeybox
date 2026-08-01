#include "CustomSamplerVoice.h"

CustomSamplerVoice::CustomSamplerVoice() {}

bool CustomSamplerVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<const CustomSamplerSound *>(sound) != nullptr;
}
void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity,
                                   juce::SynthesiserSound *sound,
                                   int /*currentPitchWheelPosition*/)

{
  if (auto *samplerSound = dynamic_cast<const CustomSamplerSound *>(sound)) {
    // Calculate pitch-shifting ratio relative to root note
    double noteHz = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    double rootHz = juce::MidiMessage::getMidiNoteInHertz(
        samplerSound->getEntry().rootNote);
    double outputSampleRate = getSampleRate();
    if (outputSampleRate > 0.0) {
      pitchRatio = (noteHz / rootHz) *
                   (samplerSound->getSourceSampleRate() / outputSampleRate);
    } else {
      pitchRatio = 1.0;
    }
    sourceSamplePosition = 0.0;
    lgain = velocity;
    rgain = velocity;
    isReleasing = false;
    releaseFactor = 1.0f;
  } else {
    jassertfalse; // Sound type mismatch
  }
}
void CustomSamplerVoice::stopNote(float /*velocity*/, bool allowTailOff) {
  if (allowTailOff) {
    isReleasing = true; // Start smooth release envelope fade-out
  } else {
    clearCurrentNote();
    sourceSamplePosition = 0.0;
  }
}
void CustomSamplerVoice::pitchWheelMoved(int /*newPitchWheelValue*/) {}
void CustomSamplerVoice::controllerMoved(int /*controllerNumber*/,
                                         int /*newControllerValue*/) {}
void CustomSamplerVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                         int startSample, int numSamples) {
  auto *currentSound = dynamic_cast<const CustomSamplerSound *>(
      getCurrentlyPlayingSound().get());
  if (currentSound == nullptr)
    return;
  const auto &attackBuffer = currentSound->getAttackBuffer();
  const int attackNumSamples = attackBuffer.getNumSamples();
  if (attackNumSamples == 0) {
    clearCurrentNote();
    return;
  }
  const float *const *inChannels = attackBuffer.getArrayOfReadPointers();
  float *outL = outputBuffer.getWritePointer(0, startSample);
  float *outR = outputBuffer.getNumChannels() > 1
                    ? outputBuffer.getWritePointer(1, startSample)
                    : nullptr;
  for (int i = 0; i < numSamples; ++i) {
    int posInt = (int)sourceSamplePosition;
    float alpha = (float)(sourceSamplePosition - posInt);
    if (posInt >= attackNumSamples - 1) {
      // Reached end of attack buffer in RAM
      clearCurrentNote();
      break;
    }
    // Linear Interpolation between adjacent audio samples for clean
    // pitch-shifting
    float sampleL = inChannels[0][posInt] +
                    alpha * (inChannels[0][posInt + 1] - inChannels[0][posInt]);
    float sampleR =
        (attackBuffer.getNumChannels() > 1)
            ? inChannels[1][posInt] +
                  alpha * (inChannels[1][posInt + 1] - inChannels[1][posInt])
            : sampleL;
    // Apply release envelope fade-out
    if (isReleasing) {
      releaseFactor *= 0.992f; // Exponential smooth release decay
      if (releaseFactor < 0.001f) {
        clearCurrentNote();
        break;
      }
    }
    // Scale by velocity gain & release envelope
    float currentGain = releaseFactor;
    sampleL *= (lgain * currentGain);
    sampleR *= (rgain * currentGain);
    // Mix output into audio buffer
    outL[i] += sampleL;
    if (outR != nullptr)
      outR[i] += sampleR;
    sourceSamplePosition += pitchRatio;
  }
}
