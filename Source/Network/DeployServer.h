#pragma once
#include "../Audio/AudioEngine.h"
#include "../Core/PresetManager.h"
#include <juce_core/juce_core.h>

class DeployServer : public juce::Thread {
public:
  static constexpr int DEPLOY_PORT = 7778;

  DeployServer(AudioEngine &engineToControl, PresetManager &presetTarget);
  ~DeployServer() override;

  bool startServer();
  void stopServer();

  void run() override;

private:
  AudioEngine &audioEngine;
  PresetManager &presetManager;

  juce::StreamingSocket serverSocket;
  bool isRunning{false};

  void handleIncomingClient(juce::StreamingSocket *clientSocket);
};
