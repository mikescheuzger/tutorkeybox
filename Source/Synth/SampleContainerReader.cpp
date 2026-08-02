#include "SampleContainerReader.h"

bool SampleContainerReader::loadContainerFile(const juce::File &binFile,
                                              LayeredSynth &synthTarget,
                                              int targetLayerIndex) {
  if (!binFile.existsAsFile()) {
    juce::Logger::writeToLog(
        "SampleContainerReader Error: File does not exist - " +
        binFile.getFullPathName());
    return false;
  }

  juce::FileInputStream stream(binFile);
  if (!stream.openedOk()) {
    juce::Logger::writeToLog(
        "SampleContainerReader Error: Failed to open stream for " +
        binFile.getFullPathName());
    return false;
  }

  // 1. Read Container Header at Byte 0
  ContainerHeader header{};
  if (stream.read(&header, sizeof(ContainerHeader)) !=
      sizeof(ContainerHeader)) {
    juce::Logger::writeToLog(
        "SampleContainerReader Error: Failed to read container header.");
    return false;
  }

  // 2. Validate "TKB1" Magic Bytes
  if (header.magic[0] != TKB_Magic[0] || header.magic[1] != TKB_Magic[1] ||
      header.magic[2] != TKB_Magic[2] || header.magic[3] != TKB_Magic[3]) {
    juce::Logger::writeToLog("SampleContainerReader Error: Invalid magic "
                             "signature! File is not a .bin container.");
    return false;
  }

  // 3. Read Index Table Entries
  std::vector<SampleEntry> indexTable;
  indexTable.reserve(header.numSampleEntries);

  for (uint32_t i = 0; i < header.numSampleEntries; ++i) {
    SampleEntry entry{};
    if (stream.read(&entry, sizeof(SampleEntry)) != sizeof(SampleEntry)) {
      juce::Logger::writeToLog(
          "SampleContainerReader Error: Corrupted index table entry.");
      return false;
    }
    indexTable.push_back(entry);
  }

  // 4. Memory-map the container file for zero-copy tail streaming
  auto memoryMap = std::make_shared<juce::MemoryMappedFile>(
      binFile, juce::MemoryMappedFile::readOnly);
  juce::WavAudioFormat wavFormat;
  for (const auto &entry : indexTable) {
    // Seek to the exact byte offset of the WAV sample inside the .bin file
    stream.setPosition(entry.fileOffset);
    // Create a sub-region stream pointing to this WAV data block
    auto subStream = std::make_unique<juce::SubregionStream>(
        new juce::FileInputStream(binFile), entry.fileOffset, entry.wavDataSize,
        true);
    std::unique_ptr<juce::AudioFormatReader> reader(
        wavFormat.createReaderFor(subStream.release(), true));
    if (reader != nullptr) {
      int samplesToRead =
          (entry.attackSampleSize > 0)
              ? (int)entry.attackSampleSize
              : juce::jmin((int)reader->lengthInSamples,
                           juce::roundToInt(0.150 * reader->sampleRate));
      juce::AudioBuffer<float> attackRamBuffer((int)reader->numChannels,
                                               samplesToRead);
      reader->read(&attackRamBuffer, 0, samplesToRead, 0, true, true);
      int targetLayer = (targetLayerIndex >= 0 && targetLayerIndex <= 3)
                            ? targetLayerIndex
                            : entry.layerIndex;
      // Create CustomSamplerSound object with memoryMap reference
      juce::SynthesiserSound::Ptr sound = new CustomSamplerSound(
          entry, attackRamBuffer, reader->sampleRate, memoryMap);
      synthTarget.addSoundToLayer(targetLayer, sound);
    }
  }

  juce::Logger::writeToLog("SampleContainerReader Success: Loaded " +
                           juce::String(header.numSampleEntries) +
                           " samples into LayeredSynth!");
  return true;
}
