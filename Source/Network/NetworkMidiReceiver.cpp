#include "NetworkMidiReceiver.h"

// CORE CONCEPT: Constructor binding NetworkMidiReceiver to local AudioEngine instance.
NetworkMidiReceiver::NetworkMidiReceiver(AudioEngine &engineToInject)
    : juce::Thread("NetworkMidiReceiver"), audioEngine(engineToInject) {}

NetworkMidiReceiver::~NetworkMidiReceiver() {
  stopListener();
}

// CORE CONCEPT: Binds UDP socket to target port (7778) and launches background listener thread.
bool NetworkMidiReceiver::startListener(int port) {
  socket = std::make_unique<juce::DatagramSocket>();
  if (!socket->bindToPort(port)) {
    juce::Logger::writeToLog("NetworkMidiReceiver Error: Failed to bind UDP socket to port " + juce::String(port));
    return false;
  }
  startThread(juce::Thread::Priority::high);
  juce::Logger::writeToLog("NetworkMidiReceiver: Started UDP MIDI listener on port " + juce::String(port));
  return true;
}

// CORE CONCEPT: Stops background listener thread and closes socket.
void NetworkMidiReceiver::stopListener() {
  signalThreadShouldExit();
  if (socket != nullptr) {
    socket->shutdown();
  }
  stopThread(1000);
  socket.reset();
}

// CORE CONCEPT: Background thread loop reading incoming UDP MidiForwardPackets and injecting them into AudioEngine.
void NetworkMidiReceiver::run() {
  while (!threadShouldExit()) {
    if (socket == nullptr)
      break;

    NetworkProtocol::MidiForwardPacket packet{};
    int bytesRead = socket->read(&packet, sizeof(packet), false);

    if (bytesRead == sizeof(packet) &&
        packet.magic[0] == 'T' && packet.magic[1] == 'K' &&
        packet.magic[2] == 'B' && packet.magic[3] == 'P' &&
        packet.packetType == (uint8_t)NetworkProtocol::PacketType::MidiForward) {

      juce::MidiMessage msg(packet.status, packet.data1, packet.data2);
      audioEngine.handleIncomingMidiMessage(nullptr, msg);
    }
  }
}
