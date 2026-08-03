#pragma once
#include "../Core/PresetManager.h"
#include "../Network/DeployClient.h"
#include <juce_gui_basics/juce_gui_basics.h>

class DeployControlBar : public juce::Component {
public:
  DeployControlBar(PresetManager &presetToDeploy);
  ~DeployControlBar() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  PresetManager &presetManager;
  juce::TextButton deployButton{"Deploy to KeyBox Hardware"};
  juce::Label statusLabel{"status", "Target: kbox.local (Ready)"};

  void triggerDeploy();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeployControlBar)
};
