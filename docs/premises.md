# Product premises

Eight decisions the rest of Aoide is built on. Every one is defensible; not one was reached by measurement. This file exists so they get revisited **on purpose** rather than inherited by default, and so a reader of [`CONTEXT.md`](../CONTEXT.md) can tell a settled term from a bet written in the same confident voice. Glossary entries that rest on one of these carry a `_Premise_` line pointing at the section number here.

**There is no telemetry.** No counters, anonymous or otherwise; nothing transmitted, nothing recorded locally to answer these questions. Aoide is **Free Forever** with no funding model, and shipping measurement infrastructure to settle a product argument is not a trade this project makes. Two of these premises look like they want a single counter each (§3 and §4). They are settled the other way instead: by a dated note saying they are unsettled, which is the honest version.

That decision puts all the weight on the triggers, so every trigger below is something a human notices without instrumentation — a support request, an issue, a store review, an hour of maintainer time, or the code getting harder to change. A trigger nobody will notice is the same as no trigger, so the last section grades them and names the weak ones.

## How to read a record

| Field | Means |
|---|---|
| **Status** | `Accepted without evidence` — nothing measured, on either side. `Constraint evidenced` — the fact forcing the decision is known, the consequence is not. `Cost evidenced` — the price is measured, the value bought is not. Variants of the last two name whichever half is actually known. |
| **Known** | Only what is verified, and where. Absent from this field means unknown, not unlikely. |
| **Cost** | What is being paid today, whether or not the bet pays off. |
| **Trigger** | The observable event that reopens the premise. |

A premise stays open until a trigger fires or someone replaces the record with evidence. Re-reading this file at each release is cheaper than re-deriving it, and it is the only review these get.

---

## 1. No media library

Recorded 2026-08-21 · **Status:** accepted without evidence

**The bet.** The listener Aoide is for already lives in folders and playlists — DJs, archivists, anyone with music curated on disk — so open, drag-and-drop, argv and M3U are enough. A scanned catalog would be effort spent on a listener the product has not met.

**Known.** Only that it is an explicit v1 non-goal ([`aoide-v1-spec.md`](aoide-v1-spec.md): How music enters, Non-goals), and that **library** is reserved rather than repurposed, so nothing has quietly claimed the word. Which listener actually arrives is unknown.

**Cost.** Nothing visible until someone arrives expecting scan-and-browse. One thing is closer than it looks, though: `playlist_tracks.json` already maps each track path in the collection to its duration and tag title, and About reports figures derived from it. That is a persisted catalog of known tracks — one browse surface short of the definition the word **library** is parked for.

**Trigger.** Reopen when either:

- three or more distinct people ask for scan-and-browse, or "where is my library", in the tracker, in support mail, or in a store review; or
- any planned feature wants to **search or browse** the track-set cache rather than look up one path in it.

The second is the code-side one and the more reliable: it fires the moment the cache stops behaving like a cache, during design, before a listener is disappointed.

---

## 2. Playlist-only organisation on paths-as-hints

Recorded 2026-08-21 · **Status:** accepted without evidence

**The bet.** Track lines are hints, not addresses; Aoide resolves them on add and Refresh and never rewrites the listener's file. This is worth more to a playlist-centric listener than the confusion it creates.

**Known.** The mechanism is real and documented (`CONTEXT.md`: **Playlist file**, **Disabled playlist**, **Disabled track**; [`architecture.md`](architecture.md): Playlist, Collection). Its cost is legible in the vocabulary itself: two entries exist only to name states that arise because paths are hints — **disabled playlist** and **disabled track** — and a third, **unplayable track**, names the engine's version of the same disappointment. How often a listener meets any of them is unknown.

**Cost.** The greyed-out row and cache-versus-file drift are permanent properties of the model, not bugs awaiting a fix. Clicking a saved playlist paints from the cache, so what is on screen is a description of a disk that may have moved since.

**Trigger.** Reopen when any of:

