# Wayfinder: Tramp v1 product spec

Label: `wayfinder:map`

## Destination

A written **v1 product spec** checked into this repo — clear enough to plan implementation from. This map decides; it does not build Tramp.

## Notes

- Domain: desktop music player; read `CONTEXT.md` for vocabulary (Tramp, app chrome, scalable UI, playlist, track, library).
- Skills every session should consult: `/grilling`, `/domain-modeling`, `/research`, `/prototype` as ticket types require.
- Plan, don't build — no app implementation inside this map unless a ticket is explicitly a Task that unblocks a decision.
- UX-heavy effort: prefer concrete prototypes over prose for look-and-feel.
- Stack **locked:** Flutter for v1; preferred defaults `window_manager` + media_kit; see ADR-0001. Prototype and later tickets assume Flutter.

## Decisions so far

<!-- the index — one line per closed ticket: enough to judge relevance, then zoom the link for the detail the ticket holds -->

- [Recommend the v1 implementation stack](issues/01-recommend-v1-stack.md) — Assume Flutter for v1 (media_kit audio, window_manager chrome); Tauri 2 only if team/size constraints dominate
- [Lock the v1 stack for the spec](issues/02-lock-v1-stack.md) — Flutter locked; chrome/audio packages preferred not frozen; no Tauri/Electron/second toolkit
- [Close remaining v1 product edges](issues/03-close-remaining-v1-product-edges.md) — Packaging yes/stores no; file associations; basic a11y; gapless/crossfade non-goals; license out of spec; write to `docs/tramp-v1-spec.md`
- [Prototype the app chrome and UI direction](issues/04-prototype-app-chrome-ui-direction.md) — A transport-stack layout + B paper/ink design language (prototype variant W)
- [Write the v1 product spec into the repo](issues/05-write-v1-product-spec.md) — Spec at `docs/tramp-v1-spec.md`

## Not yet specified

<!-- cleared — destination reached -->

## Out of scope

- Classic Winamp skins (v1) — deferred; glossary term retained for later
- Media library / scanned catalog (v1)
- Streaming services, plugin ecosystem, EQ, visualizations as v1 requirements
- Gapless playback and crossfade (v1) — explicit non-goals
- App-store listings as a v1 requirement
- Licensing posture inside the product spec
- Detachable multi-window layout (main/playlist/EQ frames)
- Building the Tramp application itself (post-map work in this repo)
