# Close remaining v1 product edges

Type: grilling
Status: resolved

## Question

Which still-fuzzy product edges must the v1 spec decide (vs leave as non-goals)?

Candidates surfaced while charting (graduate or reject each):

- Installer / distribution expectations per OS
- File associations and “open with Tramp”
- Accessibility bar for v1
- Gapless / crossfade as explicit non-goals vs silent deferral
- Whether the spec states licensing / open-source posture
- Where the finished spec file should live in this repo

Resolve with a short list of decisions (or explicit “out of spec”) ready to write into the document.

## Answer

| Edge | Decision |
|------|----------|
| Distribution | Shippable Win/Linux/macOS via Flutter packaging; app-store listings not required for v1 |
| File associations | Yes — v1 audio formats + `.m3u`/`.m3u8` open with Tramp |
| Accessibility | Keyboard for transport + playlist; Flutter semantics defaults; no WCAG certification gate |
| Gapless / crossfade | Explicit v1 non-goals |
| Licensing | Out of the product spec (decide separately via LICENSE/README) |
| Spec path | `docs/tramp-v1-spec.md` |
