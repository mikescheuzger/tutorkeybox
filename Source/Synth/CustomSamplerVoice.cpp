#include "CustomSamplerVoice.h"

CustomSamplerVoice::CustomSamplerVoice() {}

bool CustomSamplerVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<const CustomSamplerSound *>(sound) != nullptr;
}

void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity,
                                   juce::SynthesiserSound *sound,
                                   int /*currentPitchWheelPosition*/) {
  if (auto *samplerSound = dynamic_cast<const CustomSamplerSound *>(sound)) {
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
    attackRamp = 0.0f;

    // Create Zero-Copy Memory-Mapped Reader for the sample tail
    tailReader.reset();
    auto mappedFile = samplerSound->getMappedFile();
    const auto &entry = samplerSound->getEntry();

    if (mappedFile != nullptr && mappedFile->getData() != nullptr &&
        entry.wavDataSize > 0) {
      const char *wavBytes =
          static_cast<const char *>(mappedFile->getData()) + entry.fileOffset;
      auto memStream = std::make_unique<juce::MemoryInputStream>(
          wavBytes, entry.wavDataSize, false);
      tailReader.reset(wavFormat.createReaderFor(memStream.release(), true));
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
      // 1. PLAY FROM 150ms RAM ATTACK BUFFER
      sampleL = inChannels[0][posInt] +
                alpha * (inChannels[0][posInt + 1] - inChannels[0][posInt]);
      sampleR =
          (attackBuffer.getNumChannels() > 1)
              ? inChannels[1][posInt] +
                    alpha * (inChannels[1][posInt + 1] - inChannels[1][posInt])
              : sampleL;
    } else if (tailReader != nullptr &&
               posInt < tailReader->lengthInSamples - 1) {
      // 2. SEAMLESS ZERO-COPY TAIL STREAMING (Memory-Mapped Pointer)
      float tempL[2] = {0.0f, 0.0f};
      float tempR[2] = {0.0f, 0.0f};
      float *destChannels[2] = {tempL, tempR};
      juce::AudioBuffer<float> tempBuf(destChannels, 2, 2);

      tailReader->read(&tempBuf, 0, 2, posInt, true, true);
      sampleL = tempL[0] + alpha * (tempL[1] - tempL[0]);
      sampleR = (tailReader->numChannels > 1)
                    ? (tempR[0] + alpha * (tempR[1] - tempR[0]))
                    : sampleL;
    } else {
      clearCurrentNote();
      break;
    }

    // Anti-click 2ms smooth fade-in ramp
    if (attackRamp < 1.0f) {
      attackRamp += 0.005f;
    }

    // Smooth release envelope decay
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
