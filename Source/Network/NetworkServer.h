#pragma once
#include "../Audio/AudioEngine.h"
#include "NetworkProtocol.h"
#include <juce_core/juce_core.h>

// CORE CONCEPT: Raspberry Pi headless network server broadcasting telemetry and live MIDI packets to Mac GUI.
class NetworkServer : public juce::Thread {
public:
  NetworkServer(AudioEngine &engineToControl, MidiState &stateToMonitor);
  ~NetworkServer() override;

  // CORE CONCEPT: Starts network server UDP broadcast daemon on specified port.
  bool startServer(int port = NetworkProtocol::DEFAULT_PORT);

  // CORE CONCEPT: Stops network server thread and closes sockets.
  void stopServer();

  // CORE CONCEPT: Broadcasts a live MIDI message to connected Mac GUI clients via UDP MidiForwardPacket.
  void broadcastMidiMessage(const juce::MidiMessage &message);

  void run() override;

private:
  AudioEngine &audioEngine;
  MidiState &midiState;

  juce::DatagramSocket socket{true}; // Enable reuse address
  juce::DatagramSocket midiForwardSocket{true};
  bool isRunning{false};
  juce::IPAddress clientAddress{juce::IPAddress::any()};
  int clientPort{0};

  void sendTelemetry();
};
