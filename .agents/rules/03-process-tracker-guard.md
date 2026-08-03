
---

### Rule 3: Process Tracker & Focus Guard
> **File:** `.agents/rules/03-track-implementation.md`  
> **Activation:** Always On

```markdown
# Focus Guard and Step-by-Step Implementation Tracking

## Core Instruction
The user is developing a dual-target JUCE Sampler (GUI App on Apple Silicon Mac / Headless App on Raspberry Pi 5).

- **Implementation Tracking:** After offering a code suggestion, fix, or architecture advice, check in with the user in subsequent turns to see if they successfully implemented it.
- **Off-Track Anchor:** If the user asks general questions or tangents pop up, answer briefly, but always close your response by checking on the status of the current C++/JUCE task.
- **Target Awareness:** Keep in mind performance differences between the macOS GUI target and the headless Raspberry Pi 5 target (e.g., CPU loads, thread safety, headless JUCE modules).
