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

  // 4. Load the entire .bin file into memory ONCE (0 disk file descriptor exhaustion!)
  juce::MemoryBlock binData;
  if (!binFile.loadFileAsData(binData) || binData.getSize() == 0) {
    juce::Logger::writeToLog("SampleContainerReader Error: Failed to load .bin data block.");
    return false;
  }

  juce::WavAudioFormat wavFormat;

  for (const auto &entry : indexTable) {
    if (entry.fileOffset + entry.wavDataSize > binData.getSize())
      continue;

    // Create zero-copy memory stream pointing to this WAV sample block
    auto memStream = std::make_unique<juce::MemoryInputStream>(
        static_cast<const char *>(binData.getData()) + entry.fileOffset,
        entry.wavDataSize, false);

    std::unique_ptr<juce::AudioFormatReader> reader(
        wavFormat.createReaderFor(memStream.release(), true));

    if (reader != nullptr) {
      // 1. Read Full Sample Float Buffer (Clean 24-bit PCM decoding from memory!)
      juce::AudioBuffer<float> tailBuffer((int)reader->numChannels,
                                          (int)reader->lengthInSamples);
      reader->read(&tailBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

      // 2. Create Attack RAM Buffer (First 150ms slice)
      int samplesToRead =
          (entry.attackSampleSize > 0)
              ? (int)entry.attackSampleSize
              : juce::jmin((int)reader->lengthInSamples,
                           juce::roundToInt(0.150 * reader->sampleRate));

      juce::AudioBuffer<float> attackRamBuffer((int)reader->numChannels,
                                               samplesToRead);
      for (int ch = 0; ch < reader->numChannels; ++ch) {
        attackRamBuffer.copyFrom(ch, 0, tailBuffer, ch, 0, samplesToRead);
      }

      int targetLayer = (targetLayerIndex >= 0 && targetLayerIndex <= 3)
                            ? targetLayerIndex
                            : entry.layerIndex;

      // Create CustomSamplerSound object with both decoded float buffers
      juce::SynthesiserSound::Ptr sound = new CustomSamplerSound(
          entry, attackRamBuffer, tailBuffer, reader->sampleRate);

      synthTarget.addSoundToLayer(targetLayer, sound);
    }
  }

  juce::Logger::writeToLog("SampleContainerReader Success: Loaded " +
                           juce::String(header.numSampleEntries) +
                           " samples into LayeredSynth!");
  return true;
}
