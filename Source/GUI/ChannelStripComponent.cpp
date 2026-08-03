#include "ChannelStripComponent.h"

// CORE CONCEPT: Constructor setting up DAW-style channel strip slots, buttons, volume fader, and floating window launchers.
ChannelStripComponent::ChannelStripComponent(AudioEngine &engine, LibraryManager &library, int layerIndex)
    : audioEngine(engine), libraryManager(library), layerIdx(layerIndex) {

  juce::String layerTitle = (layerIdx == 0) ? "CH 0: PIANO A"
                          : (layerIdx == 1) ? "CH 1: PIANO B"
                          : (layerIdx == 2) ? "CH 2: PAD A"
                                            : "CH 3: PAD B";

  headerLabel.setText(layerTitle, juce::dontSendNotification);
  headerLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
  headerLabel.setJustificationType(juce::Justification::centred);
  headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4a90e2));
  addAndMakeVisible(headerLabel);

  // Sampler Slot Button
  samplerSlotButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d3139));
  samplerSlotButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe0e0e0));
  samplerSlotButton.onClick = [this]() {
    if (activeSamplerWindow == nullptr) {
      activeSamplerWindow = new SamplerWindow(audioEngine, libraryManager, layerIdx);
    } else {
      activeSamplerWindow->toFront(true);
    }
  };
  addAndMakeVisible(samplerSlotButton);

  // Reverb FX Insert Button
  reverbFxButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff282c34));
  reverbFxButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff9500));
  reverbFxButton.onClick = [this]() {
    if (activeReverbWindow == nullptr) {
      activeReverbWindow = new ReverbWindow(audioEngine);
    } else {
      activeReverbWindow->toFront(true);
    }
  };
  addAndMakeVisible(reverbFxButton);

  // Filter FX Insert Button (Layers 2 & 3 only)
  if (layerIdx >= 2) {
    filterFxButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff282c34));
    filterFxButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff2acc7a));
    filterFxButton.onClick = [this]() {
      if (activeFilterWindow == nullptr) {
        activeFilterWindow = new FilterWindow(audioEngine, layerIdx);
      } else {
        activeFilterWindow->toFront(true);
      }
    };
    addAndMakeVisible(filterFxButton);

    // Octave Up / Down / Hold Buttons
    octDownButton.onClick = [this]() {
      int current = audioEngine.getSynth().getLayerOctaveOffset(layerIdx);
      audioEngine.getSynth().setLayerOctaveOffset(layerIdx, current - 1);
    };
    addAndMakeVisible(octDownButton);

    octUpButton.onClick = [this]() {
      int current = audioEngine.getSynth().getLayerOctaveOffset(layerIdx);
      audioEngine.getSynth().setLayerOctaveOffset(layerIdx, current + 1);
    };
    addAndMakeVisible(octUpButton);

    holdButton.setClickingTogglesState(true);
    holdButton.onClick = [this]() {
      bool hold = holdButton.getToggleState();
      audioEngine.getSynth().setLayerHold(layerIdx, hold);
    };
    addAndMakeVisible(holdButton);
  }

  // Volume Fader
  volumeFader.setSliderStyle(juce::Slider::LinearVertical);
  volumeFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
  volumeFader.setRange(0.0f, 1.0f, 0.01f);
  volumeFader.setValue(1.0f);
  volumeFader.onValueChange = [this]() {
    audioEngine.getSynth().setLayerVolume(layerIdx, (float)volumeFader.getValue());
  };
  addAndMakeVisible(volumeFader);

  // Mute Button
  muteButton.onClick = [this]() {
    audioEngine.getSynth().setLayerMute(layerIdx, muteButton.getToggleState());
  };
  addAndMakeVisible(muteButton);
}

ChannelStripComponent::~ChannelStripComponent() {
  if (activeSamplerWindow != nullptr) delete activeSamplerWindow.getComponent();
  if (activeReverbWindow != nullptr) delete activeReverbWindow.getComponent();
  if (activeFilterWindow != nullptr) delete activeFilterWindow.getComponent();
}

void ChannelStripComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat().reduced(2.0f);
  g.setColour(juce::Colour(0xff1e2227));
  g.fillRoundedRectangle(bounds, 6.0f);
  g.setColour(juce::Colour(0xff2d3139));
  g.drawRoundedRectangle(bounds, 6.0f, 1.5f);
}

void ChannelStripComponent::resized() {
  auto bounds = getLocalBounds().reduced(6);

  headerLabel.setBounds(bounds.removeFromTop(20));
  bounds.removeFromTop(4);

  // Slot 1: Sampler Button
  samplerSlotButton.setBounds(bounds.removeFromTop(32));
  bounds.removeFromTop(6);

  // Slot 2: FX Inserts
  reverbFxButton.setBounds(bounds.removeFromTop(22));
  bounds.removeFromTop(4);

  if (layerIdx >= 2) {
    filterFxButton.setBounds(bounds.removeFromTop(22));
    bounds.removeFromTop(6);

    // Octave & Hold Row
    auto octRow = bounds.removeFromTop(22);
    octDownButton.setBounds(octRow.removeFromLeft(octRow.getWidth() / 2 - 2));
    octUpButton.setBounds(octRow);

    bounds.removeFromTop(4);
    holdButton.setBounds(bounds.removeFromTop(22));
    bounds.removeFromTop(6);
  }

  // Mute at bottom
  muteButton.setBounds(bounds.removeFromBottom(24));
  bounds.removeFromBottom(4);

  // Remaining middle is Vertical Volume Fader
  volumeFader.setBounds(bounds);
}

void ChannelStripComponent::refreshFromEngine() {}
