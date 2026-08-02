#pragma once
#include "SampleHeader.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

/**
 * Native engine packager utility to convert WAV folders into .bin packages.
 */
class SamplePackager {
public:
  SamplePackager() = delete;

  /**
   * Scans an input folder containing .wav files, calculates ~150ms
   * zero-crossings, and compiles a monolithic .bin container package file.
   *
   * @param inputWavDir Directory containing .wav files.
   * @param outputBinFile Target .bin container file to write.
   * @return True if successfully packed; false on error.
   */
  static bool createPackage(const juce::File &inputWavDir,
                            const juce::File &outputBinFile);
};
