#include "HardwareSelectorBar.h"

HardwareSelectorBar::HardwareSelectorBar(AudioEngine &engineToControl)
    : audioEngine(engineToControl) {
  audioOutputSelector.setTextWhenNothingSelected("Select Audio Output");
  midiInputSelector.setTextWhenNothingSelected("Select MIDI Input");
  latencySelector.setTextWhenNothingSelected("Select Latency");

  latencySelector.addItem("32 samples (~0.7 ms)", 1);
  latencySelector.addItem("64 samples (~1.3 ms)", 2);
  latencySelector.addItem("128 samples (~2.7 ms)", 3);
  latencySelector.addItem("256 samples (~5.3 ms)", 4);

  addAndMakeVisible(audioOutputSelector);
  addAndMakeVisible(midiInputSelector);
  addAndMakeVisible(latencySelector);

  updateDropdowns();

  // Audio Output Change Handler
  audioOutputSelector.onChange = [this]() {
    int selectedId = audioOutputSelector.getSelectedId();
    if (selectedId > 0) {
      juce::String deviceName = audioOutputSelector.getText();
      juce::AudioDeviceManager::AudioDeviceSetup setup;
      audioEngine.getDeviceManager().getAudioDeviceSetup(setup);
      setup.outputDeviceName = deviceName;
      audioEngine.getDeviceManager().setAudioDeviceSetup(setup, true);
    }
  };

  // MIDI Input Change Handler
  midiInputSelector.onChange = [this]() {
    int selectedId = midiInputSelector.getSelectedId();
    if (selectedId > 0) {
      auto midiInputs = juce::MidiInput::getAvailableDevices();
      for (const auto &input : midiInputs) {
        bool enable = (input.name == midiInputSelector.getText());
        audioEngine.getDeviceManager().setMidiInputDeviceEnabled(
            input.identifier, enable);

        if (enable) {
          audioEngine.getDeviceManager().addMidiInputDeviceCallback(
              input.identifier, &audioEngine);
        } else {
          audioEngine.getDeviceManager().removeMidiInputDeviceCallback(
              input.identifier, &audioEngine);
        }
      }
    }
  };

  // Latency Change Handler
  latencySelector.onChange = [this]() {
    int id = latencySelector.getSelectedId();
    int bufferSize = 128;
    if (id == 1)
      bufferSize = 32;
    else if (id == 2)
      bufferSize = 64;
    else if (id == 3)
      bufferSize = 128;
    else if (id == 4)
      bufferSize = 256;

    audioEngine.setBufferSize(bufferSize);
  };
}

void HardwareSelectorBar::updateDropdowns() {
  // 1. Scan Audio Output Devices
  audioOutputSelector.clear();
  int itemIdx = 1;

  const auto &deviceTypes =
      audioEngine.getDeviceManager().getAvailableDeviceTypes();
  for (auto *type : deviceTypes) {
    type->scanForDevices();
    auto outputDevices = type->getDeviceNames(false); // false = output devices

    for (const auto &devName : outputDevices) {
      audioOutputSelector.addItem(devName, itemIdx++);
    }
  }

  auto currentSetup = audioEngine.getDeviceManager().getAudioDeviceSetup();
  if (currentSetup.outputDeviceName.isNotEmpty()) {
    audioOutputSelector.setText(currentSetup.outputDeviceName,
                                juce::dontSendNotification);
  } else if (audioOutputSelector.getNumItems() > 0) {
    audioOutputSelector.setSelectedId(1, juce::dontSendNotification);
  }

  // 2. Scan MIDI Inputs
  midiInputSelector.clear();
  auto midiInputs = juce::MidiInput::getAvailableDevices();

  for (int i = 0; i < midiInputs.size(); ++i) {
    midiInputSelector.addItem(midiInputs[i].name, i + 1);
    if (audioEngine.getDeviceManager().isMidiInputDeviceEnabled(
            midiInputs[i].identifier)) {
      midiInputSelector.setSelectedId(i + 1, juce::dontSendNotification);
      audioEngine.getDeviceManager().addMidiInputDeviceCallback(
          midiInputs[i].identifier, &audioEngine);
    }
  }

  if (midiInputSelector.getSelectedId() == 0 && !midiInputs.isEmpty()) {
    midiInputSelector.setSelectedId(1, juce::dontSendNotification);
    audioEngine.getDeviceManager().setMidiInputDeviceEnabled(
        midiInputs[0].identifier, true);
    audioEngine.getDeviceManager().addMidiInputDeviceCallback(
        midiInputs[0].identifier, &audioEngine);
  }

  // 3. Select active buffer size in latency dropdown
  if (currentSetup.bufferSize == 32)
    latencySelector.setSelectedId(1, juce::dontSendNotification);
  else if (currentSetup.bufferSize == 64)
    latencySelector.setSelectedId(2, juce::dontSendNotification);
  else if (currentSetup.bufferSize == 128)
    latencySelector.setSelectedId(3, juce::dontSendNotification);
  else if (currentSetup.bufferSize == 256)
    latencySelector.setSelectedId(4, juce::dontSendNotification);
  else
    latencySelector.setSelectedId(3, juce::dontSendNotification);
}

void HardwareSelectorBar::resized() {
  auto area = getLocalBounds();
  int itemWidth = (area.getWidth() - 16) / 3;

  audioOutputSelector.setBounds(area.removeFromLeft(itemWidth));
  area.removeFromLeft(8);
  midiInputSelector.setBounds(area.removeFromLeft(itemWidth));
  area.removeFromLeft(8);
  latencySelector.setBounds(area);
}
