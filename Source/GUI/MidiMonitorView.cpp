#include "MidiMonitorView.h"

MidiMonitorView::MidiMonitorView(const MidiState &stateToMonitor)
    : midiState(stateToMonitor) {
  // Start timer at 30 Hz refresh rate (every 33 ms)
  startTimerHz(30);
}

MidiMonitorView::~MidiMonitorView() { stopTimer(); }

void MidiMonitorView::timerCallback() {
  // Read atomic state from MidiState
  bool active = midiState.isNoteActive.load(std::memory_order_relaxed);
  int note = midiState.currentNote.load(std::memory_order_relaxed);
  float vel = midiState.currentVelocity.load(std::memory_order_relaxed);
  bool sustain = midiState.isSustainPedalDown.load(std::memory_order_relaxed);
  // Only request a UI repaint if the state actually changed
  if (active != lastNoteActive || note != lastNoteNumber ||
      vel != lastVelocity || sustain != lastSustainPedal) {
    lastNoteActive = active;
    lastNoteNumber = note;
    lastVelocity = vel;
    lastSustainPedal = sustain;
    repaint();
  }
}

void MidiMonitorView::paint(juce::Graphics &g) {
  // Dark developer UI background
  g.fillAll(juce::Colour(0xff18181c));

  // Outer border
  g.setColour(juce::Colour(0xff33333d));
  g.drawRect(getLocalBounds().toFloat(), 2.0f);

  auto area = getLocalBounds().reduced(20);

  // Title Header
  g.setColour(juce::Colours::cyan);
  g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
  g.drawText("DEV SURVEILLANCE :: MIDI INPUT MONITOR", area.removeFromTop(30),
             juce::Justification::left);

  area.removeFromTop(10);

  if (lastNoteActive && lastNoteNumber >= 0) {
    // --- NOTE ACTIVE STATE ---

    // Active Status Badge (Green)
    g.setColour(juce::Colour(0xff2ed573));
    g.fillEllipse(area.getX(), area.getY() + 4, 12, 12);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("GATE OPEN", area.getX() + 20, area.getY(), 100, 20,
               juce::Justification::left);

    // Sustain Pedal Status Badge
    if (lastSustainPedal) {
      g.setColour(juce::Colour(0xffffa500));
      g.drawText("[SUSTAIN HELD]", area.getX() + 130, area.getY(), 140, 20,
                 juce::Justification::left);
    }

    area.removeFromTop(35);

    // Note Name calculation (e.g. 60 -> C4)
    juce::String noteName =
        juce::MidiMessage::getMidiNoteName(lastNoteNumber, true, true, 3);
    juce::String noteText = juce::String::formatted(
        "NOTE: %s (MIDI %d)", noteName.toRawUTF8(), lastNoteNumber);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    g.drawText(noteText, area.removeFromTop(40), juce::Justification::left);

    // Velocity Text & Bar
    int velPercent = juce::roundToInt(lastVelocity * 100.0f);
    juce::String velText = juce::String::formatted("VELOCITY: %d%% (%.2f)",
                                                   velPercent, lastVelocity);

    g.setColour(juce::Colour(0xffa4b0be));
    g.setFont(juce::FontOptions(16.0f, juce::Font::plain));
    g.drawText(velText, area.removeFromTop(25), juce::Justification::left);

    area.removeFromTop(8);

    // Visual Velocity Meter Bar
    auto barArea = area.removeFromTop(16).withWidth(300);
    g.setColour(juce::Colour(0xff2f3542));
    g.fillRect(barArea);

    g.setColour(juce::Colour(0xff1e90ff));
    g.fillRect(
        barArea.withWidth(juce::roundToInt(barArea.getWidth() * lastVelocity)));
  } else {
    // --- STANDBY STATE (Disappears when no note is pressed) ---
    g.setColour(juce::Colour(0xff57606f));
    g.setFont(juce::FontOptions(16.0f, juce::Font::italic));

    if (lastSustainPedal) {
      g.setColour(juce::Colour(0xffffa500));
      g.drawText("[SUSTAIN HELD]", area.getX() + 130, area.getY(), 140, 20,
                 juce::Justification::left);
    } else {
      g.drawText("No MIDI note active (Waiting for input...)", area,
                 juce::Justification::centredLeft);
    }
  }
}

void MidiMonitorView::resized() {
  // Layout subcomponents here if needed in future
}
