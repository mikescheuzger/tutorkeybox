#include "../Audio/AudioEngine.h"
#include "../Core/MidiState.h"
#include "../Network/NetworkServer.h"
#include <juce_core/juce_core.h>

int main(int argc, char *argv[]) {
  juce::initialiseJuce_GUI();

  juce::Logger::writeToLog("==========================================");
  juce::Logger::writeToLog("  TutorKeybox Core (Headless Audio Engine)");
  juce::Logger::writeToLog("==========================================");

  MidiState midiState;
  AudioEngine audioEngine(midiState);
  audioEngine.initialize();

  // Try loading default layer container if available in current directory
  juce::File defaultBin("SalamanderGrandPianoWAVs.bin");
  if (defaultBin.existsAsFile()) {
    juce::Logger::writeToLog("Loading default container: " +
                             defaultBin.getFullPathName());
    SampleContainerReader::loadContainerFile(defaultBin,
                                             audioEngine.getSynth(), 0);
  }

  NetworkServer server(audioEngine, midiState);
  server.startServer(NetworkProtocol::DEFAULT_PORT);

  juce::Logger::writeToLog(
      "Headless Audio Core is running live on Raspberry Pi 5!");
  juce::Logger::writeToLog("Press Ctrl+C to stop.");

  // Keep headless daemon process alive
  while (true) {
    juce::Thread::sleep(1000);
  }

  server.stopServer();
  audioEngine.shutdown();
  juce::shutdownJuce_GUI();
  return 0;
}
