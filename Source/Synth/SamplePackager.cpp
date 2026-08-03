#include "SamplePackager.h"
#include <algorithm>
#include <vector>

// ==============================================================================
// Internal filename parsing helpers
// ==============================================================================

// CORE CONCEPT: Parses a note token like "C3", "Eb4", "Gb2", "A-1" into
// a MIDI note number. Handles flats (b), the convention A-1 = MIDI 9, etc.
static int parseMidiNoteFromToken(const juce::String& token) {
    if (token.isEmpty()) return 60;

    int index = 0;
    int noteInOctave = -1;
    char noteChar = (char)std::toupper((unsigned char)token[index]);

    switch (noteChar) {
        case 'C': noteInOctave = 0;  break;
        case 'D': noteInOctave = 2;  break;
        case 'E': noteInOctave = 4;  break;
        case 'F': noteInOctave = 5;  break;
        case 'G': noteInOctave = 7;  break;
        case 'A': noteInOctave = 9;  break;
        case 'B': noteInOctave = 11; break;
        default:  return 60;
    }
    index++;

    // Accidentals: b for flat (Eb, Gb), # or s for sharp
    if (index < token.length()) {
        char acc = token[index];
        if (acc == '#' || acc == 's' || acc == 'S') { noteInOctave += 1; index++; }
        else if (acc == 'b') {
            // "b" is flat only if followed by digit or '-' (not end of string alone)
            if (index + 1 < token.length() &&
                (std::isdigit((unsigned char)token[index + 1]) || token[index + 1] == '-')) {
                noteInOctave -= 1;
                index++;
            }
        }
    }

    // Octave number (may be negative e.g. A-1)
    juce::String octStr;
    while (index < token.length() &&
           (std::isdigit((unsigned char)token[index]) || token[index] == '-')) {
        octStr += token[index++];
    }

    int octave = octStr.isNotEmpty() ? octStr.getIntValue() : 4;
    return juce::jlimit(0, 127, (octave + 1) * 12 + noteInOctave);
}

// CORE CONCEPT: Classifies a WAV filename into one of the 5 SampleTypes
// and extracts: rootNote, dynamic, roundRobinIndex, micVariant, and a
// human-readable name — all by parsing the Upright Samples filename convention:
// {mic}_{dyn}_{note}_rr{n}_{type}.wav
// e.g. Neumann M49_MF_C3_rr2_nopedal.wav
//      Neumann M49_PEDAL_DOWN_E3_rr1_pedal_down.wav
//      Neumann M49_RT_C3_rr1_rt.wav
struct ParsedSampleInfo {
    int          rootMidi       { 60 };
    SampleType   type           { SampleType::Normal };
    DynamicLayer dynamic        { DynamicLayer::Any };
    int          rrIndex        { 0 };
    int          micVariant     { 0 };
    bool         valid          { false };
    juce::String humanName;
};

