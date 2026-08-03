#pragma once
#include <juce_core/juce_core.h>

// ==============================================================================
// MIDI CC Mapping
// ==============================================================================

struct MidiCcMapping {
    int faderCc[4]    { 1, 2, 3, 4 };  // Default CCs for Faders 0–3
    int buttonCc[3]   { 5, 6, 7 };     // Default CCs for Buttons 0–2 (Oct-, Oct+, HOLD)
    int knobCc[2]     { 8, 9 };        // Default CCs for Knobs 0–1
    int sustainPedalCc{ 64 };          // Sustain pedal CC (standard)
    bool faderReversed[4] { false, false, false, false }; // Reverse CC direction per fader
    int  faderCurve[4]    { 0, 0, 0, 0 }; // 0=Linear, 1=Log, 2=Exp, 3=SCurve
};

// ==============================================================================
// DSP & Envelope Presets
// ==============================================================================

struct ReverbPreset {
    juce::String irFilePath;
    float mix{0.3f};
    float preDelayMs{10.0f};
    float dampingHz{8000.0f};
};

struct FilterPreset {
    float cutoffHz{5000.0f};
    float resonance{0.707f};
    float drive{0.0f};
    int   filterType{0};  // 0=LP, 1=BP, 2=HP
};

struct AdsrPreset {
    float attackMs{5.0f};
    float decayMs{200.0f};
    float sustain{1.0f};
    float releaseMs{300.0f};
};

struct FilterEnvPreset : public AdsrPreset {
    float depth{0.0f};  // -1.0 to +1.0
};

// ==============================================================================
// Layer Preset
// ==============================================================================

// CORE CONCEPT: Stores the persistent state for one sampler layer including
// sample container, octave transpositions, hold mode, filter, and envelopes.
struct LayerPreset {
    float        volume             { 1.0f };
    bool         muted              { false };

    // CORE CONCEPT: Relative filename within TKBLibrary (basename only).
    juce::String sampleContainerFilename; // e.g. "UprightSamples_NeumannM49.bin"
    juce::String sampleContainerPath;     // deprecated — legacy fallback

    int          octaveOffset       { 0 }; // –2 to +2 octaves (layers 2+3 only)
    bool         holdActive         { false }; // HOLD mode (layers 2+3 only)
    bool         hasFilter          { false }; // true for layers 2+3 only

    FilterPreset    filter;
    FilterEnvPreset filterEnv;
    AdsrPreset      ampEnv;
};

// ==============================================================================
// PresetManager
// ==============================================================================

class PresetManager {
public:
    PresetManager();
    ~PresetManager() = default;

    // MIDI CC mapping accessors
    const MidiCcMapping& getMapping() const { return ccMapping; }
    void setMapping(const MidiCcMapping& map) { ccMapping = map; }

    void setFaderCc(int faderIndex, int ccNumber);
    int  getFaderCc(int faderIndex) const;

    void setButtonCc(int buttonIndex, int ccNumber);
    int  getButtonCc(int buttonIndex) const;

    void setKnobCc(int knobIndex, int ccNumber);
    int  getKnobCc(int knobIndex) const;

    // Reverb preset accessors
    void setReverbPreset(const ReverbPreset& rev) { reverb = rev; }
    const ReverbPreset& getReverbPreset() const { return reverb; }

    // Layer preset accessors
    void setLayerPreset(int layerIndex, float volume, bool muted,
                        const juce::String& containerFilename);
    void setLayerPresetFull(int layerIndex, const LayerPreset& preset);
    const LayerPreset& getLayerPreset(int layerIndex) const;

    // Serialization
    juce::String toJsonString() const;
    bool loadFromJsonString(const juce::String& jsonText);
    bool saveToFile(const juce::File& file) const;
    bool loadFromFile(const juce::File& file);

private:
    MidiCcMapping            ccMapping;
    ReverbPreset             reverb;
    std::array<LayerPreset, 4> layers;
};
