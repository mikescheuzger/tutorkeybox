#pragma once
#include "../Core/PresetManager.h"
#include <juce_core/juce_core.h>

class DeployClient {
public:
  DeployClient() = default;
  ~DeployClient() = default;

  static bool deployToHardware(const PresetManager &preset,
                               const juce::String &targetHost = "kbox.local",
                               int targetPort = 7778);
};
