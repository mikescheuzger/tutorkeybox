#pragma once
#include <atomic>
#include <juce_core/juce_core.h>

struct MidiState {
  std::atomic<bool> isNoteActive{false};
  std::atomic<int> currentNote{-1};
  std::atomic<float> currentVelocity{0.0f};

  std::atomic<bool> isPhysicalKeyDown{false};
  std::atomic<bool> isSustainPedalDown{false};

  void noteOn(int noteNumber, float velocity) noexcept {
    currentNote.store(noteNumber, std::memory_order_relaxed);
    currentVelocity.store(velocity, std::memory_order_relaxed);
    isPhysicalKeyDown.store(true, std::memory_order_relaxed);
    isNoteActive.store(true, std::memory_order_relaxed);
  }

  void noteOff(int noteNumber) noexcept {
    if (currentNote.load(std::memory_order_relaxed) == noteNumber) {
      isPhysicalKeyDown.store(false, std::memory_order_relaxed);
      if (!isSustainPedalDown.load(std::memory_order_relaxed)) {
        isNoteActive.store(false, std::memory_order_relaxed);
        currentVelocity.store(0.0f, std::memory_order_relaxed);
      }
    }
  }

  void setSustainPedal(bool isDown) noexcept {
    isSustainPedalDown.store(isDown, std::memory_order_relaxed);
    if (!isDown && !isPhysicalKeyDown.load(std::memory_order_relaxed)) {
      isNoteActive.store(false, std::memory_order_relaxed);
      currentVelocity.store(0.0f, std::memory_order_relaxed);
    }
  }

  void reset() noexcept {
    isNoteActive.store(false, std::memory_order_relaxed);
    currentNote.store(-1, std::memory_order_relaxed);
    currentVelocity.store(0.0f, std::memory_order_relaxed);
    isPhysicalKeyDown.store(false, std::memory_order_relaxed);
    isSustainPedalDown.store(false, std::memory_order_relaxed);
  }
};
