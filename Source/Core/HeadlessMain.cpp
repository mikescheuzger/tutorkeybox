#include "../Audio/AudioEngine.h"
#include "../Core/MidiState.h"
#include "../Core/PresetManager.h"
#include "../Network/NetworkServer.h"
#include <juce_core/juce_core.h>

int main(int argc, char *argv[]) {
  juce::ScopedJuceInitialiser_GUI juceInit;

  juce::Logger::writeToLog("==========================================");
  juce::Logger::writeToLog("  TutorKeybox Core (Headless Audio Engine)");
  juce::Logger::writeToLog("==========================================");

  MidiState midiState;
  AudioEngine audioEngine(midiState);

  // 1. Initialize hardware audio soundcard & MIDI inputs
  if (!audioEngine.initialize()) {
    juce::Logger::writeToLog(
        "Headless Core Warning: Audio Hardware initialization issue.");
  }

  // 2. Headless Preset Recovery & Auto-Load Sequence
  PresetManager presetManager;
  juce::File presetFile =
      juce::File::getCurrentWorkingDirectory().getChildFile("preset.json");

  if (presetFile.existsAsFile() && presetManager.loadFromFile(presetFile)) {
    juce::Logger::writeToLog("Headless Core: Recovered active preset from " +
                             presetFile.getFullPathName());

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
          juce::Logger::writeToLog("Loading Layer " + juce::String(layerIdx) +
                                   " container: " + binFile.getFileName());
          SampleContainerReader::loadContainerFile(
              binFile, audioEngine.getSynth(), layerIdx);
        }
      }
    }
  } else {
    // Fallback: Scan local folder for any available .bin container (e.g.
    // TestPiano.bin)
    auto binFiles = juce::File::getCurrentWorkingDirectory().findChildFiles(
        juce::File::findFiles, false, "*.bin");
    if (!binFiles.isEmpty()) {
      juce::Logger::writeToLog(
          "Headless Core: Auto-loading default container -> " +
          binFiles[0].getFileName());
      SampleContainerReader::loadContainerFile(binFiles[0],
                                               audioEngine.getSynth(), 0);
    }
  }

  // 3. Launch Network Server Daemon
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
  return 0;
}