- "why is this track greyed out", or its disabled-playlist twin, becomes the most common theme in a release cycle's support and issues;
- a bug report shows a listener's own M3U was rewritten — the never-rewrite rule is the thing the cost buys, so if it breaks, the premise is paying and receiving nothing;
- reconciling cache against file needs a third entry point beyond add and Refresh. The model gets one notch harder to explain each time that set grows, and the explanation is the product.

---

## 3. Default zoom 75%, against mockup-absolute fidelity at 100%

Recorded 2026-08-21 · Reopened 2026-08-29 · **Status:** accepted without evidence for the default; 50% restored on one report; the tension is documented fact

**The bet.** 75% is the size that reads as right on a first run, even though every fidelity claim the project makes is made at 100%.

**Known.** The ladder was cut on 2026-08-21 from eight steps to four — 75, 100, 125, 150 — and the default stayed 75%, which made the default the floor. On 2026-08-29 one macOS listener running the first Mac build reported that 75% reads too big on a MacBook. 50% was put back on the ladder on that report. The default is still 75%. That makes these facts true at once:

- The default is **no longer the floor**. `prevZoomPercent(75)` returns 50, so zoom-out is live on a first run.
- Fidelity is mockup-absolute at 100% ([`architecture.md`](architecture.md): Notes), so the size everyone actually ships and screenshots is the one size the fidelity gate does not describe.
- 100% is the only step that lands the 825-wide main and equalizer canvas on whole logical pixels. The default gives 618.75, and 50%, 125% and 150% are fractional too; the heights are whole at every step. The one exact step is the one nobody starts on.

Which direction listeners move the zoom, or whether they touch it at all, is still unknown. One report is not a trend.

**Cost.** Every first impression, screenshot and bug report still arrives at a scale the fidelity contract does not cover. The cheapest reaction to "too big" is now available — it is opt-in, not the first-run size.

**Trigger.** Reopen when either:

- first-run size arrives unprompted again — a store review or issue saying the app is tiny, or still too big at 75%, enough to move the default rather than the ladder; or
- a fidelity or crispness defect at 75% needs the step special-cased in drawing code. At that point the default is buying a bug rather than a feel, which settles it without any counter.

---

## 4. Recolour-only skins

Recorded 2026-08-21 · **Status:** constraint evidenced, bet unevidenced

**The bet.** A skin community forms around retinting the mockup chrome — palette, a few named materials, font roles — with layout and art left alone.

**Known.** The format ships and is documented: `skin.json` (legacy `look.json` accepted), the embedded **Aoide** default plus seven bundled homage skins, install from a folder or a zip. Also known, and this half is fact rather than guess: the existing Winamp skin community's artefacts are **WSZ** files, and v1 does not read them ([`aoide-v1-spec.md`](aoide-v1-spec.md): Non-goals). The body of work the community already has is precisely the body of work Aoide cannot open.

**Cost.** A skin format carried, documented and version-tolerated for authors who may not exist, and a "no" to every WSZ request that arrives holding an artefact someone already made.

**Trigger.** Reopen at six months after the first public download — **2027-02-21** at the earliest, later if the download slips — if by then no third-party skin has appeared anywhere visible (an issue, a PR, a link) while WSZ import has been asked for by three or more distinct people. Both halves are countable by hand, and the ratio between them is the entire decision.

---

## 5. The Winamp homage as the product, not the nostalgia

Recorded 2026-08-21 · **Status:** accepted without evidence

**The bet.** People want a dense, control-forward, playlist-centric player that is good on its own terms. The homage is a shape, not a promise of feature parity.

**Known.** What the homage attracts cannot be known before it attracts anyone. What v1 refuses is written down and unambiguous: WSZ skins, visualisation modes and plugins (clutterbar **V**), doublesize (**D**), a plugin ecosystem, streaming. Global hotkeys are absent too — the whole keyboard surface is Space, Ctrl+A, Delete/Backspace, the four media keys, the options menu's own arrows and Enter/Escape, and Shift or Ctrl qualifying a mouse gesture ([`aoide-v1-spec.md`](aoide-v1-spec.md): Accessibility).

