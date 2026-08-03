#include "DeployServer.h"

DeployServer::DeployServer(AudioEngine &engineToControl,
                           PresetManager &presetTarget)
    : Thread("DeployServerThread"), audioEngine(engineToControl),
      presetManager(presetTarget) {}

DeployServer::~DeployServer() { stopServer(); }

bool DeployServer::startServer() {
  if (!serverSocket.createListener(DEPLOY_PORT)) {
    juce::Logger::writeToLog("DeployServer Error: Failed to create listener on port " +
                             juce::String(DEPLOY_PORT));
    return false;
  }

  isRunning = true;
  startThread(juce::Thread::Priority::normal);
  juce::Logger::writeToLog("DeployServer: Listening for hardware deployment packages on TCP port " +
                           juce::String(DEPLOY_PORT));
  return true;
}

void DeployServer::stopServer() {
  isRunning = false;
  serverSocket.close();
  stopThread(1000);
}

void DeployServer::run() {
  while (!threadShouldExit() && isRunning) {
    auto *clientSocket = serverSocket.waitForNextConnection();
    if (clientSocket != nullptr) {
      handleIncomingClient(clientSocket);
      delete clientSocket;
    }
  }
}

void DeployServer::handleIncomingClient(juce::StreamingSocket *clientSocket) {
  juce::Logger::writeToLog("DeployServer: Incoming connection from Mac Deployer!");

  // Read header size
  int32_t jsonSize = 0;
  if (clientSocket->read(&jsonSize, sizeof(jsonSize), true) != sizeof(jsonSize))
    return;

  if (jsonSize <= 0 || jsonSize > 10 * 1024 * 1024)
    return; // Sanity check 10 MB limit for JSON

  juce::MemoryBlock jsonBuffer((size_t)jsonSize);
  if (clientSocket->read(jsonBuffer.getData(), jsonSize, true) != jsonSize)
    return;

  juce::String jsonText = jsonBuffer.toString();
  if (presetManager.loadFromJsonString(jsonText)) {
    juce::Logger::writeToLog("DeployServer Success: Deployed new hardware preset configuration!");

    // Save deployed preset to persistent storage
    juce::File presetFile =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("deployed_preset.json");
    presetManager.saveToFile(presetFile);
  }
}
