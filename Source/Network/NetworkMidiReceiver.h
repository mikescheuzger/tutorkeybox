#pragma once
#include "../Audio/AudioEngine.h"
#include "NetworkProtocol.h"
#include <juce_core/juce_core.h>

// CORE CONCEPT: Mac-side UDP network MIDI receiver thread that listens for incoming
// MidiForwardPacket broadcasts from Raspberry Pi and injects them into local AudioEngine.
class NetworkMidiReceiver : public juce::Thread {
public:
  explicit NetworkMidiReceiver(AudioEngine &engineToInject);
  ~NetworkMidiReceiver() override;

  // CORE CONCEPT: Binds UDP socket to MIDI port and starts background listener thread.
  bool startListener(int port = NetworkProtocol::MIDI_PORT);

  // CORE CONCEPT: Stops background listener thread and closes socket.
  void stopListener();

  // CORE CONCEPT: Returns true if receiver listener thread is active.
  bool isListening() const { return isThreadRunning(); }

private:
  void run() override;

  AudioEngine &audioEngine;
  std::unique_ptr<juce::DatagramSocket> socket;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkMidiReceiver)
};
