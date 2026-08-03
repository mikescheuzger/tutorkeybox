#pragma once
#include <cstdint>

#pragma pack(push, 1) // No struct padding — binary-stable .bin format

// ==============================================================================
// .bin Container Magic
// ==============================================================================

constexpr char TKB_Magic[4] = {'T', 'K', 'B', '2'}; // v2 — new SampleEntry layout

// ==============================================================================
// Sample Classification Enums
// ==============================================================================

// CORE CONCEPT: Identifies which physical event triggers this sample,
// enabling the engine to route NoteOn/NoteOff/CC64 to the correct voice pool.
enum class SampleType : uint8_t {
    Normal       = 0, // nopedal   — NoteOn while sustain pedal is UP
    WithPedal    = 1, // withpedal — NoteOn while sustain pedal is DOWN (sympathetic resonance)
    ReleaseTail  = 2, // rt        — NoteOff (let ring, triggered on key release)
    PedalDown    = 3, // pedal_down — CC64 rising edge (pedal pressed noise)
    PedalUp      = 4, // pedal_up   — CC64 falling edge (pedal released noise)
};

// CORE CONCEPT: Velocity zone of this recording, used to select the correct
// dynamic layer when the MIDI velocity falls within a range.
enum class DynamicLayer : uint8_t {
    Piano      = 0, // P   — soft (vel 1–42)
    MezzoForte = 1, // MF  — medium (vel 43–84)
    Forte      = 2, // F   — loud (vel 85–127)
    Any        = 3, // Single-sample — covers full velocity range (0–127)
};

// ==============================================================================
// File Container Header (at byte 0 of every .bin file)
// ==============================================================================

struct ContainerHeader {
    char     magic[4];           // Must match TKB_Magic ('T','K','B','2')
    uint32_t version;            // Format version: 2
    uint32_t numSampleEntries;   // Total SampleEntry records in the index table
};

// ==============================================================================
// Per-Sample Metadata Entry (packed index table following ContainerHeader)
// ==============================================================================

// CORE CONCEPT: One SampleEntry describes exactly one recorded audio sample:
// its pitch range, velocity range, sample type, round-robin slot, dynamic
// layer, and the byte offset + size of its WAV data within the .bin blob.
struct SampleEntry {
    uint32_t     sampleID;            // Unique ID within the container
    char         name[64];            // Human-readable sample name (null-terminated)

    uint8_t      layerIndex;          // Which LayeredSynth layer this belongs to (0–3)
    uint8_t      rootNote;            // MIDI root note (the pitch this was recorded at)
    uint8_t      keyLow;              // Lowest MIDI note this sample covers
    uint8_t      keyHigh;             // Highest MIDI note this sample covers
    uint8_t      velLow;              // Lowest MIDI velocity this sample responds to
    uint8_t      velHigh;             // Highest MIDI velocity this sample responds to

    SampleType   sampleType;          // What event triggers this sample
    DynamicLayer dynamic;             // Velocity zone / dynamic marking
    uint8_t      roundRobinIndex;     // 0-based index within the RR pool (0, 1, 2…)
    uint8_t      roundRobinTotal;     // Total number of RR variants for this note+dynamic
    uint8_t      micVariantIndex;     // Which microphone variant (0 = M49, 1 = U269C, etc.)
    uint8_t      _reserved[3];        // Future use, must be zero

    uint64_t     fileOffset;          // Byte offset of the WAV block inside the .bin file
    uint64_t     wavDataSize;         // Byte size of the WAV block
    uint32_t     attackSampleSize;    // Number of samples in the preloaded RAM attack buffer
};

#pragma pack(pop)
