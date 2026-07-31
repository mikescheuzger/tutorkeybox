#include "MainComponent.h"

MainComponent::MainComponent() {
  // Make the MIDI monitor view visible inside this main container
  addAndMakeVisible(midiMonitorView);

  // Initialize audio hardware and start listening for MIDI inputs
  audioEngine.initialize();

  // Set initial window size
  setSize(650, 300);
}

MainComponent::~MainComponent() { audioEngine.shutdown(); }

void MainComponent::paint(juce::Graphics &g) {
  // Background fill
  g.fillAll(juce::Colour(0xff121214));
}

void MainComponent::resized() {
  // Fill the window with our MIDI monitor view (with a 10px margin)
  midiMonitorView.setBounds(getLocalBounds().reduced(10));
}
