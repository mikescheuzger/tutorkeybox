#include "HardwareSelectorBar.h"

HardwareSelectorBar::HardwareSelectorBar(AudioEngine &engineToControl)
    : audioEngine(engineToControl) {
  audioOutputSelector.setTextWhenNothingSelected("Select Audio Output");
  midiInputSelector.setTextWhenNothingSelected("Select MIDI Input");

  addAndMakeVisible(audioOutputSelector);
  addAndMakeVisible(midiInputSelector);

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
} // <--- Constructor ends here!

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
}

void HardwareSelectorBar::resized() {
  auto area = getLocalBounds();
  audioOutputSelector.setBounds(area.removeFromTop(32));
  area.removeFromTop(8);
  midiInputSelector.setBounds(area.removeFromTop(32));
}
