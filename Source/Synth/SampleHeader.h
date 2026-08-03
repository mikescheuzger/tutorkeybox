#pragma once
#include <cstdint>

#pragma pack(push, 1) // memory alignement, no padding

// Magic identifier for .bin

constexpr char TKB_Magic[4] = {'T', 'K', 'B', '1'};

// File Header location at Byte 0 of .bin

struct ContainerHeader {
  char magic[4];
  uint32_t version;
  uint32_t numSampleEntries;
};

// Sample Metadata Entry inside the bin header index

struct SampleEntry {
  uint32_t sampleID;
  char name[64];

  uint8_t layerIndex;
  uint8_t rootNote;
  uint8_t keyLow;
  uint8_t keyHigh;
  uint8_t velLow;
  uint8_t velHigh;

  uint8_t
      isReleaseSample; // 0 = normal attack/sustan sample, 1 = release sample
  uint8_t releaseVolume;

  uint64_t fileOffset;
  uint32_t attackSampleSize;
  uint32_t numChannels;     // e.g. 1 (mono) or 2 (stereo)
  uint32_t sampleRate;      // e.g. 44100 or 48000 Hz
  uint32_t totalNumSamples; // Total frame count in float array
  uint64_t rawDataSize;     // Size in bytes: totalNumSamples * numChannels *
                            // sizeof(float)
};

#pragma pack(pop)
