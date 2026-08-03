#pragma once
#include "../Synth/SamplePackager.h"
#include <juce_core/juce_core.h>
#include <vector>

// ==============================================================================
// Library Entry — one row in the library browser
// ==============================================================================

// CORE CONCEPT: Represents one loadable .bin instrument in the TKBLibrary.
// The GUI populates its instrument dropdown from these entries.
struct LibraryEntry {
    juce::String instrumentName;  // e.g. "UprightSamples"
    juce::String micVariantName;  // e.g. "Neumann M49"   (human-readable display)
    juce::String micVariantId;    // e.g. "NeumannM49"    (slug used in filename)
    juce::String binFileName;     // e.g. "UprightSamples_NeumannM49.bin"
    juce::File   binFile;         // full path on this machine

    // CORE CONCEPT: Returns the display string shown in the GUI dropdown:
    // "UprightSamples — Neumann M49"
    juce::String getDisplayName() const {
        return instrumentName + "  \u2014  " + micVariantName;
    }
};

// ==============================================================================
// LibraryManager
// ==============================================================================

// CORE CONCEPT: Manages the TKBLibrary folder — a single known directory that
// contains all packaged .bin files for every instrument and mic variant.
// Both the Mac GUI and the RasPi daemon refer to .bin files by basename only;
// LibraryManager resolves basenames to full paths using the platform-local
// library root. This makes the preset JSON fully portable.
class LibraryManager {
public:
    LibraryManager();
    ~LibraryManager() = default;

    // CORE CONCEPT: Returns the platform-default library root path.
    // Mac:   ~/Documents/TKBLibrary/
    // RasPi: ~/tkblibrary/  (or /opt/tkblibrary/ if system-wide)
    static juce::File getDefaultLibraryRoot();

    // CORE CONCEPT: Sets the active library root and rescans it immediately.
    void setLibraryRoot(const juce::File& dir);
    juce::File getLibraryRoot() const { return libraryRoot; }

    // CORE CONCEPT: Rescans the library folder for .bin files and rebuilds
    // the entries list. Called on startup and after packaging new instruments.
    void rescan();

    // Returns all available instruments + mic variants found in the library.
    const std::vector<LibraryEntry>& getEntries() const { return entries; }

    // Returns entries filtered by instrument name.
    std::vector<LibraryEntry> getEntriesForInstrument(const juce::String& name) const;

    // Returns all unique instrument names found in the library.
    juce::StringArray getInstrumentNames() const;

    // CORE CONCEPT: Resolves a bare filename (e.g. "UprightSamples_NeumannM49.bin")
    // to a full path under the current library root. Returns File() if not found.
    juce::File resolveFilename(const juce::String& binFilename) const;

    // CORE CONCEPT: Registers freshly packaged bins (from SamplePackager::createLibrary)
    // into the library without a full rescan.
    void registerPackagedBins(const juce::Array<PackagedBin>& bins);

    // CORE CONCEPT: Parses an instrument name and mic variant ID from a .bin
    // filename that follows the convention {InstrumentName}_{MicVariantId}.bin
    static bool parseLibraryFilename(const juce::String& binFilename,
                                     juce::String& outInstrumentName,
                                     juce::String& outMicVariantId);

private:
    juce::File              libraryRoot;
    std::vector<LibraryEntry> entries;

    // CORE CONCEPT: Parses a .bin filename into a LibraryEntry, extracting
    // the instrument name and mic variant from the naming convention.
    static LibraryEntry entryFromFile(const juce::File& binFile);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryManager)
};
