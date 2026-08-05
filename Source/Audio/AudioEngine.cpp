#include "AudioEngine.h"

AudioEngine::AudioEngine(MidiState &stateToUpdate)
    : midiState(stateToUpdate), synth(stateToUpdate) {}

AudioEngine::~AudioEngine() { shutdown(); }

bool AudioEngine::initialize() {
  audioDeviceOK = false;
  juce::String preferredDeviceType = "ALSA";
  const auto &availableTypes = deviceManager.getAvailableDeviceTypes();

  for (auto *type : availableTypes) {
    if (type->getTypeName().equalsIgnoreCase(preferredDeviceType)) {
      deviceManager.setCurrentAudioDeviceType(preferredDeviceType, true);
      break;
    }
  }

  // Scan available output devices and select direct hardware (preferring hw: or
  // USB/DAC)
  auto *currentType = deviceManager.getCurrentDeviceTypeObject();
  juce::String bestOutputDevice = "";
  if (currentType != nullptr) {
    auto outputDevices = currentType->getDeviceNames(false);
    int highestScore = -1;
    for (const auto &devName : outputDevices) {
      int score = 1;
      if (devName.containsIgnoreCase("USB") ||
          devName.containsIgnoreCase("DAC") ||
          devName.containsIgnoreCase("iO") ||
          devName.containsIgnoreCase("Focusrite") ||
          devName.containsIgnoreCase("500R8")) {
        score = 3; // Top priority: Dedicated Hardware Audio Interface / DAC
      } else if (devName.startsWithIgnoreCase("hw:") ||
                 devName.startsWithIgnoreCase("plughw:")) {
        score = 2; // Direct ALSA Hardware (bypass PulseAudio/PipeWire)
      } else if (devName.containsIgnoreCase("hdmi") ||
                 devName.containsIgnoreCase("bcm2835")) {
        score = 0; // Avoid onboard HDMI audio
      }
      if (score > highestScore) {
        highestScore = score;
        bestOutputDevice = devName;
      }
    }
  }

  // Configure Ultra-Low-Latency Setup: 64 samples @ 48kHz
  juce::AudioDeviceManager::AudioDeviceSetup setup;
  deviceManager.getAudioDeviceSetup(setup);
  setup.outputChannels = 2;
  setup.inputChannels = 0;
  setup.sampleRate = 48000.0;
  setup.bufferSize = 64; // Ultra-low latency (64 samples ≈ 1.3ms)

  if (bestOutputDevice.isNotEmpty()) {
    setup.outputDeviceName = bestOutputDevice;
  }

  juce::String error = deviceManager.setAudioDeviceSetup(setup, true);
  if (error.isNotEmpty()) {
    juce::Logger::writeToLog(
        "AudioEngine Error: Failed to open hardware audio device - " + error);
    deviceManager.initialiseWithDefaultDevices(0, 2);
  }

  auto *currentDevice = deviceManager.getCurrentAudioDevice();
  if (currentDevice != nullptr && currentDevice->isPlaying()) {
    audioDeviceOK = true;
    juce::Logger::writeToLog(
        "AudioEngine Success: Active Audio Output -> " +
        currentDevice->getName() + " [" +
        juce::String(currentDevice->getCurrentSampleRate()) + " Hz, " +
        juce::String(currentDevice->getCurrentBufferSizeSamples()) +
        " samples]");
  } else {
    juce::Logger::writeToLog(
        "AudioEngine Warning: No active audio output device opened!");
  }

  deviceManager.removeAudioCallback(this);
  deviceManager.addAudioCallback(this);
  refreshMidiInputs();

  return audioDeviceOK;
}

void AudioEngine::refreshMidiInputs() {
  auto midiInputs = juce::MidiInput::getAvailableDevices();
  for (const auto &input : midiInputs) {
    if (!deviceManager.isMidiInputDeviceEnabled(input.identifier)) {
      deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
      deviceManager.addMidiInputDeviceCallback(input.identifier, this);
      juce::Logger::writeToLog("Connected MIDI Input Device: " + input.name);
    }
  }
}

juce::String AudioEngine::getActiveAudioDeviceName() const {
  auto *device = deviceManager.getCurrentAudioDevice();
  return device != nullptr ? device->getName() : "None";
}

void AudioEngine::shutdown() {
  auto midiInputs = juce::MidiInput::getAvailableDevices();
  for (const auto &input : midiInputs) {
    deviceManager.removeMidiInputDeviceCallback(input.identifier, this);
  }

  deviceManager.removeAudioCallback(this);
}

bool AudioEngine::setBufferSize(int newBufferSize) {
  if (newBufferSize <= 0)
    return false;
  juce::AudioDeviceManager::AudioDeviceSetup setup;
  deviceManager.getAudioDeviceSetup(setup);
  setup.bufferSize = newBufferSize;
  juce::String error = deviceManager.setAudioDeviceSetup(setup, true);
  if (error.isEmpty()) {
    juce::Logger::writeToLog("AudioEngine Success: Buffer size updated -> " +
                             juce::String(newBufferSize) + " samples");
    return true;
  }
  juce::Logger::writeToLog("AudioEngine Error: Failed to set buffer size " +
                           juce::String(newBufferSize) + " - " + error);
  return false;
}

void AudioEngine::handleIncomingMidiMessage(juce::MidiInput * /*source*/,
                                            const juce::MidiMessage &message) {
  if (message.isNoteOn()) {
    midiState.noteOn(message.getNoteNumber(), message.getFloatVelocity());
  } else if (message.isNoteOff()) {
    midiState.noteOff(message.getNoteNumber());
  } else if (message.isController() && message.getControllerNumber() == 64) {
    bool pedalDown = (message.getControllerValue() >= 64);
    midiState.setSustainPedal(pedalDown);
  }

  if (onMidiMessageReceived != nullptr) {
    onMidiMessageReceived(message);
  }

  const juce::ScopedLock sl(midiLock);
  incomingMidiBuffer.addEvent(message, 0);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice *device) {
  if (device != nullptr) {
    synth.prepareToPlay(device->getCurrentSampleRate(),
                        device->getCurrentBufferSizeSamples());
  }
}

void AudioEngine::audioDeviceStopped() {}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float *const * /*inputChannelData*/, int /*numInputChannels*/,
    float *const *outputChannelData, int numOutputChannels, int numSamples,
    const juce::AudioIODeviceCallbackContext & /*context*/) {
  juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels,
                                  numSamples);
  buffer.clear();

  juce::MidiBuffer midiMessagesToProcess;
  {
    const juce::ScopedLock sl(midiLock);
    midiMessagesToProcess.addEvents(incomingMidiBuffer, 0, numSamples, 0);
    incomingMidiBuffer.clear();
  }

  synth.renderNextBlock(buffer, midiMessagesToProcess, 0, numSamples);
}
