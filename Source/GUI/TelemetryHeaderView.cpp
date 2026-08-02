#include "TelemetryHeaderView.h"

TelemetryHeaderView::TelemetryHeaderView(MidiState &stateToMonitor,
                                         AudioEngine &engineToMonitor)
    : midiState(stateToMonitor), audioEngine(engineToMonitor) {
  startTimerHz(30); // 30 Hz refresh rate
}

TelemetryHeaderView::~TelemetryHeaderView() { stopTimer(); }

void TelemetryHeaderView::timerCallback() {
  // Audio DSP Load (%) measures real-time audio thread CPU consumption
  audioCpuUsage = audioEngine.getDeviceManager().getCpuUsage() * 100.0;
  int note = midiState.currentNote.load(std::memory_order_relaxed);
  float vel = midiState.currentVelocity.load(std::memory_order_relaxed);
  bool sustain = midiState.isSustainPedalDown.load(std::memory_order_relaxed);
  if (note != lastNote || vel != lastVel || sustain != lastSustainState ||
      audioCpuUsage > 0.05) {
    lastNote = note;
    lastVel = vel;
    lastSustainState = sustain;
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
  g.drawText("TUTOR KEYBOX :: TELEMETRY SURVEILLANCE", 15, 12, 320, 20,
             juce::Justification::left);

  // Active MIDI Note
  g.setFont(juce::FontOptions(13.0f));
  juce::String noteText =
      (lastNote >= 0)
          ? "Active Note: " +
                juce::MidiMessage::getMidiNoteName(lastNote, true, true, 4) +
                " (" + juce::String(lastNote) + ")"
          : "Active Note: NONE";
  g.drawText(noteText, 15, 36, 250, 18, juce::Justification::left);

  // Telemetry Gauges (Audio DSP CPU % and Hardware CPU Cores)
  g.setColour(juce::Colours::cyan);
  g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
  g.drawText("AUDIO DSP LOAD: " + juce::String(audioCpuUsage, 1) + "%", 15, 58,
             160, 16, juce::Justification::left);
  g.setColour(juce::Colours::orange);
  g.drawText("CPU CORES: " + juce::String(juce::SystemStats::getNumCpus()) +
                 " Cores",
             180, 58, 160, 16, juce::Justification::left);

  // Sustain Badge
  if (lastSustainState) {
    g.setColour(juce::Colours::orange);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("[SUSTAIN HELD]", 15, 78, 140, 16, juce::Justification::left);
  }
}
