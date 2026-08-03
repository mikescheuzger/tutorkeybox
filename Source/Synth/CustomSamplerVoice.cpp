#include "CustomSamplerVoice.h"

CustomSamplerVoice::CustomSamplerVoice(MidiState *stateToUpdate)
    : midiState(stateToUpdate) {
  // Default envelope parameters (short attack, full sustain, smooth release)
  juce::ADSR::Parameters defaultAmp;
  defaultAmp.attack  = 0.005f; // 5 ms
  defaultAmp.decay   = 0.2f;   // 200 ms
  defaultAmp.sustain = 1.0f;
  defaultAmp.release = 0.3f;   // 300 ms
  ampAdsr.setParameters(defaultAmp);

  juce::ADSR::Parameters defaultFilter;
  defaultFilter.attack  = 0.01f;
  defaultFilter.decay   = 0.3f;
  defaultFilter.sustain = 0.5f;
  defaultFilter.release = 0.4f;
  filterAdsr.setParameters(defaultFilter);
}

bool CustomSamplerVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<const CustomSamplerSound *>(sound) != nullptr;
}

// CORE CONCEPT: Configures amplitude ADSR envelope parameters (attack, decay, sustain, release in ms/ratio).
void CustomSamplerVoice::setAmpAdsrParameters(float attackMs, float decayMs, float sustain, float releaseMs) {
  juce::ADSR::Parameters params;
  params.attack  = juce::jmax(0.001f, attackMs * 0.001f);
  params.decay   = juce::jmax(0.001f, decayMs  * 0.001f);
  params.sustain = juce::jlimit(0.0f, 1.0f, sustain);
  params.release = juce::jmax(0.001f, releaseMs * 0.001f);
  ampAdsr.setParameters(params);
}

// CORE CONCEPT: Configures filter ADSR envelope parameters and modulation depth.
void CustomSamplerVoice::setFilterAdsrParameters(float attackMs, float decayMs, float sustain, float releaseMs, float depth) {
  juce::ADSR::Parameters params;
  params.attack  = juce::jmax(0.001f, attackMs * 0.001f);
  params.decay   = juce::jmax(0.001f, decayMs  * 0.001f);
  params.sustain = juce::jlimit(0.0f, 1.0f, sustain);
  params.release = juce::jmax(0.001f, releaseMs * 0.001f);
  filterAdsr.setParameters(params);
  filterEnvDepth = juce::jlimit(-1.0f, 1.0f, depth);
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
      ampAdsr.setSampleRate(outputSampleRate);
      filterAdsr.setSampleRate(outputSampleRate);
    } else {
      pitchRatio = 1.0;
    }

    sourceSamplePosition = 0.0;
    lgain = velocity;
    rgain = velocity;

    ampAdsr.noteOn();
    filterAdsr.noteOn();

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
    ampAdsr.noteOff();
    filterAdsr.noteOff();
  } else {
    ampAdsr.reset();
    filterAdsr.reset();
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
      ampAdsr.reset();
      filterAdsr.reset();
      break;
    }

    sampleL *= lgain;
    sampleR *= rgain;

    outL[i] += sampleL;
    if (outR != nullptr)
      outR[i] += sampleR;

    sourceSamplePosition += pitchRatio;
  }

  // Apply ADSR amplitude envelope to rendered audio block
  ampAdsr.applyEnvelopeToBuffer(outputBuffer, startSample, numSamples);

  if (!ampAdsr.isActive()) {
    clearCurrentNote();
    ampAdsr.reset();
    filterAdsr.reset();
  }
}
