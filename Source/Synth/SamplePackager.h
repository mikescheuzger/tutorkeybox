#pragma once
#include "SampleHeader.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

// CORE CONCEPT: Describes one successfully packaged .bin file produced by
// createLibrary(), carrying the instrument name, mic variant display name,
// and the full output path so the GUI can register it in the library.
struct PackagedBin {
    juce::String instrumentName;  // e.g. "UprightSamples"
    juce::String micVariantName;  // e.g. "Neumann M49"   (human-readable)
    juce::String micVariantId;    // e.g. "NeumannM49"    (safe filename slug)
    juce::File   outputFile;      // full path to the written .bin
};

/**
 * Native engine packager utility to convert WAV folders into .bin packages.
 */
class SamplePackager {
public:
    SamplePackager() = delete;

    /**
     * Scans an input folder containing .wav files, classifies them by filename
     * convention, and compiles a single monolithic .bin container.
     *
     * @param inputWavDir  Directory (or single .wav file) to package.
     * @param outputBinFile  Target .bin container file to write.
     * @return True if successfully packed; false on error.
     */
    static bool createPackage(const juce::File& inputWavDir,
                              const juce::File& outputBinFile);

    /**
     * CORE CONCEPT: Scans a WAV folder, auto-detects all mic variants, and
     * writes one .bin per mic variant into the TKBLibrary directory.
     * The output filenames follow the convention:
     *   {instrumentName}_{micVariantId}.bin
     * e.g. UprightSamples_NeumannM49.bin
     *
     * @param inputWavDir      Source folder containing multi-mic WAV files.
     * @param libraryOutputDir Target TKBLibrary folder to write .bin files into.
     * @param instrumentName   Short name for this instrument (used in filename).
     * @return Array of PackagedBin descriptors, one per mic variant written.
     */
    static juce::Array<PackagedBin> createLibrary(const juce::File& inputWavDir,
                                                   const juce::File& libraryOutputDir,
                                                   const juce::String& instrumentName);
};
