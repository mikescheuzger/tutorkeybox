#pragma once
#include "CustomSamplerSound.h"
#include <juce_audio_basics/juce_audio_basics.h>

class CustomSamplerVoice : public juce::SynthesiserVoice {
public:
  CustomSamplerVoice();
  ~CustomSamplerVoice() override = default;
  // --- juce::SynthesiserVoice Interface ---

  // Returns true if this voice can play the given sound
  bool canPlaySound(juce::SynthesiserSound *sound) override;
  // Triggered when a MIDI key is pressed (Note On)
  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override;
  // Triggered when a MIDI key is released (Note Off)
  void stopNote(float velocity, bool allowTailOff) override;
  // Pitch wheel change
  void pitchWheelMoved(int newPitchWheelValue) override;
  // Controller message (e.g. Sustain pedal CC 64)
  void controllerMoved(int controllerNumber, int newControllerValue) override;
  // Real-Time Audio Renderer Loop
  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override;

private:
  double pitchRatio{0.0};
  double sourceSamplePosition{0.0};
  float lgain{0.0f};
  float rgain{0.0f};
  // ADSR release envelope tracking
  float releaseFactor{1.0f};
  bool isReleasing{false};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomSamplerVoice)
};
