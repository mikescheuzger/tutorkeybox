#include "MidiMappingDrawer.h"

static const char *faderTargetNames[4] = {
    "Fader 0 (Master Vol)",
    "Fader 1 (Piano Crossfade)",
    "Fader 2 (Macro A)",
    "Fader 3 (Macro B)"
};

static const char *buttonTargetNames[3] = {
    "Button 0 (Octave Down)",
    "Button 1 (Octave Up)",
    "Button 2 (HOLD Toggle)"
};

// CORE CONCEPT: Constructor setting up CC input fields, CC Learn buttons, curve shape dropdowns, and reverse toggles.
MidiMappingDrawerComponent::MidiMappingDrawerComponent(AudioEngine &engine)
    : audioEngine(engine) {

  titleLabel.setText("MIDI HARDWARE CONTROL & CC MAPPING DRAWER", juce::dontSendNotification);
  titleLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4a90e2));
  addAndMakeVisible(titleLabel);

  // Setup Fader Rows
  for (int i = 0; i < 4; ++i) {
    auto &row = faderRows[i];
    row.label.setText(faderTargetNames[i], juce::dontSendNotification);
    row.label.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(row.label);

    row.ccInput.setText("CC " + juce::String(audioEngine.getMidiRouter().getMapping().faderCc[i]));
    addAndMakeVisible(row.ccInput);

    row.curveCombo.addItem("Linear", 1);
    row.curveCombo.addItem("Logarithmic", 2);
    row.curveCombo.addItem("Exponential", 3);
    row.curveCombo.addItem("S-Curve", 4);
    row.curveCombo.setSelectedId(1);
    addAndMakeVisible(row.curveCombo);

    addAndMakeVisible(row.reverseToggle);

    row.learnButton.setClickingTogglesState(true);
    row.learnButton.onClick = [this, i]() {
      if (faderRows[i].learnButton.getToggleState()) {
        audioEngine.getMidiRouter().setCcLearnTarget(i);
      } else {
        audioEngine.getMidiRouter().setCcLearnTarget(-1);
      }
    };
    addAndMakeVisible(row.learnButton);
  }

  // Setup Button Rows
  for (int i = 0; i < 3; ++i) {
    auto &row = buttonRows[i];
    row.label.setText(buttonTargetNames[i], juce::dontSendNotification);
    row.label.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(row.label);

    row.ccInput.setText("CC " + juce::String(audioEngine.getMidiRouter().getMapping().buttonCc[i]));
    addAndMakeVisible(row.ccInput);

    row.learnButton.setClickingTogglesState(true);
    row.learnButton.onClick = [this, i]() {
      if (buttonRows[i].learnButton.getToggleState()) {
        audioEngine.getMidiRouter().setCcLearnTarget(i + 4);
      } else {
        audioEngine.getMidiRouter().setCcLearnTarget(-1);
      }
    };
    addAndMakeVisible(row.learnButton);
  }
}

void MidiMappingDrawerComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff1c1e24));
  g.setColour(juce::Colour(0xff2d3139));
  g.drawRect(getLocalBounds(), 1);
}

void MidiMappingDrawerComponent::resized() {
  auto bounds = getLocalBounds().reduced(8);

  titleLabel.setBounds(bounds.removeFromTop(20));
  bounds.removeFromTop(6);

  int rowHeight = 24;

  // Faders column
  auto leftCol = bounds.removeFromLeft(bounds.getWidth() / 2 - 8);
  for (int i = 0; i < 4; ++i) {
    auto r = leftCol.removeFromTop(rowHeight);
    leftCol.removeFromTop(4);

    faderRows[i].label.setBounds(r.removeFromLeft(140));
    faderRows[i].ccInput.setBounds(r.removeFromLeft(50));
    r.removeFromLeft(4);
    faderRows[i].curveCombo.setBounds(r.removeFromLeft(90));
    r.removeFromLeft(4);
    faderRows[i].reverseToggle.setBounds(r.removeFromLeft(50));
    r.removeFromLeft(4);
    faderRows[i].learnButton.setBounds(r);
  }

  // Buttons column
  auto rightCol = bounds;
  for (int i = 0; i < 3; ++i) {
    auto r = rightCol.removeFromTop(rowHeight);
    rightCol.removeFromTop(4);

    buttonRows[i].label.setBounds(r.removeFromLeft(160));
    buttonRows[i].ccInput.setBounds(r.removeFromLeft(50));
    r.removeFromLeft(6);
    buttonRows[i].learnButton.setBounds(r.removeFromLeft(65));
  }
}

void MidiMappingDrawerComponent::refreshFromEngine() {
  const auto &map = audioEngine.getMidiRouter().getMapping();
  for (int i = 0; i < 4; ++i) {
    faderRows[i].ccInput.setText("CC " + juce::String(map.faderCc[i]));
    faderRows[i].reverseToggle.setToggleState(map.faderReversed[i], juce::dontSendNotification);
  }
  for (int i = 0; i < 3; ++i) {
    buttonRows[i].ccInput.setText("CC " + juce::String(map.buttonCc[i]));
  }
}
