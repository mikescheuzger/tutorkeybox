#include "DeployClient.h"

bool DeployClient::deployToHardware(const PresetManager &preset,
                                    const juce::String &targetHost,
                                    int targetPort) {
  juce::StreamingSocket socket;

  // 1. Try primary targetHost (e.g., "kbox.local") with 1000ms timeout
  bool connected = socket.connect(targetHost, targetPort, 1000);

  // 2. Fallback to 127.0.0.1 (localhost) if kbox.local is unreachable
  if (!connected && targetHost != "127.0.0.1" && targetHost != "localhost") {
    juce::Logger::writeToLog("DeployClient: " + targetHost +
                             " unreachable. Falling back to 127.0.0.1...");
    connected = socket.connect("127.0.0.1", targetPort, 1000);
  }

  if (!connected) {
    juce::Logger::writeToLog("DeployClient Error: Could not connect to " +
                             targetHost + " or 127.0.0.1 on port " +
                             juce::String(targetPort));
    return false;
  }

  juce::String jsonText = preset.toJsonString();
  int32_t jsonSize = (int32_t)jsonText.getNumBytesAsUTF8();

  // 3. Send size header
  if (socket.write(&jsonSize, sizeof(jsonSize)) != sizeof(jsonSize))
    return false;

  // 4. Send JSON preset package payload
  if (socket.write(jsonText.toRawUTF8(), jsonSize) != jsonSize)
    return false;

  juce::Logger::writeToLog(
      "DeployClient Success: Transmitted hardware preset deployment!");
  return true;
}