**Cost.** The first wave of attention is the wave most likely to want exactly the five things v1 does not do, and it arrives at the moment the product has the least slack to answer.

**Trigger.** Reopen when more than half of the first twenty unprompted reactions — issues, store reviews, forum replies — name a v1 non-goal as the missing thing. That is not evidence about listeners in general. It is evidence that the *framing* is recruiting the wrong expectation, and the framing (store copy, the site, the first screenshot) is still cheap to change; the non-goals are not.

---

## 6. A Store listing named `aoide.music` for a product called Aoide

Recorded 2026-08-21 · **Status:** listing name chosen, consequence unevidenced

**The bet.** A listener searching the Microsoft Store for Aoide finds `aoide.music`, recognises it, and installs it; the split between wordmark and catalog title costs nothing.

**Known.** The product is **Aoide**. The Store listing and official download domain are `aoide.music`; the MSIX identity is `ProximaMagnifica.aoide` ([`distribution.md`](distribution.md)). The website EXE and in-app chrome stay **Aoide**. The previous listing `tramp.music` is retired (historical: the bare word Tramp was taken). The listing name matching the domain is the cheapest way to keep the two names spelling the same thing.

**Cost.** Two names for one product, in the one channel where the listener cannot see the website that reconciles them.

**Trigger.** Reopen when a support request or store review shows the split confusing someone — "I installed aoide.music, where is Aoide".

---

## 7. "Free Forever" with no funding model

Recorded 2026-08-21 · Updated 2026-08-29 · **Status:** the one identified bill has a payer and is non-incremental; capacity unevidenced

**The bet.** One maintainer can carry a GPL-3.0-or-later desktop player across five install channels indefinitely, at no price to the listener, and never need money to do it.

**Known.** The release surface is real, and human at both ends: Partner Center and Flathub submit stay manual, and the MSIX identity version has to be bumped per upload. macOS notarization needs a Developer ID certificate, which means a paid Apple Developer Program membership — and on 2026-08-28 that stopped being an open question. The maintainer already holds that membership for reasons unrelated to Aoide, so the recurring bill this premise worried about has a named payer **and is not incremental to this project**: cancelling Aoide would not save it. A Developer ID Application certificate (G2 chain, valid to 2031) and the five signing and notary secrets are in place. The Mac is no longer missing either: on 2026-08-29 a notarized DMG was installed on a MacBook and played audio, and CI now builds, tests and smoke-starts that bundle on every run. It is one machine, and it cost two defects to find out (a title bar under the menu bar, and 75% reading too big), which is what a first launch is for. 1.0 ships four channels (Store, website EXE, Flathub, AppImage); the fifth (macOS DMG) is still 1.1. Maintainer capacity over twelve months is unknown; there is no twelve months of history to look at.

**Cost.** Whatever release and store chores cost, paid out of the same hours the product would otherwise get.

**Trigger.** Reopen when any of:

- two consecutive releases slip because the human submit steps had nobody;
- a recurring bill comes due with no decided payer — the Apple membership was the candidate and is settled, being pre-existing and non-incremental, so what to watch now is a cost this project would *add*;
- release and store chores take more maintainer time in a month than the product does.

All three are noticed by the person doing the work, which is the only measuring instrument this project has and, for this premise, a sufficient one.

---

## 8. The virtual-desktop host with fully custom chrome

Recorded 2026-08-21 · **Status:** cost evidenced, value unevidenced

The most expensive premise here, and the one most likely to be right.

**The bet.** App-owned multi-panel dragging and the mockup chrome are worth a virtual-desktop-sized translucent surface, a permanent stream of compositor edge cases, and no accessibility tree.

