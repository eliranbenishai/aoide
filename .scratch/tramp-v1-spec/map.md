# Wayfinder: Tramp v1 product spec

Label: `wayfinder:map`

## Destination

A written **v1 product spec** checked into this repo — clear enough to plan implementation from. This map decides; it does not build Tramp.

## Notes

- Domain: desktop music player; read `CONTEXT.md` for vocabulary (Tramp, app chrome, scalable UI, playlist, track, library).
- Skills every session should consult: `/grilling`, `/domain-modeling`, `/research`, `/prototype` as ticket types require.
- Plan, don't build — no app implementation inside this map unless a ticket is explicitly a Task that unblocks a decision.
- UX-heavy effort: prefer concrete prototypes over prose for look-and-feel.
- Stack research brief (locked in charting): Flutter primary candidate; Tauri 2 + Rust runner-up; Electron out of the first cut. Hybrid web/native only if research shows it stays extremely performant for a dense player UI.

## Decisions so far

<!-- the index — one line per closed ticket: enough to judge relevance, then zoom the link for the detail the ticket holds -->

- [Recommend the v1 implementation stack](issues/01-recommend-v1-stack.md) — Assume Flutter for v1 (media_kit audio, window_manager chrome); Tauri 2 only if team/size constraints dominate

## Not yet specified

- Installer / distribution channel expectations for Win/Linux/macOS
- OS file associations and “open with Tramp”
- Accessibility bar for v1
- Whether gapless / crossfade stay explicitly deferred in the spec or get a one-line non-goal
- Exact resting place and filename for the finished spec in-repo (e.g. `docs/` vs `.scratch/`)
- Licensing / open-source posture for the project (if the spec should mention it)

## Out of scope

- Classic Winamp skins (v1) — deferred; glossary term retained for later
- Media library / scanned catalog (v1)
- Streaming services, plugin ecosystem, EQ, visualizations as v1 requirements
- Detachable multi-window layout (main/playlist/EQ frames)
- Building the Tramp application itself (post-map work in this repo)
