#pragma once
#include "../Core/MidiState.h"
#include <juce_audio_devices/juce_audio_devices.h>

class AudioEngine : public juce::AudioIODeviceCallback,
                    public juce::MidiInputCallback

{
public:
  explicit AudioEngine(MidiState &stateToUpdate);
  ~AudioEngine() override;

  void initialize();
  void shutdown();

  juce::AudioDeviceManager &getDeviceManager() { return deviceManager; }

  void handleIncomingMidiMessage(juce::MidiInput *source,
                                 const juce::MidiMessage &message) override;

  void audioDeviceIOCallbackWithContext(
      const float *const *inputChannelData, int numInputChannels,
      float *const *outputChannelData, int numOutputChannels, int numSamples,
      const juce::AudioIODeviceCallbackContext &context) override;

  void audioDeviceAboutToStart(juce::AudioIODevice *device) override;
  void audioDeviceStopped() override;

private:
  MidiState &midiState;
  juce::AudioDeviceManager deviceManager;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine);
};
