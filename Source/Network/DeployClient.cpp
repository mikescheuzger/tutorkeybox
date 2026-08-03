#include "DeployClient.h"

bool DeployClient::deployToHardware(const PresetManager &preset,
                                    const juce::String &targetHost,
                                    int targetPort) {
  juce::StreamingSocket socket;
  if (!socket.connect(targetHost, targetPort, 3000)) {
    juce::Logger::writeToLog("DeployClient Error: Could not connect to " +
                             targetHost + ":" + juce::String(targetPort));
    return false;
  }

  juce::String jsonText = preset.toJsonString();
  int32_t jsonSize = (int32_t)jsonText.getNumBytesAsUTF8();

  // 1. Send size header
  if (socket.write(&jsonSize, sizeof(jsonSize)) != sizeof(jsonSize))
    return false;

  // 2. Send JSON preset package
  if (socket.write(jsonText.toRawUTF8(), jsonSize) != jsonSize)
    return false;

  juce::Logger::writeToLog("DeployClient Success: Transmitted hardware preset deployment to " +
                           targetHost + "!");
  return true;
}