**Known — the costs, measured rather than estimated** ([`agents/title-bar-drag.md`](agents/title-bar-drag.md)):

- The host is one frameless toplevel sized to the bounding rectangle of every screen, with input punched to panel shapes. On the pairing host that is 4389×1188 at DPR 1.75 — about **64 MB per shm buffer**, up to five buffers.
- One **~38 ms stall per second of dragging** comes from committing that surface. Reproducible, and ruled out as the analyser tick, as CPU contention, and as app painting. It is a property of the shape, not a bug to chase.
- `WA_TranslucentBackground` means Qt never sets an opaque region, so the compositor cannot occlusion-cull anything beneath the host.
- Deferring the punch to reduce that cost was tried and undone. On KWin the mask is the hole the compositor actually shows and hits, so a deferred punch left ghost rectangles on the canvas. `grabMouse` is refused outright for non-popup windows on Wayland.
- **There is no accessibility tree.** Nothing under `src/` mentions `QAccessible` or `setAccessibleName`; the only `keyPressEvent` in the tree belongs to our own painted popup, and the tooltip explicitly takes `Qt::NoFocus`. Everything else — volume, seek, every EQ band, presets, the whole Playlist Manager, settings, skins, zoom, shade, dock — is mouse-only, and there is no focus indication to build on later. Shift-undock is not a counter-example: the modifier only qualifies a mouse drag, so it opens no keyboard route and leaves that one gesture needing both devices.

**Known — the other side, and it is the strongest claim in this file.** There is no alternative mechanism. Wayland has no `xdg_toplevel` set_position, so a panel-per-toplevel shape cannot place its own windows; `startSystemMove` would slide a virtual-desktop-sized toplevel; extra OS windows per panel is a retired shape for exactly these reasons. If app-owned dragging and docking are product requirements, this host is the way to have them, not a preference among several.

**What is unevidenced** is only the value side: whether listeners want the docking and the chrome enough to pay that bill. Nothing here measures that, and nothing will.

**Cost, stated plainly.** On 2026-08-21 keyboard navigation and the accessibility tree were **deferred whole** — not staged, not partially delivered. So the accessibility cost of this premise is not merely unpaid; it is postponed by decision. It is also the only cost in this file that excludes a person rather than inconveniencing one, and it should be the first thing picked up next regardless of what happens to the premise.

**Trigger.** Reopen when any of:

- a compositor update breaks dragging again after it was fixed, or Wayland and compositor issues outnumber product issues across a release cycle;
- a wanted feature cannot be built without changing the host shape. The shape is load-bearing for the layout and command-routing work already queued behind it, and every addition raises the price of reversing it;
- someone who needs a screen reader or the keyboard reports that Aoide is unusable. This is a **lagging** trigger and a poor one — it only fires after the harm — which is an argument for closing the accessibility gap on its own schedule rather than waiting for this premise to be reopened.

---

## Which triggers will actually fire

Written down so the weak ones are not mistaken for cover.

**Fire on their own, during work nobody can skip.** §1's browse-the-cache test and §2's third reconciliation path both fire in design or review. §3's "special-case 75% in drawing code" fires the same way. §6's name re-check rides a manual Partner Center step that already exists. §7 is noticed by whoever pays the bill or misses the release. §8's compositor and blocked-feature triggers show up on the pairing host and in the queue.

**Need a habit that does not exist yet.** §2's "most common theme" assumes support and issues are labelled by theme; today's triage vocabulary is status only, so one theme label applied at triage is what makes that trigger real. §4's six-month ratio needs the date carried forward — hence the literal date. §5's "first twenty reactions" needs someone to tally them in the launch window, once.

**Strained, and worth admitting.** §3 has no observable side other than complaints, which is exactly why a counter looked attractive; with telemetry ruled out it is the weakest-triggered premise of the eight, and its code-side half is doing most of the work. §5 depends on there being an audience at all. §8's accessibility trigger is lagging by construction, as noted above.
