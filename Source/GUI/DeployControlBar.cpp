#include "DeployControlBar.h"

DeployControlBar::DeployControlBar(PresetManager &presetToDeploy)
    : presetManager(presetToDeploy) {
  deployButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colour(0xff2e7d32)); // Dark green
  deployButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
  deployButton.onClick = [this] { triggerDeploy(); };
  addAndMakeVisible(deployButton);

  statusLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
  statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
  addAndMakeVisible(statusLabel);
}

void DeployControlBar::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff18181c));
  g.setColour(juce::Colour(0xff2d2d35));
  g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());
}

void DeployControlBar::resized() {
  auto bounds = getLocalBounds().reduced(8, 4);
  deployButton.setBounds(bounds.removeFromRight(220));
  statusLabel.setBounds(bounds.removeFromLeft(300));
}

void DeployControlBar::triggerDeploy() {
  statusLabel.setText("Deploying to kbox.local...", juce::dontSendNotification);
  statusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);

  bool success = DeployClient::deployToHardware(presetManager, "kbox.local");

  if (success) {
    statusLabel.setText("Successfully Deployed to KeyBox!", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
  } else {
    statusLabel.setText("Deploy Failed (Check kbox.local)", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
  }
}