static ParsedSampleInfo parseUprightFilename(const juce::String& stem,
                                              const juce::StringArray& knownMics) {
    // Tokenise by underscore
    juce::StringArray parts;
    parts.addTokens(stem, "_", "");

    ParsedSampleInfo info;
    if (parts.size() < 3) return info;

    // -----------------------------------------------------------------------
    // 1. Detect microphone variant by matching known mic name prefixes
    // -----------------------------------------------------------------------
    // The mic name may contain spaces and underscores split across several
    // tokens (e.g. "Neumann M49" → two tokens). We try to greedily match
    // from the start of the token list.
    int partsConsumedByMic = 0;
    for (int m = 0; m < knownMics.size(); ++m) {
        juce::StringArray micParts;
        micParts.addTokens(knownMics[m], "_", "");
        bool match = true;
        if (micParts.size() > parts.size()) { match = false; }
        else {
            for (int mp = 0; mp < micParts.size(); ++mp) {
                if (!parts[mp].equalsIgnoreCase(micParts[mp])) { match = false; break; }
            }
        }
        if (match) {
            info.micVariant = m;
            partsConsumedByMic = micParts.size();
            break;
        }
    }

    // Remaining tokens after the mic name
    juce::StringArray rest;
    for (int i = partsConsumedByMic; i < parts.size(); ++i)
        rest.add(parts[i]);

    if (rest.size() < 2) return info;

    // -----------------------------------------------------------------------
    // 2. Detect sample type — check last token AND last-two-tokens-joined
    //    so that "pedal_down" split across two tokens is handled correctly.
    // -----------------------------------------------------------------------
    juce::String lastToken       = rest.strings.getLast().toLowerCase();
    juce::String lastTwoJoined   = (rest.size() >= 2)
        ? (rest[rest.size()-2] + "_" + rest.strings.getLast()).toLowerCase()
        : lastToken;

    if      (lastToken == "nopedal")                          info.type = SampleType::Normal;
    else if (lastToken == "withpedal")                        info.type = SampleType::WithPedal;
    else if (lastToken == "rt")                               info.type = SampleType::ReleaseTail;
    else if (lastToken == "down" && lastTwoJoined.contains("pedal_down")) {
        info.type = SampleType::PedalDown;
        rest.remove(rest.size() - 1); // remove "down"
    }
    else if (lastToken == "up"   && lastTwoJoined.contains("pedal_up")) {
        info.type = SampleType::PedalUp;
        rest.remove(rest.size() - 1); // remove "up"
    }
    else if (lastTwoJoined == "pedal_down")                   info.type = SampleType::PedalDown;
    else if (lastTwoJoined == "pedal_up")                     info.type = SampleType::PedalUp;
    else return info; // Unrecognised — skip

    // Remove the type token(s) from the end (for single-token types)
    if (info.type != SampleType::PedalDown && info.type != SampleType::PedalUp ||
        rest.strings.getLast().toLowerCase() == "nopedal" ||
        rest.strings.getLast().toLowerCase() == "withpedal" ||
        rest.strings.getLast().toLowerCase() == "rt") {
        rest.remove(rest.size() - 1);
    }

    // -----------------------------------------------------------------------
    // 3. Detect and remove round-robin token (rr1, rr2, rr3…)
    // -----------------------------------------------------------------------
    info.rrIndex = 0;
    for (int i = rest.size() - 1; i >= 0; --i) {
        if (rest[i].startsWithIgnoreCase("rr")) {
            info.rrIndex = rest[i].substring(2).getIntValue() - 1; // 0-based
            if (info.rrIndex < 0) info.rrIndex = 0;
            rest.remove(i);
            break;
        }
    }

    // -----------------------------------------------------------------------
    // 4. For PEDAL_DOWN / PEDAL_UP: no note or dynamic, parse static root
    // -----------------------------------------------------------------------
    if (info.type == SampleType::PedalDown || info.type == SampleType::PedalUp) {
        // Remaining tokens: e.g. {"PEDAL", "DOWN", "E3"} or {"PEDAL_DOWN", "E3"}
        // Find the note token (contains a letter A-G)
        for (int i = 0; i < rest.size(); ++i) {
            char first = (char)std::toupper((unsigned char)rest[i][0]);
            if (first >= 'A' && first <= 'G') {
                info.rootMidi = parseMidiNoteFromToken(rest[i]);
                break;
            }
        }
        info.dynamic = DynamicLayer::Any;
        info.humanName = stem;
        info.valid = true;
        return info;
    }

    // -----------------------------------------------------------------------
    // 5. For RELEASE TAIL: remaining tokens after mic: {"RT", "C3"}
    // -----------------------------------------------------------------------
    if (info.type == SampleType::ReleaseTail) {
        // Remove the "RT" token
        for (int i = rest.size() - 1; i >= 0; --i) {
            if (rest[i].equalsIgnoreCase("RT")) { rest.remove(i); break; }
        }
        // The remaining token should be the note
        if (!rest.isEmpty()) {
            info.rootMidi = parseMidiNoteFromToken(rest.strings.getLast());
        }
        info.dynamic = DynamicLayer::Any;
        info.humanName = stem;
        info.valid = true;
        return info;
    }

    // -----------------------------------------------------------------------
    // 6. Normal / WithPedal: remaining tokens are {dyn, note}
    //    e.g. {"MF", "C3"} or {"F", "A-1"}
    // -----------------------------------------------------------------------
    if (rest.size() < 2) return info;

    juce::String dynToken  = rest[rest.size() - 2].toUpperCase();
    juce::String noteToken = rest[rest.size() - 1];

    if      (dynToken == "P")  { info.dynamic = DynamicLayer::Piano;      }
    else if (dynToken == "MF") { info.dynamic = DynamicLayer::MezzoForte; }
    else if (dynToken == "F")  { info.dynamic = DynamicLayer::Forte;      }
    else                       { info.dynamic = DynamicLayer::Any;        }

    info.rootMidi = parseMidiNoteFromToken(noteToken);
    info.humanName = dynToken + "_" + noteToken;
    info.valid = true;
    return info;
}

