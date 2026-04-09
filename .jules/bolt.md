## 2024-11-20 - Capstone Initialization Overhead
**Learning:** Capstone instantiation inside highly active callbacks (like GUI repaints or memory view updates) is unexpectedly expensive and causes noticeable UI stutter when inspecting raw memory arrays.
**Action:** Always instantiate Capstone once and cache it in the application state. Never build it inside loop contexts or repeated networking/UI callbacks.
