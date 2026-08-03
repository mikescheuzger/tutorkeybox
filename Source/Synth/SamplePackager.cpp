#include "SamplePackager.h"
#include <algorithm>
#include <vector>

// Helper to parse note names like A0, C1, D#3, Eb4 from filenames
static int parseMidiNoteFromName(const juce::String &filename) {
  juce::String name = filename.trim();
  int noteInOctave = -1;
  int index = 0;

  if (name.isEmpty())
    return 60;

  char noteChar = std::toupper(name[index]);
  switch (noteChar) {
  case 'C':
    noteInOctave = 0;
    break;
  case 'D':
    noteInOctave = 2;
    break;
  case 'E':
    noteInOctave = 4;
    break;
  case 'F':
    noteInOctave = 5;
    break;
  case 'G':
    noteInOctave = 7;
    break;
  case 'A':
    noteInOctave = 9;
    break;
  case 'B':
    noteInOctave = 11;
    break;
  default:
    return 60; // Default Middle C if no note letter found
  }
  index++;

  if (index < name.length()) {
    if (name[index] == '#' || name[index] == 's' || name[index] == 'S') {
      noteInOctave += 1;
      index++;
    } else if (name[index] == 'b') {
      if (index + 1 < name.length() &&
          (std::isdigit(name[index + 1]) || name[index + 1] == '-')) {
        noteInOctave -= 1;
        index++;
      }
    }
  }

  juce::String octaveStr;
  while (index < name.length() &&
         (std::isdigit(name[index]) || name[index] == '-')) {
    octaveStr += name[index];
    index++;
  }

  int octave = octaveStr.isNotEmpty() ? octaveStr.getIntValue() : 4;
  int midiNote = (octave + 1) * 12 + noteInOctave;
  return juce::jlimit(0, 127, midiNote);
}

// Helper to parse velocity layer v1 through v16 from filenames
static void parseVelocityRange(const juce::String &filename, uint8_t &velLow,
                               uint8_t &velHigh) {
  int vIdx = filename.lastIndexOfIgnoreCase("v");
  if (vIdx >= 0 && vIdx + 1 < filename.length()) {
    int layerNum = filename.substring(vIdx + 1).getIntValue();
    if (layerNum >= 1 && layerNum <= 16) {
      velLow = (uint8_t)((layerNum - 1) * 8);
      velHigh = (uint8_t)(layerNum == 16 ? 127 : (layerNum * 8 - 1));
      return;
    }
  }
  velLow = 0;
  velHigh = 127;
}

static int findZeroCrossing(const juce::AudioBuffer<float> &buffer,
                            int targetSampleIndex) {
  const int numSamples = buffer.getNumSamples();
  if (targetSampleIndex >= numSamples - 1)
    return numSamples;
  const float *samples = buffer.getReadPointer(0);
  int searchWindow = juce::jmin(1000, numSamples - targetSampleIndex - 2);
  for (int offset = 0; offset < searchWindow; ++offset) {
    int idx = targetSampleIndex + offset;
    if (samples[idx] * samples[idx + 1] <= 0.0f)
      return idx;
  }
  return targetSampleIndex;
}