// CORE CONCEPT: Maps a DynamicLayer enum to the MIDI velocity zone [low, high]
// used during sample selection in LayeredSynth.
static void dynamicToVelocityRange(DynamicLayer dyn, uint8_t& velLow, uint8_t& velHigh) {
    switch (dyn) {
        case DynamicLayer::Piano:      velLow =   1; velHigh =  42; break;
        case DynamicLayer::MezzoForte: velLow =  43; velHigh =  84; break;
        case DynamicLayer::Forte:      velLow =  85; velHigh = 127; break;
        case DynamicLayer::Any:
        default:                       velLow =   0; velHigh = 127; break;
    }
}

// CORE CONCEPT: Finds the nearest zero-crossing point at or after
// targetSampleIndex to ensure clean seamless split between attack RAM buffer
// and tail buffer, preventing click artefacts.
static int findZeroCrossing(const juce::AudioBuffer<float>& buffer, int targetSampleIndex) {
    const int numSamples = buffer.getNumSamples();
    if (targetSampleIndex >= numSamples - 1) return numSamples;
    const float* samples = buffer.getReadPointer(0);
    int searchWindow = juce::jmin(1000, numSamples - targetSampleIndex - 2);
    for (int offset = 0; offset < searchWindow; ++offset) {
        int idx = targetSampleIndex + offset;
        if (samples[idx] * samples[idx + 1] <= 0.0f)
            return idx;
    }
    return targetSampleIndex;
}

// ==============================================================================
// Main packager entry point
// ==============================================================================

