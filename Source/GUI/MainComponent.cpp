#include "MainComponent.h"

MainComponent::MainComponent() : midiMonitorView(midiState, audioEngine) {
  addAndMakeVisible(midiMonitorView);

  // Initialize audio hardware and start listening for MIDI inputs
  audioEngine.initialize();

  // Set 800x500 window size for 4-layer controls
  setSize(800, 500);
}

MainComponent::~MainComponent() { audioEngine.shutdown(); }

void MainComponent::paint(juce::Graphics &g) {
  // Dark Background fill
  g.fillAll(juce::Colour(0xff121214));
}

void MainComponent::resized() {
  // Fill the window with our MIDI monitor view (with a 10px margin)
  midiMonitorView.setBounds(getLocalBounds().reduced(10));
}
