#include "AudioEngine.h"

AudioEngine::AudioEngine(MidiState &stateToUpdate) : midiState(stateToUpdate) {}

AudioEngine::~AudioEngine() { shutdown(); }

void AudioEngine::initialize() {
  // Initialize default audio output (0 inputs, 2 output channels for stereo
  // speakers)
  deviceManager.initialiseWithDefaultDevices(0, 2);

  // Register this class to receive audio callbacks
  deviceManager.addAudioCallback(this);

  // Automatically enable and listen to all connected MIDI input devices
  auto midiInputs = juce::MidiInput::getAvailableDevices();
  for (const auto &input : midiInputs) {
    deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
    deviceManager.addMidiInputDeviceCallback(input.identifier, this);
  }
}

void AudioEngine::shutdown() {
  // Unregister MIDI callbacks from hardware inputs
  auto midiInputs = juce::MidiInput::getAvailableDevices();
  for (const auto &input : midiInputs) {
    deviceManager.removeMidiInputDeviceCallback(input.identifier, this);
  }

  // Stop audio processing callback
  deviceManager.removeAudioCallback(this);
}

void AudioEngine::handleIncomingMidiMessage(juce::MidiInput * /*source*/,
                                            const juce::MidiMessage &message) {
  if (message.isNoteOn()) {
    midiState.noteOn(message.getNoteNumber(), message.getFloatVelocity());
  } else if (message.isNoteOff()) {
    midiState.noteOff(message.getNoteNumber());
  } else if (message.isController() && message.getControllerNumber() == 64) {
    // 64 weil MIDI CC 64 Sustain Pedal ist
    bool pedalDown = (message.getControllerValue() >= 64);
    midiState.setSustainPedal(pedalDown);
  }

  // Queue MIDI message safely for real-time sound engine
  const juce::ScopedLock sl(midiLock);
  incomingMidiBuffer.addEvent(message, 0);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice *device) {
  // Will be used later when we prepare our sound sample engine
  if (device != nullptr) {
    synth.prepareToPlay(device->getCurrentSampleRate(),
                        device->getCurrentBufferSizeSamples());
  }
}

void AudioEngine::audioDeviceStopped() {
  // Cleanup when audio hardware stops
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float *const * /*inputChannelData*/, int /*numInputChannels*/,
    float *const *outputChannelData, int numOutputChannels, int numSamples,
    const juce::AudioIODeviceCallbackContext & /*context*/) {
  // 1. Wrap raw speaker channel pointers into JUCE AudioBuffer
  juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels,
                                  numSamples);
  buffer.clear();
  // 2. Safely extract queued real-time MIDI messages
  juce::MidiBuffer midiMessagesToProcess;
  {
    const juce::ScopedLock sl(midiLock);
    midiMessagesToProcess.addEvents(incomingMidiBuffer, 0, numSamples, 0);
    incomingMidiBuffer.clear();
  }
  // 3. Render 4-layer polyphonic synth audio directly to speakers!
  synth.renderNextBlock(buffer, midiMessagesToProcess, 0, numSamples);
}
