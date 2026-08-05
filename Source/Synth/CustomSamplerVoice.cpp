#include "CustomSamplerVoice.h"

CustomSamplerVoice::CustomSamplerVoice(MidiState *stateToUpdate)
    : midiState(stateToUpdate) {}

bool CustomSamplerVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<const CustomSamplerSound *>(sound) != nullptr;
}

void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity,
                                   juce::SynthesiserSound *sound,
                                   int /*currentPitchWheelPosition*/) {
  activeSound = static_cast<const CustomSamplerSound *>(sound);
  if (activeSound != nullptr) {
    int velInt = juce::roundToInt(velocity * 127.0f);
    if (!activeSound->appliesToVelocity(velInt)) {
      clearCurrentNote();
      activeSound = nullptr;
      return;
    }

    double noteHz = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    double rootHz =
        juce::MidiMessage::getMidiNoteInHertz(activeSound->getEntry().rootNote);
    double outputSampleRate = getSampleRate();

    if (outputSampleRate > 0.0) {
      pitchRatio = (noteHz / rootHz) *
                   (activeSound->getSourceSampleRate() / outputSampleRate);
    } else {
      pitchRatio = 1.0;
    }

    sourceSamplePosition = 0.0;
    lgain = 1.0f;
    rgain = 1.0f;
    isReleasing = false;
    releaseFactor = 1.0f;
    attackRamp = 0.0f;

    const auto &entry = activeSound->getEntry();
    if (midiState != nullptr) {
      midiState->updateSampleInspector(entry.name, entry.rootNote, entry.keyLow,
                                       entry.keyHigh, entry.velLow,
                                       entry.velHigh);
    }
  } else {
    clearCurrentNote();
  }
}

void CustomSamplerVoice::stopNote(float /*velocity*/, bool allowTailOff) {
  if (allowTailOff) {
    isReleasing = true;
  } else {
    clearCurrentNote();
    sourceSamplePosition = 0.0;
    activeSound = nullptr;
  }
}

void CustomSamplerVoice::pitchWheelMoved(int /*newPitchWheelValue*/) {}
void CustomSamplerVoice::controllerMoved(int /*controllerNumber*/,
                                         int /*newControllerValue*/) {}

void CustomSamplerVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                         int startSample, int numSamples) {
  if (activeSound == nullptr)
    return;

  const auto &attackBuffer = activeSound->getAttackBuffer();
  const int attackNumSamples = attackBuffer.getNumSamples();
  const float *const *attackChannels = attackBuffer.getArrayOfReadPointers();

  const float *const *tailChannels = activeSound->getTailChannelPointers();
  const int tailNumSamples = activeSound->getTailNumSamples();

  float *outL = outputBuffer.getWritePointer(0, startSample);
  float *outR = outputBuffer.getNumChannels() > 1
                    ? outputBuffer.getWritePointer(1, startSample)
                    : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    int posInt = (int)sourceSamplePosition;
    float alpha = (float)(sourceSamplePosition - posInt);
    float sampleL = 0.0f;
    float sampleR = 0.0f;

    if (posInt < attackNumSamples - 1) {
      // Phase 1: Render from RAM Attack Buffer (0ms latency, zero glitch risk)
      sampleL =
          attackChannels[0][posInt] +
          alpha * (attackChannels[0][posInt + 1] - attackChannels[0][posInt]);
      sampleR = (attackBuffer.getNumChannels() > 1)
                    ? attackChannels[1][posInt] +
                          alpha * (attackChannels[1][posInt + 1] -
                                   attackChannels[1][posInt])
                    : sampleL;
    } else if (posInt < tailNumSamples - 1) {
      // Phase 2: Seamlessly stream Tail from Memory-Mapped File (mmap)
      sampleL = tailChannels[0][posInt] +
                alpha * (tailChannels[0][posInt + 1] - tailChannels[0][posInt]);
      sampleR =
          (activeSound->getNumChannels() > 1)
              ? tailChannels[1][posInt] + alpha * (tailChannels[1][posInt + 1] -
                                                   tailChannels[1][posInt])
              : sampleL;
    } else {
      clearCurrentNote();
      activeSound = nullptr;
      break;
    }

    if (attackRamp < 1.0f) {
      attackRamp += 0.05f;
    }

    if (isReleasing) {
      releaseFactor *= 0.9992f;
      if (releaseFactor < 0.001f) {
        clearCurrentNote();
        activeSound = nullptr;
        break;
      }
    }

    float currentGain = releaseFactor * attackRamp;
    sampleL *= (lgain * currentGain);
    sampleR *= (rgain * currentGain);

    outL[i] += sampleL;
    if (outR != nullptr)
      outR[i] += sampleR;

    sourceSamplePosition += pitchRatio;
  }
}
