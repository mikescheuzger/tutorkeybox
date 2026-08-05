#include "DeployServer.h"

DeployServer::DeployServer(AudioEngine &engineToControl,
                           PresetManager &presetTarget)
    : Thread("DeployServerThread"), audioEngine(engineToControl),
      presetManager(presetTarget) {}

DeployServer::~DeployServer() { stopServer(); }

bool DeployServer::startServer() {
  if (!serverSocket.createListener(DEPLOY_PORT)) {
    juce::Logger::writeToLog("DeployServer Error: Failed listener on port " +
                             juce::String(DEPLOY_PORT));
    return false;
  }

  isRunning = true;
  startThread(juce::Thread::Priority::normal);
  juce::Logger::writeToLog("DeployServer: Listening on TCP port " +
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
  juce::Logger::writeToLog(
      "DeployServer: Incoming preset deployment from Mac!");

  int32_t jsonSize = 0;
  if (clientSocket->read(&jsonSize, sizeof(jsonSize), true) != sizeof(jsonSize))
    return;

  if (jsonSize <= 0 || jsonSize > 10 * 1024 * 1024)
    return;

  juce::MemoryBlock jsonBuffer((size_t)jsonSize);
  if (clientSocket->read(jsonBuffer.getData(), jsonSize, true) != jsonSize)
    return;

  juce::String jsonText = jsonBuffer.toString();
  if (presetManager.loadFromJsonString(jsonText)) {
    juce::Logger::writeToLog("DeployServer Success: Deployed hardware preset!");

    // Apply deployed preset directly to live Pi audio engine
    for (int layerIdx = 0; layerIdx < 4; ++layerIdx) {
      const auto &layerPreset = presetManager.getLayerPreset(layerIdx);
      audioEngine.getSynth().setLayerVolume(layerIdx, layerPreset.volume);
      audioEngine.getSynth().setLayerMute(layerIdx, layerPreset.muted);

      if (layerPreset.sampleContainerPath.isNotEmpty()) {
        juce::File binFile(layerPreset.sampleContainerPath);
        if (!juce::File::isAbsolutePath(layerPreset.sampleContainerPath)) {
          binFile = juce::File::getCurrentWorkingDirectory().getChildFile(
              layerPreset.sampleContainerPath);
        }
        if (binFile.existsAsFile()) {
          SampleContainerReader::loadContainerFile(
              binFile, audioEngine.getSynth(), layerIdx);
        }
      }
    }

    // Persist active preset on Pi disk
    juce::File presetFile =
        juce::File::getCurrentWorkingDirectory().getChildFile("preset.json");
    presetManager.saveToFile(presetFile);
  }
}
