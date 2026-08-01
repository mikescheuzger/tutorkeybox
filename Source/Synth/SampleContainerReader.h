#pragma once
#include "CustomSamplerSound.h"
#include "LayeredSynth.h"
#include "SampleHeader.h"
#include <juce_audio_formats/juce_audio_formats.h>

/**
 * Utility class to parse .bin sample container packages and load sound presets.
 */
class SampleContainerReader {
public:
  SampleContainerReader() =
      delete; // Static utility class, cannot be instantiated

  static bool loadContainerFile(const juce::File &binFile,
                                LayeredSynth &synthTarget,
                                int targetLayerIndex = -1);
};
