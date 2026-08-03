#include "NetworkServer.h"

NetworkServer::NetworkServer(AudioEngine &engineToControl,
                             MidiState &stateToMonitor)
    : Thread("NetworkServerThread"), audioEngine(engineToControl),
      midiState(stateToMonitor) {}

NetworkServer::~NetworkServer() { stopServer(); }

bool NetworkServer::startServer(int port) {
  if (!socket.bindToPort(port)) {
    juce::Logger::writeToLog("NetworkServer Error: Failed to bind to port " +
                             juce::String(port));
    return false;
  }

  isRunning = true;
  startThread(juce::Thread::Priority::normal);
  juce::Logger::writeToLog("NetworkServer: Listening on UDP port " +
                           juce::String(port));
  return true;
}

void NetworkServer::stopServer() {
  isRunning = false;
  socket.shutdown();
  stopThread(1000);
}

void NetworkServer::run() {
  uint8_t buffer[1024];

  while (!threadShouldExit() && isRunning) {
    juce::String senderIP;
    int senderPort = 0;

    int bytesRead = socket.waitUntilReady(true, 50) > 0
                        ? socket.read(buffer, sizeof(buffer), false, senderIP, senderPort)
                        : 0;

    if (bytesRead > 0) {
      clientAddress = juce::IPAddress(senderIP);
      clientPort = senderPort;

      if (bytesRead >= sizeof(NetworkProtocol::ControlPacket)) {
        auto *packet =
            reinterpret_cast<NetworkProtocol::ControlPacket *>(buffer);
        if (packet->magic[0] == 'T' && packet->magic[1] == 'K' &&
            packet->magic[2] == 'B' && packet->magic[3] == 'P') {
          if (packet->packetType ==
              (uint8_t)NetworkProtocol::PacketType::ControlCommand) {
            audioEngine.getSynth().setLayerVolume(packet->layerIndex,
                                                  packet->gain);
            audioEngine.getSynth().setLayerMute(packet->layerIndex,
                                                packet->isMuted != 0);
          }
        }
      }
    }

    // Broadcast 30 Hz Telemetry to client
    sendTelemetry();
  }
}

void NetworkServer::sendTelemetry() {
  if (clientAddress.toString().isEmpty() || clientPort == 0)
    return;

  NetworkProtocol::TelemetryPacket packet{};
  packet.cpuUsage = audioEngine.getCpuUsage();
  packet.activeVoices = (uint32_t)audioEngine.getActiveVoiceCount();

  for (int i = 0; i < 4; ++i) {
    packet.layerGain[i] = 1.0f; // Tracked layer gain
    packet.layerMuted[i] = 0;
  }

  socket.write(clientAddress.toString(), clientPort, &packet,
               sizeof(NetworkProtocol::TelemetryPacket));
}
