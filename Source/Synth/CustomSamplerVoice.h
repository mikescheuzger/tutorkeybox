#pragma once
#include "../Core/MidiState.h"
#include "CustomSamplerSound.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

class CustomSamplerVoice : public juce::SynthesiserVoice {
public:
  explicit CustomSamplerVoice(MidiState *stateToUpdate = nullptr);

  ~CustomSamplerVoice() override = default;

  bool canPlaySound(juce::SynthesiserSound *sound) override;
  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override;
  void stopNote(float velocity, bool allowTailOff) override;
  void pitchWheelMoved(int newPitchWheelValue) override;
  void controllerMoved(int controllerNumber, int newControllerValue) override;
  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override;

private:
  MidiState *midiState{nullptr};

  double pitchRatio{0.0};
  double sourceSamplePosition{0.0};
  float lgain{0.0f};
  float rgain{0.0f};

  // ADSR release envelope tracking

  float attackRamp{0.0f};
  float releaseFactor{1.0f};
  bool isReleasing{false};

  // Disk Tail Reader
  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::AudioFormatReader> tailReader;
  juce::AudioBuffer<float> tailBlockBuffer{2,
                                           512}; // Small buffer for disk reads

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomSamplerVoice)
};
