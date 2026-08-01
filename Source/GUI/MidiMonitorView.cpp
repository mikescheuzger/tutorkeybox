#include "MidiMonitorView.h"
#include "../Synth/SampleContainerReader.h"

MidiMonitorView::MidiMonitorView(MidiState &stateToMonitor,
                                 AudioEngine &engineToControl)
    : midiState(stateToMonitor), audioEngine(engineToControl) {
  // Setup Volume Sliders & Mute Buttons for all 4 Layers
  for (int i = 0; i < 4; ++i) {
    // Volume Slider Setup
    volumeSliders[i].setSliderStyle(juce::Slider::LinearVertical);
    volumeSliders[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSliders[i].setRange(0.0, 1.0, 0.01);
    volumeSliders[i].setValue(1.0);
    volumeSliders[i].onValueChange = [this, i]() {
      audioEngine.getSynth().setLayerVolume(i,
                                            (float)volumeSliders[i].getValue());
    };
    addAndMakeVisible(volumeSliders[i]);

    // Mute Button Setup
    muteButtons[i].setButtonText("Mute");
    muteButtons[i].onClick = [this, i]() {
      audioEngine.getSynth().setLayerMuted(i, muteButtons[i].getToggleState());
    };
    addAndMakeVisible(muteButtons[i]);
  }

  startTimerHz(30); // 30 Hz surveillance GUI refresh rate
}

MidiMonitorView::~MidiMonitorView() { stopTimer(); }

void MidiMonitorView::timerCallback() {
  int currentNote = midiState.currentNote.load(std::memory_order_relaxed);
  float currentVel = midiState.currentVelocity.load(std::memory_order_relaxed);
  bool currentKeyState =
      midiState.isPhysicalKeyDown.load(std::memory_order_relaxed);
  bool currentSustainState =
      midiState.isSustainPedalDown.load(std::memory_order_relaxed);

  if (currentNote != lastNote || currentVel != lastVel ||
      currentKeyState != lastKeyState ||
      currentSustainState != lastSustainState) {
    lastNote = currentNote;
    lastVel = currentVel;
    lastKeyState = currentKeyState;
    lastSustainState = currentSustainState;

    repaint();
  }
}

// --- Drag & Drop Implementation ---

bool MidiMonitorView::isInterestedInFileDrag(const juce::StringArray &files) {
  for (const auto &filePath : files) {
    juce::File file(filePath);
    if (file.getFileExtension().equalsIgnoreCase(".bin") || file.isDirectory())
      return true;
  }
  return false;
}

void MidiMonitorView::fileDragEnter(const juce::StringArray & /*files*/, int x,
                                    int /*y*/) {
  int cardWidth = getWidth() / 4;
  hoveredLayerIndex = juce::jlimit(0, 3, x / juce::jmax(1, cardWidth));
  repaint();
}

void MidiMonitorView::fileDragExit(const juce::StringArray & /*files*/) {
  hoveredLayerIndex = -1;
  repaint();
}

void MidiMonitorView::filesDropped(const juce::StringArray &files, int x,
                                   int /*y*/) {
  int cardWidth = getWidth() / 4;
  int targetLayer = juce::jlimit(0, 3, x / juce::jmax(1, cardWidth));
  hoveredLayerIndex = -1;

  for (const auto &filePath : files) {
    juce::File file(filePath);
    if (file.getFileExtension().equalsIgnoreCase(".bin")) {
      // Load .bin package file into target layer
      if (SampleContainerReader::loadContainerFile(file, audioEngine.getSynth(),
                                                   targetLayer)) {
        layerLoadedNames[targetLayer] = file.getFileNameWithoutExtension();
        repaint();
      }
    }
  }
}

void MidiMonitorView::resized() {
  auto area = getLocalBounds();
  area.removeFromTop(110); // Reserve top 110px for MIDI surveillance header

  int cardWidth = area.getWidth() / 4;

  for (int i = 0; i < 4; ++i) {
    auto cardArea = area.removeFromLeft(cardWidth).reduced(10);
    cardArea.removeFromTop(70); // Space for Layer Title & Status

    auto sliderArea = cardArea.removeFromTop(120);
    volumeSliders[i].setBounds(sliderArea.reduced(20, 0));

    muteButtons[i].setBounds(cardArea.removeFromTop(30).reduced(15, 0));
  }
}

void MidiMonitorView::paint(juce::Graphics &g) {
  // Background Dark Gradient
  g.fillAll(juce::Colour(0xff121216));

  // Top Surveillance Header Box
  auto headerBounds = getLocalBounds().removeFromTop(100).reduced(10);
  g.setColour(juce::Colour(0xff1e1e24));
  g.fillRoundedRectangle(headerBounds.toFloat(), 8.0f);
  g.setColour(juce::Colour(0xff33333f));
  g.drawRoundedRectangle(headerBounds.toFloat(), 8.0f, 1.5f);

  // Render MIDI Surveillance State
  g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
  g.setColour(juce::Colours::white);
  g.drawText("TUTOR KEYBOX :: DEVELOPER SURVEILLANCE",
             headerBounds.removeFromTop(30).reduced(15, 0),
             juce::Justification::left);

  g.setFont(juce::FontOptions(15.0f));
  juce::String noteText =
      (lastNote >= 0)
          ? "Active Note: " +
                juce::MidiMessage::getMidiNoteName(lastNote, true, true, 4) +
                " (" + juce::String(lastNote) + ")"
          : "Active Note: NONE";

  g.drawText(noteText, headerBounds.removeFromTop(25).reduced(15, 0),
             juce::Justification::left);

  // Sustain Badge
  if (lastSustainState) {
    g.setColour(juce::Colours::orange);
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("[SUSTAIN HELD]", getWidth() - 160, 45, 140, 25,
               juce::Justification::right);
  }

  // Render 4 Layer Control Cards
  auto layerArea = getLocalBounds();
  layerArea.removeFromTop(110);
  int cardWidth = layerArea.getWidth() / 4;

  for (int i = 0; i < 4; ++i) {
    auto cardBounds = layerArea.removeFromLeft(cardWidth).reduced(10);

    // Hover highlight during drag-and-drop
    if (hoveredLayerIndex == i) {
      g.setColour(juce::Colour(0xff2a4066));
      g.fillRoundedRectangle(cardBounds.toFloat(), 10.0f);
      g.setColour(juce::Colours::cyan);
      g.drawRoundedRectangle(cardBounds.toFloat(), 10.0f, 2.5f);
    } else {
      g.setColour(juce::Colour(0xff181820));
      g.fillRoundedRectangle(cardBounds.toFloat(), 10.0f);
      g.setColour(juce::Colour(0xff2a2a36));
      g.drawRoundedRectangle(cardBounds.toFloat(), 10.0f, 1.0f);
    }

    // Layer Title & Instrument Name
    g.setColour(juce::Colours::cyan);
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.drawText("LAYER " + juce::String(i),
               cardBounds.removeFromTop(30).reduced(10, 0),
               juce::Justification::centred);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(layerLoadedNames[i], cardBounds.removeFromTop(20).reduced(10, 0),
               juce::Justification::centred);

    g.setColour(juce::Colour(0xff555566));
    g.setFont(juce::FontOptions(11.0f, juce::Font::italic));
    g.drawText("[Drop .bin File Here]", cardBounds.removeFromBottom(20),
               juce::Justification::centred);
  }
}
