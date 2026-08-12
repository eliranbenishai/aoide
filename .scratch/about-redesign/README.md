# About window redesign — layout exploration

Throwaway renders from a golden harness (deleted after the decision), kept as
the primary source behind the About window's current layout.

The complaint: the company lockup floated in an empty CRT well with the URL
crammed under it, and the copy read like a spec sheet.

| Shot | Direction | Verdict |
|------|-----------|---------|
| `00-baseline.png` | Before — left-aligned hero, tagline, lockup adrift in the well | The problem |
| `01-variant-a.png` | Credits screen: lockup framed with copyright + link chip | Composed, but the well is still mostly a logo parking space |
| `02-variant-b.png` | Marquee + maker's nameplate | Nameplate won; badge inside the well failed (see constraints) |
| `03-variant-c.png` | Full-bleed CRT boot screen | Same badge failure, and no room for the lockup to read |
| `04-variant-d.png` | Glowing tagline + spectrum flourish + nameplate | Handsome but the well carried little |
| `05-variant-e.png` | "MADE BY" credits card, lockup kept whole | The brand-fidelity option if the device-only mark is ever rejected |
| `06-variant-f.png` | Nameplate + back-panel spec readout | Chosen shell; specs later swapped for usage stats |
| `10-redesign.png` | Shipped layout | — |

## Constraints found the hard way

- **The Tramp badge cannot sit inside a `MockupScreen`.** The logo is line art
  over transparency, so it needs its pale disc to read, and the screen's
  scanline overlay bands that disc into mush (`03-variant-c.png`).
- **`assets/proximamagnifica.svg` is a lockup, not a symbol.** Its own two-line
  wordmark is ~1/10 of the asset's height, so it only reads at roughly 100
  logical pixels and up — which is exactly why 96px looked stranded. Small
  placements use `ProximaMagnificaMark` (the comet device, cropped) with the
  company name set in Tramp Condensed beside it. A symbol-only export would
  remove the crop; see `docs/design/ASSETS_NEEDED.md`.

## Shipped

Three registers of the same hardware the chrome imitates: product on the shell
face (badge, wordmark, backronym with lit initials, version readout), the
listener's counters glowing in a CRT well, maker's brushed nameplate bolted
along the bottom edge with the website as a lit chip.

The stats are `AboutStats.placeholder` until the playlist-manager overhaul
lands — `.scratch/about-usage-stats/issues/01-wire-real-usage-counters.md`.
