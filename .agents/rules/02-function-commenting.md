---
trigger: always_on
---

# C++ / JUCE Code Documentation Standard

## Core Instruction
Whenever you write, draft, or propose any new function or method for this project:

- Always include a concise comment directly above the function declaration/definition explaining its **core concept** and **purpose**.
- Keep comments brief and tailored for audio processing context (e.g., distinguishing real-time audio thread constraints from GUI thread tasks in JUCE).

## Example Format
```cpp
// CORE CONCEPT: Calculates peak amplitude across the audio buffer 
// to prevent clipping before passing samples to the DAC.
void ProcessBlockSignal(juce::AudioBuffer<float>& buffer) {
    // ...
}
