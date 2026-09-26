# Technical and review notes

This document deliberately calls out implementation details that deserve independent review.

## Loading model

The mod is a 32-bit proxy DLL. `Caravan.def` forwards the three version-information exports used by the game to `version_real.dll`. The installer:

1. verifies the exact SHA-256 of each supported HotA 1.8.0 executable;
2. copies the original executable to a new `h3hota Caravan*.exe` launcher;
3. changes the copied launcher's single `VERSION.dll` import to `Caravan.dll` (the names have equal length);
4. copies the operating system's 32-bit `version.dll` to `version_real.dll`;
5. leaves the original game executables untouched.

There is no networking, telemetry, updater or downloaded code. Runtime logging is limited to `Caravan.log` in the game directory.

## Game integration

- The DLL waits for `HotA.dll`, then reads known HotA 1.8.0 structures through fixed virtual addresses and offsets.
- The interface is a child window of the game window. Its textures and fonts are loaded at runtime from the user's own HD Mod installation; no extracted assets are distributed.
- A physical caravan is stored in the game's garrison vector. Two otherwise unused bytes contain markers `0xCA, 0x47`; another byte stores the destination town ID.
- The adventure-map drawing is changed to the built-in wagon object while the interaction remains garrison-like, allowing normal troop transfer and combat.
- Route planning is an eight-direction breadth-first search on the current map level. The caravan advances a maximum of eight route cells when the in-game date changes.

## Known limitations and review targets

- All game addresses and structure layouts are version-specific. Never remove the installer hash checks without re-validating them against a new HotA build.
- Surface-to-underground routing is not implemented.
- Multiplayer and hot-seat behavior is untested.
- Caravan markers are stored in padding-like bytes of the garrison record. Persistence and compatibility with every save/load path need wider testing.
- A worker thread polls the hotkey and requests physical-caravan ticks. Map movement and object deletion are dispatched through a thread-specific Windows message hook and execute on the game's main message thread. Other UI-driven game-memory access still deserves additional review.
- Pathfinding intentionally treats heroes, monsters and garrisons as temporary blockers. Other unusual map objects may need special handling.
- The UI and test coverage currently target Windows with HD Mod and Russian game text.

Use disposable saves while reviewing the experimental physical-delivery mode. Crash logs and exact reproduction steps are especially useful when reporting a problem.

