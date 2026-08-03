#pragma once
#include "../Audio/AudioEngine.h"
#include "NetworkProtocol.h"
#include <juce_core/juce_core.h>

class NetworkServer : public juce::Thread {
public:
  NetworkServer(AudioEngine &engineToControl, MidiState &stateToMonitor);
  ~NetworkServer() override;

  bool startServer(int port = NetworkProtocol::DEFAULT_PORT);
  void stopServer();

  void run() override;

private:
  AudioEngine &audioEngine;
  MidiState &midiState;

  juce::DatagramSocket socket{true}; // Enable reuse address
  bool isRunning{false};
  juce::IPAddress clientAddress{juce::IPAddress::any()};
  int clientPort{0};

  void sendTelemetry();
};
