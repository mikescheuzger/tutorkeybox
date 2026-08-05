#include "LayerCardComponent.h"

LayerCardComponent::LayerCardComponent(int layerIndexToManage,
                                       AudioEngine &engineToControl,
                                       PresetManager &presetToUpdate)
    : layerIndex(layerIndexToManage), audioEngine(engineToControl),
      presetManager(presetToUpdate) {
  // Volume Slider Setup
  volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
  volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  volumeSlider.setRange(0.0, 1.0, 0.01);
  volumeSlider.setValue(1.0);
  volumeSlider.onValueChange = [this]() {
    audioEngine.getSynth().setLayerVolume(layerIndex,
                                          (float)volumeSlider.getValue());
    updatePresetState();
  };
  addAndMakeVisible(volumeSlider);

  // Mute Button Setup
  muteButton.setButtonText("Mute");
  muteButton.setClickingTogglesState(true);
  muteButton.onClick = [this]() {
    audioEngine.getSynth().setLayerMuted(layerIndex,
                                         muteButton.getToggleState());
    updatePresetState();
  };
  addAndMakeVisible(muteButton);
}

void LayerCardComponent::updatePresetState() {
  presetManager.setLayerPreset(layerIndex, (float)volumeSlider.getValue(),
                               muteButton.getToggleState(),
                               loadedContainerPath);
}

bool LayerCardComponent::isInterestedInFileDrag(
    const juce::StringArray &files) {
  for (const auto &filePath : files) {
    juce::File file(filePath);
    if (file.getFileExtension().equalsIgnoreCase(".bin") ||
        file.getFileExtension().equalsIgnoreCase(".wav") || file.isDirectory())
      return true;
  }
  return false;
}

void LayerCardComponent::fileDragEnter(const juce::StringArray & /*files*/,
                                       int /*x*/, int /*y*/) {
  isHoveredDuringDrag = true;
  repaint();
}

void LayerCardComponent::fileDragExit(const juce::StringArray & /*files*/) {
  isHoveredDuringDrag = false;
  repaint();
}

void LayerCardComponent::filesDropped(const juce::StringArray &files, int /*x*/,
                                      int /*y*/) {
  isHoveredDuringDrag = false;

  for (const auto &filePath : files) {
    juce::File file(filePath);

    if (file.getFileExtension().equalsIgnoreCase(".bin")) {
      if (SampleContainerReader::loadContainerFile(file, audioEngine.getSynth(),
                                                   layerIndex)) {
        loadedInstrumentName = file.getFileNameWithoutExtension();
        loadedContainerPath = file.getFullPathName();
        updatePresetState();
        repaint();
      }
    } else if (file.isDirectory() ||
               file.getFileExtension().equalsIgnoreCase(".wav")) {
      juce::File tempBin =
          juce::File::getSpecialLocation(juce::File::tempDirectory)
              .getChildFile("Layer" + juce::String(layerIndex) + ".bin");

      if (SamplePackager::createPackage(file, tempBin)) {
        if (SampleContainerReader::loadContainerFile(
                tempBin, audioEngine.getSynth(), layerIndex)) {
          loadedInstrumentName = file.getFileNameWithoutExtension();
          loadedContainerPath = tempBin.getFullPathName();
          updatePresetState();
          repaint();
        }
      }
    }
  }
}

void LayerCardComponent::resized() {
  auto area = getLocalBounds();
  area.removeFromTop(60); // Space for Layer Title & Instrument Name

  auto sliderArea = area.removeFromTop(120);
  volumeSlider.setBounds(sliderArea.reduced(20, 0));

  muteButton.setBounds(area.removeFromTop(30).reduced(15, 0));
}

void LayerCardComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Hover highlight during drag-and-drop
  if (isHoveredDuringDrag) {
    g.setColour(juce::Colour(0xff2a4066));
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(juce::Colours::cyan);
    g.drawRoundedRectangle(bounds, 10.0f, 2.5f);
  } else {
    g.setColour(juce::Colour(0xff181820));
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(juce::Colour(0xff2a2a36));
    g.drawRoundedRectangle(bounds, 10.0f, 1.0f);
  }

  // Title
  g.setColour(juce::Colours::cyan);
  g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  g.drawText("LAYER " + juce::String(layerIndex), 10, 10, getWidth() - 20, 22,
             juce::Justification::centred);

  // Instrument Name
  g.setColour(juce::Colours::lightgrey);
  g.setFont(juce::FontOptions(13.0f));
  g.drawText(loadedInstrumentName, 10, 34, getWidth() - 20, 20,
             juce::Justification::centred);

  // Drag-and-drop hint
  g.setColour(juce::Colour(0xff555566));
  g.setFont(juce::FontOptions(11.0f, juce::Font::italic));
  g.drawText("[Drop .wav Folder or .bin Here]", 5, getHeight() - 22,
             getWidth() - 10, 18, juce::Justification::centred);
}
