#include "MainComponent.h"

// CORE CONCEPT: Master application constructor initializing audio engine, loading startup preset, and laying out DAW channel strips and MIDI drawer.
MainComponent::MainComponent() {
  addAndMakeVisible(deployBar);
  addAndMakeVisible(telemetryHeader);
  addAndMakeVisible(hardwareBar);

  addAndMakeVisible(channelStrip0);
  addAndMakeVisible(channelStrip1);
  addAndMakeVisible(channelStrip2);
  addAndMakeVisible(channelStrip3);

  addAndMakeVisible(midiDrawer);

  // 1. Initialize audio hardware & MIDI inputs FIRST
  audioEngine.initialize();

  // 2. Populate dropdown choices NOW that hardware is initialized!
  hardwareBar.updateDropdowns();

  // 3. Load startup preset if default preset file exists
  juce::File presetFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("TutorKeybox/preset.json");
  if (presetFile.existsAsFile()) {
    presetManager.loadFromFile(presetFile);
    audioEngine.getMidiRouter().updateFromPreset(presetManager.getMapping());
  }

  setSize(960, 680);
}

// CORE CONCEPT: Master destructor auto-saving current app preset state to application data directory.
MainComponent::~MainComponent() {
  juce::File presetFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("TutorKeybox/preset.json");
  presetFile.getParentDirectory().createDirectory();
  presetManager.saveToFile(presetFile);

  audioEngine.shutdown();
}

void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff121216)); // Sleek dark mode background
}

void MainComponent::resized() {
  auto area = getLocalBounds().reduced(8);

  deployBar.setBounds(area.removeFromTop(36));
  area.removeFromTop(8);

  auto topRow = area.removeFromTop(80);
  hardwareBar.setBounds(topRow.removeFromRight(320));
  telemetryHeader.setBounds(topRow);

  area.removeFromTop(8);

  // Bottom MIDI Drawer
  auto drawerArea = area.removeFromBottom(150);
  midiDrawer.setBounds(drawerArea);

  area.removeFromBottom(8);

  // 4 DAW Channel Strips
  int stripWidth = area.getWidth() / 4;
  channelStrip0.setBounds(area.removeFromLeft(stripWidth).reduced(3, 0));
  channelStrip1.setBounds(area.removeFromLeft(stripWidth).reduced(3, 0));
  channelStrip2.setBounds(area.removeFromLeft(stripWidth).reduced(3, 0));
  channelStrip3.setBounds(area.removeFromLeft(stripWidth).reduced(3, 0));
}
