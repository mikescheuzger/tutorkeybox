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

  // 1. Zero-copy Memory Map the container file instantly (< 0.1ms!)
  auto mappedFile = std::make_shared<juce::MemoryMappedFile>(
      binFile, juce::MemoryMappedFile::readOnly);

  if (mappedFile->getSize() < (int64_t)sizeof(ContainerHeader)) {
    juce::Logger::writeToLog(
        "SampleContainerReader Error: Memory mapping failed or file empty - " +
        binFile.getFullPathName());
    return false;
  }

  const char *baseDataPtr = static_cast<const char *>(mappedFile->getData());

  // 2. Read Container Header at Byte 0 directly from mmap pointer
  const auto *header = reinterpret_cast<const ContainerHeader *>(baseDataPtr);

  // 3. Validate "TKB1" Magic Bytes
  if (header->magic[0] != TKB_Magic[0] || header->magic[1] != TKB_Magic[1] ||
      header->magic[2] != TKB_Magic[2] || header->magic[3] != TKB_Magic[3]) {
    juce::Logger::writeToLog("SampleContainerReader Error: Invalid magic "
                             "signature! File is not a .bin container.");
    return false;
  }

  // 4. Index table entries location in mmap pointer
  const auto *indexTable = reinterpret_cast<const SampleEntry *>(
      baseDataPtr + sizeof(ContainerHeader));

  for (uint32_t i = 0; i < header->numSampleEntries; ++i) {
    const auto &entry = indexTable[i];
    if (entry.fileOffset + entry.rawDataSize > (size_t)mappedFile->getSize())
      continue;

    // 5. Direct pointer to raw 32-bit Float PCM array in memory mapping
    const float *rawFloatPtr =
        reinterpret_cast<const float *>(baseDataPtr + entry.fileOffset);

    int numChannels = (int)entry.numChannels;
    int numSamples = (int)entry.totalNumSamples;
    double sampleRate = (double)entry.sampleRate;

    // Build channel pointers array into mmap data
    std::vector<const float *> tailChannelPointers((size_t)numChannels);
    for (int ch = 0; ch < numChannels; ++ch) {
      tailChannelPointers[(size_t)ch] = rawFloatPtr + (ch * numSamples);
    }

    // 6. Create Attack RAM Buffer (First ~150ms slice pre-cached in RAM for 0ms
    // latency)
    int samplesToRead =
        (entry.attackSampleSize > 0)
            ? (int)entry.attackSampleSize
            : juce::jmin(numSamples, juce::roundToInt(0.150 * sampleRate));

    juce::AudioBuffer<float> attackRamBuffer(numChannels, samplesToRead);
    for (int ch = 0; ch < numChannels; ++ch) {
      attackRamBuffer.copyFrom(ch, 0, tailChannelPointers[(size_t)ch],
                               samplesToRead);
    }

    int targetLayer = (targetLayerIndex >= 0 && targetLayerIndex <= 3)
                          ? targetLayerIndex
                          : entry.layerIndex;

    // Create CustomSamplerSound with mmap tail pointers and RAM attack buffer
    juce::SynthesiserSound::Ptr sound =
        new CustomSamplerSound(entry, attackRamBuffer, tailChannelPointers,
                               numSamples, sampleRate, mappedFile);

    synthTarget.addSoundToLayer(targetLayer, sound);
  }

  juce::Logger::writeToLog("SampleContainerReader Success: Instantly loaded " +
                           juce::String(header->numSampleEntries) +
                           " samples via mmap into LayeredSynth!");
  return true;
}
