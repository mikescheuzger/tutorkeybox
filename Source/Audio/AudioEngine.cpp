#include "AudioEngine.h"
AudioEngine::AudioEngine(MidiState &stateToUpdate)
    : midiState(stateToUpdate), synth(stateToUpdate) {}
AudioEngine::~AudioEngine() { shutdown(); }

void AudioEngine::initialize() {
  // 1. Initialize hardware soundcard
  deviceManager.initialiseWithDefaultDevices(0, 2);
  // 2. Set low latency buffer size (prefer 128/256 samples for ALSA compatibility)
  juce::AudioDeviceManager::AudioDeviceSetup setup;
  deviceManager.getAudioDeviceSetup(setup);
  if (setup.bufferSize == 0 || setup.bufferSize > 512) {
    setup.bufferSize = 256;
  } else {
    setup.bufferSize = 128;
  }
  deviceManager.setAudioDeviceSetup(setup, true);
  // 3. Register audio callback
  deviceManager.addAudioCallback(this);
  // 4. Automatically enable and listen to EVERY connected MIDI input device!
  auto midiInputs = juce::MidiInput::getAvailableDevices();
  for (const auto &input : midiInputs) {
    deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
    deviceManager.addMidiInputDeviceCallback(input.identifier, this);
    juce::Logger::writeToLog("Connected MIDI Input Device: " + input.name);
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
    juce::Logger::writeToLog("MIDI NOTE ON: " +
                             juce::String(message.getNoteNumber()));
    midiState.noteOn(message.getNoteNumber(), message.getFloatVelocity());
  } else if (message.isNoteOff()) {
    midiState.noteOff(message.getNoteNumber());
  } else if (message.isController()) {
    if (message.getControllerNumber() == 64) {
      bool pedalDown = (message.getControllerValue() >= 64);
      midiState.setSustainPedal(pedalDown);
    }
    midiRouter.processMidiCc(message);
  }
  // Queue MIDI message safely for real-time sound engine
  const juce::ScopedLock sl(midiLock);
  incomingMidiBuffer.addEvent(message, 0);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice *device) {
  if (device != nullptr) {
    synth.prepareToPlay(device->getCurrentSampleRate(),
                        device->getCurrentBufferSizeSamples());
    reverbEngine.prepare(device->getCurrentSampleRate(),
                         device->getCurrentBufferSizeSamples(),
                         device->getActiveOutputChannels().countNumberOfSetBits());
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

  // 4. Render post-fader convolution reverb effect
  reverbEngine.process(buffer);
}
