# Lock the v1 stack for the spec

Type: grilling
Status: resolved
Blocked by: 01

## Question

Given the stack research answer, which stack do we **lock** into the Tramp v1 product spec — and what do we explicitly not commit to (e.g. specific packages inside the stack)?

## Answer

**Lock Flutter** for the Tramp v1 product spec (Windows, Linux, macOS desktop).

Preferred defaults (same responsibilities, swappable later): `window_manager` for app chrome; media_kit (libmpv) for playback.

**Not locked:** state-management library, routing, exact Flutter/Dart SDK versions, design-system packages.

**Not v1:** Tauri, Electron, or a second UI toolkit.

ADR: [docs/adr/0001-flutter-for-v1.md](../../../docs/adr/0001-flutter-for-v1.md). Research: [research/v1-stack.md](../research/v1-stack.md).