// CORE CONCEPT: Scans an input folder for WAV files, classifies each one
// using the Upright Samples filename convention, auto-computes key zones by
// analysing adjacent root notes per type+dynamic group, and writes a binary
// .bin container file with a packed index table followed by raw WAV blobs.
// Also supports single-WAV input (pitched across the full MIDI range).
bool SamplePackager::createPackage(const juce::File& inputWavDir,
                                   const juce::File& outputBinFile) {
    // 1. Collect WAV files — non-recursive to avoid IR Samples subdirectory
    juce::Array<juce::File> wavFiles;
    if (inputWavDir.isDirectory()) {
        // findChildFiles with false = non-recursive (top-level only)
        wavFiles = inputWavDir.findChildFiles(juce::File::findFiles, false, "*.wav");
    } else if (inputWavDir.getFileExtension().equalsIgnoreCase(".wav")) {
        wavFiles.add(inputWavDir);
    }

    if (wavFiles.isEmpty()) {
        juce::Logger::writeToLog("SamplePackager: No WAV files found in " +
                                 inputWavDir.getFullPathName());
        return false;
    }

    // 2. Auto-detect microphone variant names from filenames
    //    Strategy: scan all stems and collect the part before the first known
    //    dynamic token (P_, MF_, F_, RT_, PEDAL_) as the mic name.
    juce::StringArray knownMics;
    {
        juce::StringArray dynMarkers { "_P_", "_MF_", "_F_", "_RT_", "_PEDAL_" };
        for (auto& f : wavFiles) {
            juce::String stem = f.getFileNameWithoutExtension();
            for (auto& marker : dynMarkers) {
                int idx = stem.indexOfIgnoreCase(marker);
                if (idx > 0) {
                    juce::String micName = stem.substring(0, idx).replace("_", " ").trim();
                    if (!knownMics.contains(micName))
                        knownMics.add(micName);
                    break;
                }
            }
        }
        // Normalise mic names back to underscore-separated so parsing works
        juce::StringArray knownMicsUnder;
        for (auto& m : knownMics)
            knownMicsUnder.add(m.replace(" ", "_"));
        knownMics = knownMicsUnder;
    }

    // If mic detection failed, fall back to legacy generic parsing
    if (knownMics.isEmpty())
        knownMics.add(""); // empty mic prefix — whole filename is the payload

    juce::Logger::writeToLog("SamplePackager: Detected " + juce::String(knownMics.size()) +
                             " mic variant(s): " + knownMics.joinIntoString(", "));

    // 3. Parse all WAV files and build SampleEntry records
    juce::WavAudioFormat wavFormat;
    struct EntryWithData {
        SampleEntry       entry;
        juce::MemoryBlock wavData;
        ParsedSampleInfo  info;
    };

    std::vector<EntryWithData> items;
    uint32_t sampleIdCounter = 1;

    for (auto& wavFile : wavFiles) {
        juce::String stem = wavFile.getFileNameWithoutExtension();

        // --- Single-WAV mode (no mic prefix, no dynamic suffix) ---
        // If only one file and it doesn't match the naming convention,
        // map it across the entire MIDI range.
        ParsedSampleInfo info = parseUprightFilename(stem, knownMics);

        if (!info.valid) {
            // Single-sample fallback: pitch-stretch across full MIDI range
            info.rootMidi   = parseMidiNoteFromToken(stem); // best-effort note parse
            info.type       = SampleType::Normal;
            info.dynamic    = DynamicLayer::Any;
            info.rrIndex    = 0;
            info.micVariant = 0;
            info.humanName  = stem;
            info.valid      = true;
            juce::Logger::writeToLog("SamplePackager: Fallback mode for: " + stem);
        }

        // Read WAV data
        std::unique_ptr<juce::AudioFormatReader> reader(
            wavFormat.createReaderFor(wavFile.createInputStream().release(), true));
        if (!reader) continue;

        juce::MemoryBlock block;
        wavFile.loadFileAsData(block);

        // Attack split point at ~150ms zero-crossing
        int target150ms = juce::roundToInt(0.150 * reader->sampleRate);
        juce::AudioBuffer<float> peekBuffer(
            (int)reader->numChannels,
            juce::jmin((int)reader->lengthInSamples, target150ms + 1050));
        reader->read(&peekBuffer, 0, peekBuffer.getNumSamples(), 0, true, true);
        int zeroCross = findZeroCrossing(peekBuffer, target150ms);

        SampleEntry entry{};
        entry.sampleID = sampleIdCounter++;
        stem.copyToUTF8(entry.name, sizeof(entry.name) - 1);
        entry.layerIndex        = 0; // overridden by caller if needed
        entry.rootNote          = (uint8_t)info.rootMidi;
        entry.keyLow            = 0; // computed below
        entry.keyHigh           = 127;
        dynamicToVelocityRange(info.dynamic, entry.velLow, entry.velHigh);
        entry.sampleType        = info.type;
        entry.dynamic           = info.dynamic;
        entry.roundRobinIndex   = (uint8_t)info.rrIndex;
        entry.roundRobinTotal   = 1; // updated in pass-2
        entry.micVariantIndex   = (uint8_t)info.micVariant;
        entry.fileOffset        = 0; // filled in write pass
        entry.wavDataSize       = (uint64_t)block.getSize();
        entry.attackSampleSize  = (uint32_t)zeroCross;

        items.push_back({ entry, block, info });
    }

    if (items.empty()) return false;

    // 4. Compute roundRobinTotal per (rootNote, sampleType, dynamic, mic) group
    for (auto& item : items) {
        int total = 0;
        for (auto& other : items) {
            if (other.entry.rootNote    == item.entry.rootNote   &&
                other.entry.sampleType  == item.entry.sampleType  &&
                other.entry.dynamic     == item.entry.dynamic     &&
                other.entry.micVariantIndex == item.entry.micVariantIndex) {
                ++total;
            }
        }
        item.entry.roundRobinTotal = (uint8_t)juce::jmax(1, total);
    }

    // 5. Compute key zones per (sampleType, dynamic, mic) group
    //    Each group gets its own sorted root list and key-zone split.
    {
        // Collect distinct group keys
        struct GroupKey {
            SampleType   type;
            DynamicLayer dyn;
            uint8_t      mic;
            bool operator<(const GroupKey& o) const {
                if (type != o.type) return type < o.type;
                if (dyn  != o.dyn)  return dyn  < o.dyn;
                return mic < o.mic;
            }
            bool operator==(const GroupKey& o) const {
                return type == o.type && dyn == o.dyn && mic == o.mic;
            }
        };

        std::vector<GroupKey> groups;
        for (auto& item : items) {
            GroupKey gk { item.entry.sampleType, item.entry.dynamic, item.entry.micVariantIndex };
            if (std::find(groups.begin(), groups.end(), gk) == groups.end())
                groups.push_back(gk);
        }

        for (auto& gk : groups) {
            // Collect unique root notes in this group (RR siblings share the same root)
            std::vector<uint8_t> roots;
            for (auto& item : items) {
                if (item.entry.sampleType == gk.type &&
                    item.entry.dynamic == gk.dyn &&
                    item.entry.micVariantIndex == gk.mic) {
                    if (std::find(roots.begin(), roots.end(), item.entry.rootNote) == roots.end())
                        roots.push_back(item.entry.rootNote);
                }
            }
            std::sort(roots.begin(), roots.end());

            // Assign key zones: each root extends DOWN to midpoint between itself
            // and previous root, and UP to the root note itself (downward-stretch model).
            for (size_t ri = 0; ri < roots.size(); ++ri) {
                uint8_t root    = roots[ri];
                uint8_t keyLow  = (ri == 0)
                    ? 0
                    : (uint8_t)((roots[ri - 1] + root) / 2 + 1);
                uint8_t keyHigh = (ri == roots.size() - 1)
                    ? 127
                    : (uint8_t)((root + roots[ri + 1]) / 2);

                // Pedal/Release samples span their root ±2 or full range
                if (gk.type == SampleType::PedalDown || gk.type == SampleType::PedalUp ||
                    gk.type == SampleType::ReleaseTail) {
                    keyLow  = 0;
                    keyHigh = 127;
                }

                // Apply to all items in this group with this root
                for (auto& item : items) {
                    if (item.entry.sampleType == gk.type &&
                        item.entry.dynamic    == gk.dyn  &&
                        item.entry.micVariantIndex == gk.mic &&
                        item.entry.rootNote   == root) {
                        item.entry.keyLow  = keyLow;
                        item.entry.keyHigh = keyHigh;
                    }
                }
            }
        }
    }

    // 6. Assign file offsets and write .bin
    uint64_t currentOffset = sizeof(ContainerHeader) +
                             (uint64_t)(items.size() * sizeof(SampleEntry));
    for (auto& item : items) {
        item.entry.fileOffset = currentOffset;
        currentOffset += item.entry.wavDataSize;
    }

    outputBinFile.deleteFile();
    juce::FileOutputStream outStream(outputBinFile);
    if (!outStream.openedOk()) return false;

    ContainerHeader hdr{};
    hdr.magic[0] = TKB_Magic[0]; hdr.magic[1] = TKB_Magic[1];
    hdr.magic[2] = TKB_Magic[2]; hdr.magic[3] = TKB_Magic[3];
    hdr.version  = 2;
    hdr.numSampleEntries = (uint32_t)items.size();

    outStream.write(&hdr, sizeof(ContainerHeader));
    for (auto& item : items)
        outStream.write(&item.entry, sizeof(SampleEntry));
    for (auto& item : items)
        outStream.write(item.wavData.getData(), item.wavData.getSize());

    juce::Logger::writeToLog("SamplePackager: Wrote " + juce::String(items.size()) +
                             " samples \u2192 " + outputBinFile.getFullPathName());
    return true;
}

