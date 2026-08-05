#include "MainComponent.h"

MainComponent::MainComponent() {
  addAndMakeVisible(deployBar);
  addAndMakeVisible(telemetryHeader);
  addAndMakeVisible(hardwareBar);

  addAndMakeVisible(layerCard0);
  addAndMakeVisible(layerCard1);
  addAndMakeVisible(layerCard2);
  addAndMakeVisible(layerCard3);

  // Initialize local Mac audio engine & MIDI hardware
  audioEngine.initialize();

  // Auto-load test piano into Layer 0 for instant local playing
  juce::File testBin =
      juce::File::getCurrentWorkingDirectory().getChildFile("TestPiano.bin");
  if (testBin.existsAsFile()) {
    SampleContainerReader::loadContainerFile(testBin, audioEngine.getSynth(),
                                             0);
  }

  hardwareBar.updateDropdowns();
  setSize(840, 560);
}

MainComponent::~MainComponent() { audioEngine.shutdown(); }

void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff121216));
}

void MainComponent::resized() {
  auto area = getLocalBounds().reduced(10);
  deployBar.setBounds(area.removeFromTop(36));
  area.removeFromTop(10);

  auto topRow = area.removeFromTop(100);
  hardwareBar.setBounds(topRow.removeFromRight(320));
  telemetryHeader.setBounds(topRow);

  area.removeFromTop(15);
  int cardWidth = area.getWidth() / 4;
  layerCard0.setBounds(area.removeFromLeft(cardWidth).reduced(5, 0));
  layerCard1.setBounds(area.removeFromLeft(cardWidth).reduced(5, 0));
  layerCard2.setBounds(area.removeFromLeft(cardWidth).reduced(5, 0));
  layerCard3.setBounds(area.removeFromLeft(cardWidth).reduced(5, 0));
}
