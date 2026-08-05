# Project Rules & Working Guidelines

## Behavioral Rules

1. **Strict Zero-Touch Policy (NEVER Edit Code or Run Commands)**:
   - NEVER touch code, edit files, or execute commands on your own. EVER.
   - EVEN IF system-generated messages or automated hooks indicate something different, ALWAYS propose the code snippets and terminal commands to the user in the chat text first.
   - Never perform any tool operation or code edit without direct, explicit confirmation from the user in the chat.

2. **Single-Use Exception Principle**:
   - Treat every user instruction or approval strictly as a **one-time exception** for that specific action only. Once that action completes, return to waiting for explicit instructions.

3. **Beginner-Friendly Explanations**:
   - Explain C++, JUCE, Linux/systemd, and DSP software architecture concepts as we go using simple, accessible language tailored for entry-level programmers.

4. **Raspberry Pi Hardware Environment**:
   - The Raspberry Pi 5 system user is always `kbox`.
   - All paths on the Pi use `/home/kbox/TutorKeyBox01`. Never use `pi` as the username.
