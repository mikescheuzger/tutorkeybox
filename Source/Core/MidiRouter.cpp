#include "MidiRouter.h"
#include <cmath>

// CORE CONCEPT: Constructor binding MidiRouter to target LayeredSynth instance.
MidiRouter::MidiRouter(LayeredSynth &synthToControl)
    : synth(synthToControl) {
  updatePianoLayerVolumes();
}

// CORE CONCEPT: Applies selected curve shape and optional reversal to normalized 0.0–1.0 control input.
float MidiRouter::applyFaderCurve(float normVal, int curveType, bool reversed) {
  float val = juce::jlimit(0.0f, 1.0f, normVal);
  if (reversed) {
    val = 1.0f - val;
  }

  switch (curveType) {
    case 1: // Logarithmic (audio taper response)
      return std::pow(val, 2.0f);
    case 2: // Exponential
      return (std::exp(val * 2.0f) - 1.0f) / (std::exp(2.0f) - 1.0f);
    case 3: // S-Curve (Smoothstep)
      return val * val * (3.0f - 2.0f * val);
    case 0: // Linear
    default:
      return val;
  }
}

// CORE CONCEPT: Computes constant-power (power-law) crossfade between Layer 0 and Layer 1 scaled by master volume.
void MidiRouter::updatePianoLayerVolumes() {
  float x = juce::jlimit(0.0f, 1.0f, crossfadePosition);
  float gainL0 = std::cos(x * juce::MathConstants<float>::halfPi);
  float gainL1 = std::sin(x * juce::MathConstants<float>::halfPi);

  synth.setLayerVolume(0, gainL0 * masterVolume);
  synth.setLayerVolume(1, gainL1 * masterVolume);
}

// CORE CONCEPT: Updates internal CC mappings from PresetManager struct.
void MidiRouter::updateFromPreset(const MidiCcMapping &mapping) {
  currentMapping = mapping;
}

// CORE CONCEPT: Sets or clears active CC Learn target index.
void MidiRouter::setCcLearnTarget(int targetIndex) {
  if (targetIndex >= 0 && targetIndex < static_cast<int>(MidiControlTarget::Count)) {
    learnTargetIndex = targetIndex;
  } else {
    learnTargetIndex = -1;
  }
}

// CORE CONCEPT: Main CC message dispatch loop handling CC Learn mode, fader values, buttons, and macros.
void MidiRouter::processMidiCc(const juce::MidiMessage &message) {
  if (!message.isController())
    return;

  int cc = message.getControllerNumber();
  int ccVal = message.getControllerValue();
  float normVal = ccVal / 127.0f;

  // Handle CC Learn if active
  if (learnTargetIndex >= 0) {
    int target = learnTargetIndex;
    learnTargetIndex = -1; // Reset learn target after capture

    if (target >= 0 && target <= 3) {
      currentMapping.faderCc[target] = cc;
    } else if (target >= 4 && target <= 6) {
      currentMapping.buttonCc[target - 4] = cc;
    }
    juce::Logger::writeToLog("CC Learn: Target " + juce::String(target) + " mapped to CC " + juce::String(cc));
    return;
  }

  // Fader 0: Piano Master Volume (layers 0 & 1)
  if (cc == currentMapping.faderCc[0]) {
    masterVolume = applyFaderCurve(normVal, currentMapping.faderCurve[0], currentMapping.faderReversed[0]);
    updatePianoLayerVolumes();
    return;
  }

  // Fader 1: Crossfade Layer 0 <-> Layer 1
  if (cc == currentMapping.faderCc[1]) {
    crossfadePosition = applyFaderCurve(normVal, currentMapping.faderCurve[1], currentMapping.faderReversed[1]);
    updatePianoLayerVolumes();
    return;
  }

  // Fader 2: Macro A (Layer 2 volume)
  if (cc == currentMapping.faderCc[2]) {
    float gain = applyFaderCurve(normVal, currentMapping.faderCurve[2], currentMapping.faderReversed[2]);
    synth.setLayerVolume(2, gain);
    return;
  }

  // Fader 3: Macro B (Layer 3 volume)
  if (cc == currentMapping.faderCc[3]) {
    float gain = applyFaderCurve(normVal, currentMapping.faderCurve[3], currentMapping.faderReversed[3]);
    synth.setLayerVolume(3, gain);
    return;
  }

  // Button 0: Octave Down for layers 2 & 3 (triggered on button press ccVal >= 64)
  if (cc == currentMapping.buttonCc[0] && ccVal >= 64) {
    int currentOffset = synth.getLayerOctaveOffset(2);
    int newOffset = juce::jlimit(-2, 2, currentOffset - 1);
    synth.setLayerOctaveOffset(2, newOffset);
    synth.setLayerOctaveOffset(3, newOffset);
    juce::Logger::writeToLog("MidiRouter: Octave Down -> " + juce::String(newOffset));
    return;
  }

  // Button 1: Octave Up for layers 2 & 3 (triggered on button press ccVal >= 64)
  if (cc == currentMapping.buttonCc[1] && ccVal >= 64) {
    int currentOffset = synth.getLayerOctaveOffset(2);
    int newOffset = juce::jlimit(-2, 2, currentOffset + 1);
    synth.setLayerOctaveOffset(2, newOffset);
    synth.setLayerOctaveOffset(3, newOffset);
    juce::Logger::writeToLog("MidiRouter: Octave Up -> " + juce::String(newOffset));
    return;
  }

  // Button 2: HOLD toggle for layers 2 & 3 (triggered on button press ccVal >= 64)
  if (cc == currentMapping.buttonCc[2] && ccVal >= 64) {
    bool newHold = !synth.getLayerHold(2);
    synth.setLayerHold(2, newHold);
    synth.setLayerHold(3, newHold);
    juce::Logger::writeToLog("MidiRouter: HOLD mode -> " + juce::String(newHold ? "ON" : "OFF"));
    return;
  }
}
