#include "LibraryManager.h"

// ==============================================================================
// LibraryManager implementation
// ==============================================================================

LibraryManager::LibraryManager() {
    libraryRoot = getDefaultLibraryRoot();
    rescan();
}

// CORE CONCEPT: Chooses the platform-appropriate default library path.
// On Mac this is ~/Documents/TKBLibrary, on Linux (RasPi) ~/tkblibrary.
juce::File LibraryManager::getDefaultLibraryRoot() {
#if JUCE_MAC
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
               .getChildFile("TKBLibrary");
#elif JUCE_LINUX
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
               .getChildFile("tkblibrary");
#else
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
               .getChildFile("TKBLibrary");
#endif
}

void LibraryManager::setLibraryRoot(const juce::File& dir) {
    libraryRoot = dir;
    rescan();
}

// CORE CONCEPT: Scans the library root for all .bin files, parses each one
// into a LibraryEntry, and rebuilds the entries list. Called on startup and
// after packaging. Non-bin files (preset JSONs etc.) are ignored.
void LibraryManager::rescan() {
    entries.clear();
    if (!libraryRoot.isDirectory()) return;

    auto binFiles = libraryRoot.findChildFiles(juce::File::findFiles, false, "*.bin");
    for (auto& f : binFiles) {
        LibraryEntry entry = entryFromFile(f);
        if (entry.binFileName.isNotEmpty())
            entries.push_back(entry);
    }

    // Sort alphabetically by display name for predictable GUI ordering
    std::sort(entries.begin(), entries.end(), [](const LibraryEntry& a, const LibraryEntry& b) {
        return a.getDisplayName() < b.getDisplayName();
    });

    juce::Logger::writeToLog("LibraryManager: Scanned " + juce::String(entries.size()) +
                             " instruments in " + libraryRoot.getFullPathName());
}

// CORE CONCEPT: Parses a .bin filename using the convention
//   {InstrumentName}_{MicVariantId}.bin
// The last underscore-separated token is treated as the mic variant ID.
// Everything before it is the instrument name.
// Example: "UprightSamples_NeumannM49.bin" → instr="UprightSamples", mic="NeumannM49"
// Single-part names (no underscore) → instr=name, mic="Default"
LibraryEntry LibraryManager::entryFromFile(const juce::File& binFile) {
    LibraryEntry e;
    e.binFile     = binFile;
    e.binFileName = binFile.getFileName();

    juce::String stem = binFile.getFileNameWithoutExtension();

    // Split at the LAST underscore to separate instrument from mic slug
    int lastUnder = stem.lastIndexOfChar('_');
    if (lastUnder > 0) {
        e.instrumentName = stem.substring(0, lastUnder);
        e.micVariantId   = stem.substring(lastUnder + 1);
        // Convert slug back to readable: "NeumannM49" → "Neumann M49"
        // Heuristic: insert space before each uppercase letter that follows a lowercase
        juce::String readable;
        for (int i = 0; i < e.micVariantId.length(); ++i) {
            char c = e.micVariantId[i];
            if (i > 0 && std::isupper((unsigned char)c) &&
                std::islower((unsigned char)e.micVariantId[i - 1])) {
                readable += ' ';
            }
            readable += c;
        }
        e.micVariantName = readable;
    } else {
        e.instrumentName = stem;
        e.micVariantId   = "Default";
        e.micVariantName = "Default";
    }
    return e;
}

bool LibraryManager::parseLibraryFilename(const juce::String& binFilename,
                                           juce::String& outInstrumentName,
                                           juce::String& outMicVariantId) {
    juce::String stem = juce::File(binFilename).getFileNameWithoutExtension();
    int lastUnder = stem.lastIndexOfChar('_');
    if (lastUnder > 0) {
        outInstrumentName = stem.substring(0, lastUnder);
        outMicVariantId   = stem.substring(lastUnder + 1);
        return true;
    }
    outInstrumentName = stem;
    outMicVariantId   = "Default";
    return false;
}

std::vector<LibraryEntry> LibraryManager::getEntriesForInstrument(
        const juce::String& name) const {
    std::vector<LibraryEntry> result;
    for (auto& e : entries) {
        if (e.instrumentName.equalsIgnoreCase(name))
            result.push_back(e);
    }
    return result;
}

juce::StringArray LibraryManager::getInstrumentNames() const {
    juce::StringArray names;
    for (auto& e : entries) {
        if (!names.contains(e.instrumentName))
            names.add(e.instrumentName);
    }
    return names;
}

// CORE CONCEPT: Resolves a bare .bin filename to a full path under the current
// library root. If the file doesn't exist locally (e.g. not yet transferred to
// RasPi), returns an invalid File so the caller can prompt to deploy.
juce::File LibraryManager::resolveFilename(const juce::String& binFilename) const {
    juce::File candidate = libraryRoot.getChildFile(binFilename);
    return candidate.existsAsFile() ? candidate : juce::File();
}

// CORE CONCEPT: Registers freshly packaged bins from SamplePackager::createLibrary()
// directly into the entry list — avoids a full rescan after packaging.
void LibraryManager::registerPackagedBins(const juce::Array<PackagedBin>& bins) {
    for (auto& pb : bins) {
        LibraryEntry e;
        e.instrumentName = pb.instrumentName;
        e.micVariantName = pb.micVariantName;
        e.micVariantId   = pb.micVariantId;
        e.binFileName    = pb.outputFile.getFileName();
        e.binFile        = pb.outputFile;
        entries.push_back(e);
    }
    // Keep sorted
    std::sort(entries.begin(), entries.end(), [](const LibraryEntry& a, const LibraryEntry& b) {
        return a.getDisplayName() < b.getDisplayName();
    });
}
