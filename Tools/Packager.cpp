#include "../Source/Synth/SampleHeader.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Finds the nearest Zero-Crossing sample index near targetSampleIndex (~150ms)
 * to prevent audio clicks when truncating attack buffers.
 */
static int findZeroCrossing(const juce::AudioBuffer<float> &buffer,
                            int targetSampleIndex) {
  const int numSamples = buffer.getNumSamples();
  if (targetSampleIndex >= numSamples - 1)
    return numSamples;

  const float *samples = buffer.getReadPointer(0); // Check Channel 0
  int searchWindow = juce::jmin(1000, numSamples - targetSampleIndex - 2);

  for (int offset = 0; offset < searchWindow; ++offset) {
    int idx = targetSampleIndex + offset;
    // Zero-crossing condition: waveform changes sign from positive to negative
    // (or vice versa)
    if (samples[idx] * samples[idx + 1] <= 0.0f) {
      return idx;
    }
  }
  return targetSampleIndex; // Fallback if no zero crossing found in window
}

int main(int argc, char *argv[]) {
  juce::ScopedJuceInitialiser_GUI juceInit; // Initialize JUCE core environment

  juce::ConsoleApplication app;

  std::cout << "===================================================="
            << std::endl;
  std::cout << "   TUTOR KEYBOX :: C++ SAMPLE CONTAINER PACKAGER    "
            << std::endl;
  std::cout << "===================================================="
            << std::endl;

  if (argc < 3) {
    std::cout << "Usage: ./TKBPackager <input_wav_folder> <output_bin_file>"
              << std::endl;
    return 1;
  }

  juce::File inputDir(argv[1]);
  juce::File outputFile(argv[2]);

  if (!inputDir.isDirectory()) {
    std::cout << "Error: Input path is not a directory: "
              << inputDir.getFullPathName().toStdString() << std::endl;
    return 1;
  }

  // Find all .wav files inside the input directory
  auto wavFiles =
      inputDir.findChildFiles(juce::File::findFiles, false, "*.wav");
  if (wavFiles.isEmpty()) {
    std::cout << "Error: No .wav files found in directory!" << std::endl;
    return 1;
  }

  std::cout << "Found " << wavFiles.size() << " WAV sample files to pack."
            << std::endl;

  // Register WAV format reader
  juce::WavAudioFormat wavFormat;

  std::vector<SampleEntry> entries;
  std::vector<juce::MemoryBlock> wavDataBlocks;

  uint64_t currentOffset =
      sizeof(ContainerHeader) + (wavFiles.size() * sizeof(SampleEntry));

  for (int i = 0; i < wavFiles.size(); ++i) {
    const auto &wavFile = wavFiles[i];
    std::unique_ptr<juce::AudioFormatReader> reader(
        wavFormat.createReaderFor(wavFile.createInputStream().release(), true));

    if (reader == nullptr) {
      std::cout << "Warning: Skipping unreadable file "
                << wavFile.getFileName().toStdString() << std::endl;
      continue;
    }

    // Read entire raw WAV file into memory block
    juce::MemoryBlock block;
    wavFile.loadFileAsData(block);

    // Pre-load attack buffer & find Zero-Crossing near ~150ms
    int target150ms = juce::roundToInt(0.150 * reader->sampleRate);
    juce::AudioBuffer<float> tempBuffer(
        (int)reader->numChannels,
        juce::jmin((int)reader->lengthInSamples, target150ms + 1050));
    reader->read(&tempBuffer, 0, tempBuffer.getNumSamples(), 0, true, true);

    int zeroCrossingIndex = findZeroCrossing(tempBuffer, target150ms);

    // Populate SampleEntry metadata
    SampleEntry entry{};
    entry.sampleID = (uint32_t)(i + 1);

    juce::String nameStr = wavFile.getFileNameWithoutExtension();
    nameStr.copyToUTF8(entry.name, sizeof(entry.name) - 1);

    entry.layerIndex = 0; // Default Layer 0
    entry.rootNote = 60;  // Default Middle C (User can customize mapping)
    entry.keyLow = 0;
    entry.keyHigh = 127;
    entry.velLow = 0;
    entry.velHigh = 127;
    entry.isReleaseSample = 0;
    entry.releaseVolume = 127;

    entry.fileOffset = currentOffset;
    entry.wavDataSize = block.getSize();
    entry.attackSampleSize = (uint32_t)zeroCrossingIndex;

    entries.push_back(entry);
    wavDataBlocks.push_back(block);

    currentOffset += block.getSize();

    std::cout << " Packed [" << (i + 1) << "/" << wavFiles.size()
              << "]: " << entry.name << " (Zero-Crossing at sample "
              << zeroCrossingIndex << ")" << std::endl;
  }

  // Write .bin package file to disk
  outputFile.deleteFile();
  juce::FileOutputStream outStream(outputFile);

  if (!outStream.openedOk()) {
    std::cout << "Error: Failed to open output file for writing!" << std::endl;
    return 1;
  }

  // 1. Write Container Header
  ContainerHeader containerHeader{};
  containerHeader.magic[0] = TKB_Magic[0];
  containerHeader.magic[1] = TKB_Magic[1];
  containerHeader.magic[2] = TKB_Magic[2];
  containerHeader.magic[3] = TKB_Magic[3];
  containerHeader.version = 1;
  containerHeader.numSampleEntries = (uint32_t)entries.size();

  outStream.write(&containerHeader, sizeof(ContainerHeader));

  // 2. Write SampleEntry Index Table
  for (const auto &entry : entries) {
    outStream.write(&entry, sizeof(SampleEntry));
  }

  // 3. Write PCM Audio Blocks
  for (const auto &block : wavDataBlocks) {
    outStream.write(block.getData(), block.getSize());
  }

  outStream.flush();

  std::cout << "\nSUCCESS: Created package "
            << outputFile.getFullPathName().toStdString() << " ("
            << (outStream.getPosition() / (1024 * 1024)) << " MB)" << std::endl;

  return 0;
}
