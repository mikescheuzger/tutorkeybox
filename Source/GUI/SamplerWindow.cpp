#include "SamplerWindow.h"

// CORE CONCEPT: Constructor setting up library dropdown, ADSR sliders, velocity curve editor, and drag-and-drop target.
SamplerWindowComponent::SamplerWindowComponent(AudioEngine &engine, LibraryManager &library, int layerIndex)
    : audioEngine(engine), libraryManager(library), layerIdx(layerIndex) {

  titleLabel.setText("LAYER " + juce::String(layerIdx) + " SAMPLER SETTINGS", juce::dontSendNotification);
  titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4a90e2));
  addAndMakeVisible(titleLabel);

  addAndMakeVisible(micSelectLabel);
  addAndMakeVisible(micComboBox);

  // Populate mic dropdown from LibraryManager
  micComboBox.addItem("-- Select Mic Variant --", 1);
  const auto &entries = libraryManager.getEntries();
  for (size_t i = 0; i < entries.size(); ++i) {
    micComboBox.addItem(entries[i].getDisplayName(), static_cast<int>(i + 2));
  }

  micComboBox.onChange = [this]() {
    int selectedId = micComboBox.getSelectedId();
    if (selectedId >= 2) {
      size_t entryIdx = static_cast<size_t>(selectedId - 2);
      const auto &entry = libraryManager.getEntries()[entryIdx];

      audioEngine.getSynth().clearLayer(layerIdx);
      SampleContainerReader::loadContainerFile(entry.binFile, audioEngine.getSynth(), layerIdx);
      juce::Logger::writeToLog("SamplerWindow: Selected mic variant -> " + entry.binFileName);
    }
  };

  addAndMakeVisible(ampGroup);
  setupSlider(attackSlider, attackLabel, 1.0f, 5000.0f, 5.0f, " ms");
  setupSlider(decaySlider, decayLabel, 1.0f, 5000.0f, 200.0f, " ms");
  setupSlider(sustainSlider, sustainLabel, 0.0f, 1.0f, 1.0f, "");
  setupSlider(releaseSlider, releaseLabel, 1.0f, 10000.0f, 300.0f, " ms");

  addAndMakeVisible(velGroup);
  addAndMakeVisible(velCurveEditor);

  dropZoneLabel.setText("Drop .bin or .wav folder here", juce::dontSendNotification);
  dropZoneLabel.setJustificationType(juce::Justification::centred);
  dropZoneLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
  addAndMakeVisible(dropZoneLabel);
}

void SamplerWindowComponent::setupSlider(juce::Slider &slider, juce::Label &label, float minVal, float maxVal, float defaultVal, const juce::String &suffix) {
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
  slider.setRange(minVal, maxVal, 1.0f);
  slider.setValue(defaultVal);
  slider.setTextValueSuffix(suffix);

  label.setJustificationType(juce::Justification::centred);
  label.setFont(juce::FontOptions(11.0f));

  addAndMakeVisible(slider);
  addAndMakeVisible(label);
}

void SamplerWindowComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff21252b));

  auto dropBounds = getLocalBounds().removeFromBottom(40).reduced(8);
  g.setColour(juce::Colour(0xff2d3139));
  g.fillRoundedRectangle(dropBounds.toFloat(), 4.0f);
  g.setColour(juce::Colour(0xff4a90e2));
  g.drawRoundedRectangle(dropBounds.toFloat(), 4.0f, 1.0f);
}

void SamplerWindowComponent::resized() {
  auto bounds = getLocalBounds().reduced(12);

  titleLabel.setBounds(bounds.removeFromTop(24));
  bounds.removeFromTop(8);

  auto micRow = bounds.removeFromTop(28);
  micSelectLabel.setBounds(micRow.removeFromLeft(90));
  micComboBox.setBounds(micRow);

  bounds.removeFromTop(12);

  // Amp envelope group
  auto ampBounds = bounds.removeFromTop(120);
  ampGroup.setBounds(ampBounds);
  auto sliderArea = ampBounds.reduced(10, 20);
  int sliderWidth = sliderArea.getWidth() / 4;

  auto s0 = sliderArea.removeFromLeft(sliderWidth);
  attackLabel.setBounds(s0.removeFromTop(14));
  attackSlider.setBounds(s0);

  auto s1 = sliderArea.removeFromLeft(sliderWidth);
  decayLabel.setBounds(s1.removeFromTop(14));
  decaySlider.setBounds(s1);

  auto s2 = sliderArea.removeFromLeft(sliderWidth);
  sustainLabel.setBounds(s2.removeFromTop(14));
  sustainSlider.setBounds(s2);

  auto s3 = sliderArea;
  releaseLabel.setBounds(s3.removeFromTop(14));
  releaseSlider.setBounds(s3);

  bounds.removeFromTop(12);

  // Velocity group
  auto velBounds = bounds.removeFromTop(160);
  velGroup.setBounds(velBounds);
  velCurveEditor.setBounds(velBounds.reduced(10, 20));

  // Drop zone
  auto dropArea = bounds.removeFromBottom(36);
  dropZoneLabel.setBounds(dropArea);
}

bool SamplerWindowComponent::isInterestedInFileDrag(const juce::StringArray &files) {
  for (const auto &file : files) {
    if (file.endsWithIgnoreCase(".bin") || juce::File(file).isDirectory())
      return true;
  }
  return false;
}

void SamplerWindowComponent::filesDropped(const juce::StringArray &files, int /*x*/, int /*y*/) {
  if (files.isEmpty()) return;

  juce::File droppedFile(files[0]);
  audioEngine.getSynth().clearLayer(layerIdx);

  if (droppedFile.getFileExtension().equalsIgnoreCase(".bin")) {
    SampleContainerReader::loadContainerFile(droppedFile, audioEngine.getSynth(), layerIdx);
  } else if (droppedFile.isDirectory()) {
    // Package into temp .bin and load
    juce::File tempBin = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("dropped_temp.bin");
    if (SamplePackager::createPackage(droppedFile, tempBin)) {
      SampleContainerReader::loadContainerFile(tempBin, audioEngine.getSynth(), layerIdx);
    }
  }
}

void SamplerWindowComponent::refreshFromEngine() {}

// ==============================================================================
// SamplerWindow implementation
// ==============================================================================

SamplerWindow::SamplerWindow(AudioEngine &engine, LibraryManager &library, int layerIndex)
    : DocumentWindow("Layer " + juce::String(layerIndex) + " Sampler",
                     juce::Colour(0xff21252b),
                     DocumentWindow::closeButton) {
  setContentOwned(new SamplerWindowComponent(engine, library, layerIndex), true);
  setUsingNativeTitleBar(true);
  setResizable(false, false);
  centreWithSize(480, 440);
  setVisible(true);
}

void SamplerWindow::closeButtonPressed() {
  delete this;
}
