#include "CustomSamplerVoice.h"

CustomSamplerVoice::CustomSamplerVoice(MidiState *stateToUpdate)
    : midiState(stateToUpdate) {}

bool CustomSamplerVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<const CustomSamplerSound *>(sound) != nullptr;
}

void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity,
                                   juce::SynthesiserSound *sound,
                                   int /*currentPitchWheelPosition*/) {
  if (auto *samplerSound = dynamic_cast<const CustomSamplerSound *>(sound)) {
    int velInt = juce::roundToInt(velocity * 127.0f);
    if (!samplerSound->appliesToVelocity(velInt)) {
      clearCurrentNote();
      return;
    }

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
    lgain = 1.0f;
    rgain = 1.0f;
    isReleasing = false;
    releaseFactor = 1.0f;
    attackRamp = 0.0f;

    const auto &entry = samplerSound->getEntry();
    if (midiState != nullptr) {
      midiState->updateSampleInspector(entry.name, entry.rootNote, entry.keyLow,
                                       entry.keyHigh, entry.velLow,
                                       entry.velHigh);
    }
  } else {
    jassertfalse;
  }
}

void CustomSamplerVoice::stopNote(float /*velocity*/, bool allowTailOff) {
  if (allowTailOff) {
    isReleasing = true;
  } else {
    clearCurrentNote();
    sourceSamplePosition = 0.0;
    tailReader.reset();
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
  const float *const *inChannels = attackBuffer.getArrayOfReadPointers();

  const auto &tailBuffer = currentSound->getTailBuffer();
  const int tailNumSamples = tailBuffer.getNumSamples();
  const float *const *tailChannels = tailBuffer.getArrayOfReadPointers();

  float *outL = outputBuffer.getWritePointer(0, startSample);
  float *outR = outputBuffer.getNumChannels() > 1
                    ? outputBuffer.getWritePointer(1, startSample)
                    : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    int posInt = (int)sourceSamplePosition;
    float alpha = (float)(sourceSamplePosition - posInt);
    float sampleL = 0.0f;
    float sampleR = 0.0f;

    if (posInt < tailNumSamples - 1) {
      sampleL = tailChannels[0][posInt] +
                alpha * (tailChannels[0][posInt + 1] - tailChannels[0][posInt]);
      sampleR =
          (tailBuffer.getNumChannels() > 1)
              ? tailChannels[1][posInt] + alpha * (tailChannels[1][posInt + 1] -
                                                   tailChannels[1][posInt])
              : sampleL;
    } else {
      clearCurrentNote();
      break;
    }

    if (attackRamp < 1.0f) {
      attackRamp += 0.05f;
    }

    if (isReleasing) {
      releaseFactor *= 0.9992f;
      if (releaseFactor < 0.001f) {
        clearCurrentNote();
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