bool SamplePackager::createPackage(const juce::File &inputWavDir,
                                   const juce::File &outputBinFile) {
  juce::Array<juce::File> wavFiles;
  if (inputWavDir.isDirectory()) {
    wavFiles = inputWavDir.findChildFiles(juce::File::findFiles, true, "*.wav");
  } else if (inputWavDir.getFileExtension().equalsIgnoreCase(".wav")) {
    wavFiles.add(inputWavDir);
  }
  if (wavFiles.isEmpty())
    return false;
  juce::WavAudioFormat wavFormat;
  std::vector<SampleEntry> entries;
  std::vector<juce::MemoryBlock> floatDataBlocks;
  uint64_t currentOffset =
      sizeof(ContainerHeader) + (wavFiles.size() * sizeof(SampleEntry));
  for (int i = 0; i < wavFiles.size(); ++i) {
    const auto &wavFile = wavFiles[i];
    std::unique_ptr<juce::AudioFormatReader> reader(
        wavFormat.createReaderFor(wavFile.createInputStream().release(), true));
    if (reader == nullptr)
      continue;
    // 1. Read entire WAV file into normalized 32-bit float AudioBuffer
    int numSamples = (int)reader->lengthInSamples;
    int numChannels = (int)reader->numChannels;
    juce::AudioBuffer<float> fullSampleBuffer(numChannels, numSamples);
    reader->read(&fullSampleBuffer, 0, numSamples, 0, true, true);
    // 2. Find zero crossing for 150ms attack buffer
    int target150ms = juce::roundToInt(0.150 * reader->sampleRate);
    int zeroCrossingIndex = findZeroCrossing(fullSampleBuffer, target150ms);
    // 3. Convert float AudioBuffer into raw memory block (channel data
    // sequential)
    juce::MemoryBlock floatBlock;
    size_t rawBytes = (size_t)(numChannels * numSamples * sizeof(float));
    floatBlock.setSize(rawBytes, false);
    float *destPtr = reinterpret_cast<float *>(floatBlock.getData());
    for (int ch = 0; ch < numChannels; ++ch) {
      std::memcpy(destPtr + (ch * numSamples),
                  fullSampleBuffer.getReadPointer(ch),
                  numSamples * sizeof(float));
    }
    // 4. Populate updated SampleEntry metadata
    SampleEntry entry{};
    entry.sampleID = (uint32_t)(i + 1);
    juce::String nameStr = wavFile.getFileNameWithoutExtension();
    nameStr.copyToUTF8(entry.name, sizeof(entry.name) - 1);
    entry.layerIndex = 0;
    entry.rootNote = (uint8_t)parseMidiNoteFromName(nameStr);
    // Parse Velocity Layer (e.g. v1 = 0-7, v16 = 120-127)
    parseVelocityRange(nameStr, entry.velLow, entry.velHigh);
    entry.isReleaseSample =
        nameStr.containsIgnoreCase("rel") ? (uint8_t)1 : (uint8_t)0;
    entry.releaseVolume = 127;
    entry.fileOffset = currentOffset;
    entry.attackSampleSize = (uint32_t)zeroCrossingIndex;
    entry.numChannels = (uint32_t)numChannels;
    entry.sampleRate = (uint32_t)reader->sampleRate;
    entry.totalNumSamples = (uint32_t)numSamples;
    entry.rawDataSize = (uint64_t)rawBytes;
    entries.push_back(entry);
    floatDataBlocks.push_back(floatBlock);
    currentOffset += rawBytes;
  }
  // 1. Extract unique pitch root notes for non-release samples
  std::vector<uint8_t> uniqueRoots;
  for (const auto &entry : entries) {
    if (entry.isReleaseSample == 0 &&
        std::find(uniqueRoots.begin(), uniqueRoots.end(), entry.rootNote) ==
            uniqueRoots.end()) {
      uniqueRoots.push_back(entry.rootNote);
    }
  }
  std::sort(uniqueRoots.begin(), uniqueRoots.end());
  // 2. Calculate Key Zones based on adjacent pitch roots
  for (auto &entry : entries) {
    if (entry.isReleaseSample != 0) {
      entry.keyLow = entry.rootNote;
      entry.keyHigh = entry.rootNote;
      continue;
    }
    auto it = std::find(uniqueRoots.begin(), uniqueRoots.end(), entry.rootNote);
    if (it != uniqueRoots.end()) {
      size_t idx = std::distance(uniqueRoots.begin(), it);
      uint8_t prevRoot = (idx == 0) ? 0 : uniqueRoots[idx - 1];
      entry.keyLow = (idx == 0) ? 0 : (uint8_t)(prevRoot + 1);
      entry.keyHigh = entry.rootNote;
    }
  }
  outputBinFile.deleteFile();
  juce::FileOutputStream outStream(outputBinFile);
  if (!outStream.openedOk())
    return false;
  ContainerHeader containerHeader{};
  containerHeader.magic[0] = TKB_Magic[0];
  containerHeader.magic[1] = TKB_Magic[1];
  containerHeader.magic[2] = TKB_Magic[2];
  containerHeader.magic[3] = TKB_Magic[3];
  containerHeader.version = 1;
  containerHeader.numSampleEntries = (uint32_t)entries.size();
  outStream.write(&containerHeader, sizeof(ContainerHeader));
  for (const auto &entry : entries)
    outStream.write(&entry, sizeof(SampleEntry));
  for (const auto &block : floatDataBlocks)
    outStream.write(block.getData(), block.getSize());
  return true;
}