// ==============================================================================
// Library packager — one .bin per mic variant
// ==============================================================================

// CORE CONCEPT: Converts a multi-mic WAV folder into a set of per-mic .bin
// files in the TKBLibrary directory. The engine, preset manager, and GUI all
// refer to these files by basename only, making them portable between Mac and
// RasPi as long as both have the TKBLibrary folder populated.
juce::Array<PackagedBin> SamplePackager::createLibrary(const juce::File& inputWavDir,
                                                        const juce::File& libraryOutputDir,
                                                        const juce::String& instrumentName) {
    juce::Array<PackagedBin> result;

    if (!inputWavDir.isDirectory()) {
        juce::Logger::writeToLog("SamplePackager::createLibrary: input is not a directory.");
        return result;
    }

    libraryOutputDir.createDirectory();

    // 1. Collect top-level WAVs only (no subfolders)
    auto wavFiles = inputWavDir.findChildFiles(juce::File::findFiles, false, "*.wav");
    if (wavFiles.isEmpty()) {
        juce::Logger::writeToLog("SamplePackager::createLibrary: No WAV files found.");
        return result;
    }

    // 2. Detect mic variant names (same logic as createPackage)
    juce::StringArray knownMics;
    {
        juce::StringArray dynMarkers { "_P_", "_MF_", "_F_", "_RT_", "_PEDAL_" };
        for (auto& f : wavFiles) {
            juce::String stem = f.getFileNameWithoutExtension();
            for (auto& marker : dynMarkers) {
                int idx = stem.indexOfIgnoreCase(marker);
                if (idx > 0) {
                    juce::String micName = stem.substring(0, idx).replace("_", " ").trim();
                    if (!knownMics.contains(micName))
                        knownMics.add(micName);
                    break;
                }
            }
        }
    }

    if (knownMics.isEmpty()) {
        // Single-mic folder — package everything as one file
        juce::String slug = instrumentName.replace(" ", "").replace(".", "");
        juce::File outFile = libraryOutputDir.getChildFile(slug + ".bin");
        if (createPackage(inputWavDir, outFile)) {
            PackagedBin pb;
            pb.instrumentName = instrumentName;
            pb.micVariantName = "Default";
            pb.micVariantId   = slug;
            pb.outputFile     = outFile;
            result.add(pb);
        }
        return result;
    }

    // 3. Per-mic: copy filtered WAVs to a temp dir and package
    for (auto& micName : knownMics) {
        juce::String micSlug   = micName.replace(" ", "").replace(".", "").replace("_", "");
        juce::String instrSlug = instrumentName.replace(" ", "").replace(".", "");
        juce::String binName   = instrSlug + "_" + micSlug + ".bin";
        juce::File outFile     = libraryOutputDir.getChildFile(binName);

        juce::String micNameUnder = micName.replace(" ", "_");
        juce::Array<juce::File> micFiles;
        for (auto& f : wavFiles) {
            juce::String stem = f.getFileNameWithoutExtension();
            if (stem.startsWith(micNameUnder) || stem.startsWith(micName))
                micFiles.add(f);
        }
        if (micFiles.isEmpty()) continue;

        // Copy to temp dir and package
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                             .getChildFile("TKBPack_" + micSlug);
        tempDir.deleteRecursively();
        tempDir.createDirectory();
        for (auto& f : micFiles)
            f.copyFileTo(tempDir.getChildFile(f.getFileName()));

        if (createPackage(tempDir, outFile)) {
            PackagedBin pb;
            pb.instrumentName = instrumentName;
            pb.micVariantName = micName;
            pb.micVariantId   = micSlug;
            pb.outputFile     = outFile;
            result.add(pb);
            juce::Logger::writeToLog("SamplePackager::createLibrary: Wrote " + binName +
                                     " (" + juce::String(micFiles.size()) + " WAVs)");
        }
        tempDir.deleteRecursively();
    }

    juce::Logger::writeToLog("SamplePackager::createLibrary: " +
                             juce::String(result.size()) + " mic variants packaged into " +
                             libraryOutputDir.getFullPathName());
    return result;
}