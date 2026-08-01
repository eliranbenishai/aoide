# Tramp UI prototype (throwaway)

Answers: *What should Tramp’s v1 scalable UI look like?*

Not Flutter production code — a cheap browser shell so we can react to structure before writing the real app.

## Run

From this directory:

```bash
python -m http.server 8765
```

Open http://localhost:8765/?variant=W

## Locked direction

**W** — Variant A’s transport-stack layout with Variant B’s paper/ink design language.

## Variants

| Key | Name | Structure |
|-----|------|-----------|
| W | Winner (locked) | A layout + B language |
| A | Transport stack | Brand + transport block on top; playlist fills the rest |
| B | Playlist-first column | Playlist dominates left; art + transport on the right |
| C | Playlist surface + HUD | Playlist is the whole surface; transport as bottom HUD |

Switch with the bottom bar or `←` / `→`.
