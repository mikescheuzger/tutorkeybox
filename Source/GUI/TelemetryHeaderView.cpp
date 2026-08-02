#include "TelemetryHeaderView.h"

TelemetryHeaderView::TelemetryHeaderView(MidiState &stateToMonitor,
                                         AudioEngine &engineToMonitor)
    : midiState(stateToMonitor), audioEngine(engineToMonitor) {
  startTimerHz(30); // 30 Hz refresh rate
}

TelemetryHeaderView::~TelemetryHeaderView() { stopTimer(); }

void TelemetryHeaderView::timerCallback() {
  audioCpuUsage = audioEngine.getDeviceManager().getCpuUsage() * 100.0;

  int note = midiState.currentNote.load(std::memory_order_relaxed);
  float vel = midiState.currentVelocity.load(std::memory_order_relaxed);
  bool sustain = midiState.isSustainPedalDown.load(std::memory_order_relaxed);

  int root = midiState.lastRootNote.load(std::memory_order_relaxed);
  int kLow = midiState.lastKeyLow.load(std::memory_order_relaxed);
  int kHigh = midiState.lastKeyHigh.load(std::memory_order_relaxed);
  int vLow = midiState.lastVelLow.load(std::memory_order_relaxed);
  int vHigh = midiState.lastVelHigh.load(std::memory_order_relaxed);

  if (note != lastNote || vel != lastVel || sustain != lastSustainState ||
      root != cachedRootNote ||
      std::strcmp(cachedSampleName, midiState.lastSampleName) != 0 ||
      audioCpuUsage > 0.05) {
    lastNote = note;
    lastVel = vel;
    lastSustainState = sustain;

    std::strncpy(cachedSampleName, midiState.lastSampleName,
                 sizeof(cachedSampleName));
    cachedRootNote = root;
    cachedKeyLow = kLow;
    cachedKeyHigh = kHigh;
    cachedVelLow = vLow;
    cachedVelHigh = vHigh;

    repaint();
  }
}

void TelemetryHeaderView::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Dark Card Background
  g.setColour(juce::Colour(0xff1e1e24));
  g.fillRoundedRectangle(bounds, 8.0f);
  g.setColour(juce::Colour(0xff33333f));
  g.drawRoundedRectangle(bounds, 8.0f, 1.5f);

  // Title
  g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  g.setColour(juce::Colours::white);
  g.drawText("TUTOR KEYBOX :: TELEMETRY & SAMPLE INSPECTOR", 15, 10, 360, 20,
             juce::Justification::left);

  // Active MIDI Note
  g.setFont(juce::FontOptions(13.0f));
  juce::String noteText =
      (lastNote >= 0)
          ? "Active Note: " +
                juce::MidiMessage::getMidiNoteName(lastNote, true, true, 4) +
                " (" + juce::String(lastNote) + ")"
          : "Active Note: NONE";
  g.drawText(noteText, 15, 32, 220, 18, juce::Justification::left);

  // Telemetry Gauges (Audio DSP CPU & System CPU Cores)
  g.setColour(juce::Colours::cyan);
  g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
  g.drawText("AUDIO DSP: " + juce::String(audioCpuUsage, 1) + "%", 15, 52, 140,
             16, juce::Justification::left);

  g.setColour(juce::Colours::orange);
  g.drawText("CORES: " + juce::String(juce::SystemStats::getNumCpus()), 160, 52,
             100, 16, juce::Justification::left);

  // Sustain Badge
  if (lastSustainState) {
    g.setColour(juce::Colours::orange);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("[SUSTAIN HELD]", 270, 32, 120, 18, juce::Justification::left);
  }

  // --- LIVE SAMPLE MAPPING INSPECTION LINE ---
  g.setColour(juce::Colour(0xff88aaee));
  g.setFont(juce::FontOptions(11.0f, juce::Font::bold));

  juce::String inspectorText =
      "INSPECTOR: \"" + juce::String(cachedSampleName) + "\"";
  if (cachedRootNote >= 0) {
    inspectorText +=
        " | Root: " +
        juce::MidiMessage::getMidiNoteName(cachedRootNote, true, true, 4) +
        " (" + juce::String(cachedRootNote) + ")";
    inspectorText += " | KeyZone: [" + juce::String(cachedKeyLow) + "-" +
                     juce::String(cachedKeyHigh) + "]";
    inspectorText += " | VelZone: [" + juce::String(cachedVelLow) + "-" +
                     juce::String(cachedVelHigh) + "]";
  } else {
    inspectorText += " | No Sample Triggered Yet";
  }

  g.drawText(inspectorText, 15, 74, getWidth() - 30, 18,
             juce::Justification::left);
}
