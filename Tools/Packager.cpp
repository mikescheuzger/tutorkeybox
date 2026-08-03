// CORE CONCEPT: CLI entry point for the TKBPackager tool.
// Two modes:
//   Single .bin: ./TKBPackager pack  <wav_folder> <output.bin>
//   TKBLibrary:  ./TKBPackager lib   <wav_folder> <library_dir> <instrument_name>
// The 'lib' mode auto-detects mic variants and creates one .bin per variant
// in the TKBLibrary directory.
#include "../Source/Synth/SamplePackager.h"
#include <iostream>
#include <juce_gui_basics/juce_gui_basics.h>

static void printUsage() {
    std::cout
        << "TKBPackager — TutorKeyBox sample packaging tool\n\n"
        << "MODES:\n"
        << "  pack  <wav_folder_or_wav> <output.bin>\n"
        << "        Package all WAVs in a folder into a single .bin file.\n\n"
        << "  lib   <wav_folder> <tkb_library_dir> <instrument_name>\n"
        << "        Auto-detect mic variants and write one .bin per variant\n"
        << "        into the TKBLibrary directory.\n"
        << "        e.g. ./TKBPackager lib ~/UprightSamples ~/Documents/TKBLibrary UprightSamples\n\n";
}

int main(int argc, char* argv[]) {
    juce::ScopedJuceInitialiser_GUI juceInit;

    if (argc < 2) {
        printUsage();
        return 1;
    }

    juce::String mode = juce::String::fromUTF8(argv[1]).toLowerCase();

    // -------------------------------------------------------------------------
    // MODE: pack — single monolithic .bin
    // -------------------------------------------------------------------------
    if (mode == "pack") {
        if (argc < 4) {
            std::cout << "Usage: ./TKBPackager pack <wav_folder_or_wav> <output.bin>\n";
            return 1;
        }
        juce::File inputDir(juce::String::fromUTF8(argv[2]));
        juce::File outputFile(juce::String::fromUTF8(argv[3]));
        if (!inputDir.exists()) {
            std::cout << "ERROR: Input path does not exist: " << argv[2] << "\n";
            return 1;
        }
        std::cout << "Packaging (single): " << inputDir.getFullPathName().toStdString() << "\n";
        if (SamplePackager::createPackage(inputDir, outputFile)) {
            std::cout << "SUCCESS: " << outputFile.getFullPathName().toStdString() << "\n";
            return 0;
        }
        std::cout << "ERROR: Packaging failed.\n";
        return 1;
    }

    // -------------------------------------------------------------------------
    // MODE: lib — one .bin per mic variant → TKBLibrary directory
    // -------------------------------------------------------------------------
    if (mode == "lib") {
        if (argc < 5) {
            std::cout << "Usage: ./TKBPackager lib <wav_folder> <library_dir> <instrument_name>\n";
            return 1;
        }
        juce::File inputDir(juce::String::fromUTF8(argv[2]));
        juce::File libraryDir(juce::String::fromUTF8(argv[3]));
        juce::String instrName = juce::String::fromUTF8(argv[4]);

        if (!inputDir.isDirectory()) {
            std::cout << "ERROR: Input must be a directory: " << argv[2] << "\n";
            return 1;
        }

        std::cout << "Library packaging: " << inputDir.getFullPathName().toStdString() << "\n";
        std::cout << "Library output:    " << libraryDir.getFullPathName().toStdString() << "\n";
        std::cout << "Instrument name:   " << instrName.toStdString() << "\n\n";

        auto results = SamplePackager::createLibrary(inputDir, libraryDir, instrName);

        if (results.isEmpty()) {
            std::cout << "ERROR: No .bin files were created.\n";
            return 1;
        }

        std::cout << "SUCCESS — " << results.size() << " mic variant(s) packaged:\n";
        for (auto& pb : results) {
            std::cout << "  [" << pb.micVariantName.toStdString() << "] "
                      << pb.outputFile.getFileName().toStdString() << "\n";
        }
        return 0;
    }

    // Unknown mode
    std::cout << "ERROR: Unknown mode '" << mode.toStdString() << "'\n\n";
    printUsage();
    return 1;
}
