# Tramp

A multi-platform desktop music player positioned as a spiritual successor to Winamp.

## Language

Most entries here are settled: the term means what it says, and the phrasings in `_Avoid_` were considered and rejected. Eight rest on a product bet nobody has tested. Those carry a `_Premise_` line — the bet, and the observable event that should reopen it — between the definition and `_Avoid_`. The dated record, with what each one costs today and what is genuinely known, is `docs/premises.md`; the section numbers match. An entry with no `_Premise_` line is vocabulary, not a wager.

**Tramp**:
The product — a desktop music player that can be built for Windows, Linux, and macOS.
_Premise_ (2026-08-21, accepted without evidence): that the homage is the product and the nostalgia is not — that people want this player rather than classic skins, visualisations, doublesize, global hotkeys and plugins. Revisit if over half of the first twenty unprompted reactions name a v1 non-goal as the thing that was missing — `docs/premises.md` §5.
_Avoid_: Winamp clone, media player (when meaning this product)

**Proxima Magnifica**:
The company that makes Tramp. Named on the About panel's maker's plate.
_Avoid_: publisher (vague); using the company name as the product wordmark

**Application id**:
The reverse-DNS id for the desktop file, Flatpak, icon theme, and Linux support dir: **com.proximamagnifica.tramp**.
_Avoid_: com.tramp.tramp (retired), com.tramp, treating this as the product name or Store listing name

**Free Forever**:
The product pledge on the maker's plate: Tramp costs nothing to use. Price, not a license name.
_Premise_ (2026-08-21, cost partly evidenced, capacity unevidenced): that one maintainer can carry five install channels and a GPL release indefinitely for nothing. Revisit if two consecutive releases slip for want of a human, or a recurring bill (the Apple Developer membership first) comes due with no decided payer — `docs/premises.md` §7.
_Avoid_: freeware, donationware (unless donations become a product); using this phrase to mean open-source

**Official download**:
How listeners obtain Tramp — the website (`https://tramp.music`). Not GitHub. Windows also has a Microsoft Store listing and Linux a Flathub listing; the site still offers an unsigned EXE and an AppImage when a store is missing or unwanted.
_Avoid_: GitHub Release as the product surface, treating Store or Flathub as the only install, shipping from the repo

**Store listing name**:
The Microsoft Store catalog title: **tramp.music**. The product is still Tramp; this is only the reserved Store name (the bare word was taken).
_Premise_ (2026-08-21, constraint evidenced, consequence unevidenced): the bare name being taken is fact; that a listener searching the Store finds and recognises `tramp.music` regardless is not. Revisit if support meets "I installed tramp.music, where is Tramp", or the bare name frees up — re-check that at each Store upload, since nothing else will surface it — `docs/premises.md` §6.
_Avoid_: renaming the product, using this as the wordmark, assuming the website EXE listing is named tramp.music

**Install channel**:
How this copy of Tramp was packaged — Microsoft Store, website EXE, Flathub, AppImage, or macOS DMG. The in-app new-version prompt is meant to follow that channel; nothing in the app detects the channel or prompts yet.
_Avoid_: flavor (compiler jargon), edition, SKU, treating the new-version prompt as shipped behaviour

**Classic skin**:
A Winamp-compatible skin that replaces the built-in chrome with the skin's own layout and art (WSZ). Out of v1 product path for mockup chrome.
_Avoid_: theme (when meaning a classic Winamp skin), skin pack (unless referring to a collection)

**Skin** (mockup recolor):
A shareable folder or zip (`skin.json` preferred, legacy `look.json` accepted + optional TTF/OTF) that recolors and optionally retypes the built-in mockup chrome — palette, a few named materials (bevel, spectrum/rail gradients), corner radii (window, black surface, button), and font roles — without changing layout or art. Window and surface radii stay rectangular: they cap at a quarter of the shorter side of the element they paint. Zero is a sharp corner. Friendly slug ids; the embedded default is **Tramp** (id `builtin`). Bundled homage skins (Arc, Shield, Thunder, Gamma, Widow, Marksman, Mind) sit beside it. May extend `builtin` or another skin. Not a classic Winamp WSZ skin. Catalog default directory name is `skins`.
_Premise_ (2026-08-21, constraint evidenced, bet unevidenced): that a skin community forms around retinting, while the existing Winamp community's artefacts are WSZ files v1 does not read. Revisit at six months from the first public download (2027-02-21 at the earliest) if no third-party skin has appeared anywhere visible and three or more people have asked for WSZ import — `docs/premises.md` §4.
_Avoid_: classic skin, WSZ, theme (when meaning this pack), graphite skin, look pack (retired product term — same concept)

**Host window**:
The single OS toplevel the compositor sees. Frameless; titled `Tramp`; the taskbar/pager entry. Geometry is the **virtual desktop** (bounding rectangle of every screen). It does not grow, shrink, or move with panel drags; it refits only when that desktop rectangle changes. Input is punched to panel shapes so the desktop is clickable in the gaps. Dragging the main panel’s title bar translates all panels inside the host (the cluster moves as a unit); dragging any other panel moves only that panel. Every panel stays fully on the virtual desktop.
_Premise_ (2026-08-21, cost evidenced, value unevidenced): that app-owned panel dragging is worth ~64 MB of translucent surface, a ~38 ms stall per second of dragging, a stream of Wayland edge cases, and no accessibility tree — keyboard navigation and that tree were deferred whole on 2026-08-21. Wayland offers no other mechanism, which is the strongest thing said for any premise here. Revisit if a compositor update breaks dragging again, or a wanted feature cannot be built on this shape — `docs/premises.md` §8.
_Avoid_: treating this as the main player canvas, extra toplevels, tight union of panels (retired host geometry)

**Panel**:
A product chrome surface (main, equalizer, playlist, settings, about, skins) that Tramp draws and moves inside the host window.
_Avoid_: OS window (for these surfaces), extra window, dialog (for settings/about/skins as product surfaces)

**App chrome**:
Tramp's own decoration — no OS title bar or standard window frame; the visible UI is six **panels** inside one **host window**, with Winamp-style docking among main/EQ/PL. Settings, about, and skins are freestanding (not snappable). Main player and equalizer never stretch; on-screen size follows the global zoom step only. The playlist panel may be freely resized. Main title bar carries logo + wordmark; EQ/playlist/settings/about/skins title bars show role title only. EQ band faders use a spectrum-gradient value fill.
_Avoid_: borderless (alone), frameless window (implementation jargon in product talk), Scalable UI (retired as a whole-chrome free-resize mode), stretching the main or EQ canvas, single-window EQ/PL swap (retired product model), five OS windows (retired host shape)

**Mockup chrome** / **code-constructed chrome**:
The built-in look for main, equalizer, and playlist — painted from the recipe in `player-mockup-2.html` (tokens, geometry, gradients, type, icon paths). No PNG panel faces or nine-slice graphite pack on the product path.
_Avoid_: graphite skin (as the *current* look), PNG skin, coded chrome (vague), vector chrome (alone), theme (when meaning this construction), Classic skin (Winamp WSZ — different thing)

**Graphite skin** *(historical)*:
The retired PNG-first chrome look (panel faces under `assets/skin/graphite/`). Kept only as history; must not steer new work.
_Avoid_: using this term for the current product look

**Session host**:
The single process that owns shared controllers (playback, playlist, EQ, zoom, settings) and the docking coordinator; the six panels are views onto that session inside one host window.
_Avoid_: multi-process, separate apps per window, extra OS windows per panel

**Docking** / **dock group**:
Winamp-style edge snap between panels. Dragging the main title bar translates every panel inside the host so the cluster stays together; main never snaps and never creates dock edges. Dragging EQ, playlist, settings, about, or skins moves only that panel on screen; siblings stay put. EQ or playlist peel their dock edges on drag; snap runs only on EQ/PL drag end. Settings, about, and skins never snap and are never snap targets. EQ and playlist may snap to any side, and on both axes at once (flush under main and against a neighbor in the same drop). Undock via peel or **Shift**: peel breaks the edges when a drag jumps far enough in one movement, and Shift breaks them however slowly the panel is dragged, leaving it where it was dropped instead of snapping back. A dock edge lives only as long as the contact it names: every placement re-checks each edge against the panels' rectangles and drops the ones that are no longer flush, so a crawl too slow to peel still ends up undocked, and a panel dropped back within snapping distance re-docks rather than being stranded claiming an edge it is nowhere near. A panel cannot hang off the virtual desktop. If a monitor is unplugged, the cluster is translated onto what remains when it still fits, otherwise each panel is clamped on its own and any dock edge that clamping breaks goes with it. Main minimize may hide/restore visible secondaries (including settings/about/skins) when the preference is on; always-on-top and main-minimize apply to the host window. Settings and skins stay raised among panels. The taskbar/pager shows Tramp (the host window).
_Avoid_: tiling WM, snap layouts (OS), tabs; docking settings/about/skins to main/EQ/PL; extra OS windows for docked surfaces; break-threshold undock (retired — never built, and Shift covers the slow drag peel misses)

**Playlist**:
An ordered list of playable tracks the user can manage (add, remove, reorder, play from).
_Avoid_: queue (unless a separate now-playing queue is later distinguished), library (the collection of known media)

**Playlist file**:
A saved playlist on disk (v1: M3U/M3U8) that Tramp can open and write. Its track lines are **hints, not addresses**: the same album is `\\server\share` on Windows and a mount point on Linux, and a mount point that moves leaves every absolute line stale, so a line that lands on nothing is re-read against the folder the playlist itself sits in. Tramp resolves those hints on **add** and **Refresh**, stores the resolved paths in the track-set cache, and never rewrites the listener's file to suit this machine — that would break it for the machine that wrote it. Clicking a saved playlist reads the cache, not the file.
_Premise_ (2026-08-21, accepted without evidence): that hints-not-addresses is worth the disabled rows and cache-versus-file drift it produces. Revisit if greyed-out tracks become the commonest support theme in a release cycle, a listener's own M3U is found rewritten, or reconciling cache against file needs a third entry point beyond add and Refresh — `docs/premises.md` §2.
_Avoid_: playlist document, playlist export (unless meaning a one-way share), fixing / correcting a playlist file (Tramp reinterprets, it does not edit)

**Playlist collection**:
The set of playlists Tramp keeps for the listener — *references* to playlist files at the paths the listener chose, never copies. Shown in the Playlist Manager's left panel.
_Avoid_: library (deliberately unspent — see Library), playlist library, collection (alone, where a track collection could be meant)

**Saved playlist**:
One entry in the playlist collection: a display name plus a reference to a playlist file. The name may differ from the filename; renaming an entry never renames the file.
_Avoid_: playlist entry, collection item, imported playlist (nothing is imported — the file stays where it is)

**Current playlist**:
The one playlist loaded for playback, shown in the Playlist Manager's right panel; what the transport plays from. May come from a saved playlist, from an arbitrary playlist file, or from tracks added ad hoc.
_Avoid_: active playlist, now playing (that is the transport's track, not this list), queue

**Altered current playlist**:
A current playlist whose track list has changed since it was loaded or last saved — tracks added, removed, reordered, sorted, or cleared. Navigating to another saved playlist asks the listener before discarding it. Only writing the whole list to a file makes it unaltered again.
_Avoid_: dirty (implementation jargon in product talk), modified playlist, unsaved playlist (a current playlist can be unsaved without being altered)

**Session resume**:
What a launch brings back. The **current playlist** always comes back — the **altered** list if one was left unsaved, otherwise the last saved playlist that was loaded, painted from the track-set cache — and so do panel positions, shade, playlist size and **zoom step**. What the **Resume playback** preference (on by default) governs is the **transport**: quitting while paused reopens that track and seeks back to where it was left, still paused; quitting while playing reopens it and plays, so Tramp is audible on launch. Quitting from **stop** keeps the current track (rewound, not playing): Resume playback reopens it paused at the start. Turn Resume playback off for a quiet launch with the playlist only.
_Avoid_: session restore (OS session management — a different thing), remembering the queue, autoplay (for the resumed-playing case), treating a stopped quit as a lost session

**Resume playback**:
The Settings row (on by default) that starts the transport on launch. Playlist, geometry and zoom always come back; this switch is only whether Tramp is audible. The retired label was *Resume last session*.
_Avoid_: Resume last session (retired label); treating the switch as what restores the playlist

**Empty state**:
The copy a well paints when it has no rows, and the main title when nothing is loaded yet. The playlist track well says `THIS LIST IS EMPTY` / `Drop files here, or open one from PLAYLISTS.`; the collection well says `NO SAVED PLAYLISTS` / `Tramp only saves references to your playlist files. You can name them whatever you want, without affecting the files. How you manage the files themselves is up to you.`; the main title is `Drop files to play` while the display title would be `No track` *and* the current list is empty. A stopped current track still shows its title; `No track` is only when nothing is loaded. There is no first-run flag and no tour — the empty wells carry the idea that music enters through files and playlists.
_Avoid_: wizard, onboarding, first-run tour, repeating the footer drop hint as a third well line, treating `No track` as the first-run title

**Disabled playlist**:
A saved playlist whose file was missing at the last validation pass. It cannot be *re-read* (Refresh is disabled) but it can still be opened: the current list is painted from the track-set cache. It can only be removed from the collection; if the file comes back, it enables itself again.
_Avoid_: broken playlist, orphaned playlist, deleted playlist (the entry survives — the file is what is gone)

**Track**:
A single local audio file Tramp can play; v1 supported kinds are MP3, AAC/M4A, FLAC, WAV, Ogg Vorbis, and Opus.
_Avoid_: song (when meaning the file), media item, clip

**Unplayable track**:
A track the transport was told to play and the engine then refused — the file has moved, the share dropped, the codec is absent. The transport stops reading as playing rather than sitting silently on it, and says which track and what the engine said. The listener's counterpart concept for a whole playlist is a **disabled playlist**.
_Avoid_: missing track (the file may be present and still unplayable), broken track, dead track, failed playback (as a state name)

**Disabled track**:
A current-playlist row whose file was missing at the last background check after a JSON-first load. It stays in the list, paints faint, and remains selectable. Play / double-click do not open it; Next / Prev / shuffle skip it. Footer TOTAL and N TRACKS omit it. Refresh (when the playlist file exists) drops it from the list and the cache and marks the current playlist altered; the listener's M3U is not rewritten until Save. Not an **unplayable track** (the engine never opened it) and not a **disabled playlist** (that is the M3U).
_Avoid_: missing track (as a state name), broken track, dead row

**Library** *(reserved — deliberately unused)*:
A persisted, browsable catalog of known tracks on disk, built by indexing designated folders. Out of scope for v1 — music enters via open, drag-and-drop, and playlists only. The word stays parked rather than repurposed: what Tramp stores is the **playlist collection**, which is not a track catalog.
_Premise_ (2026-08-21, accepted without evidence): that the listener already lives in folders and playlists rather than expecting scan-and-browse. Revisit if three or more people ask for a library, or if anything wants to *browse or search* the track-set cache rather than look one path up in it — `docs/premises.md` §1.
_Avoid_: collection, media database (in v1 discussions); using "library" for the playlist collection

**Zoom step**:
One of the discrete scale factors (75%, 100%, 125%, 150%; default **75%**) applied globally to the main, equalizer, and playlist panels’ logical canvases. Persisted — a saved factor that is no longer a step snaps to the nearest surviving one. Availability is measured against the **whole visible cluster**, never one panel: a step up is offered only while every open panel, scaled to that step, would still fit together inside the work area of the display the cluster sits on. So on a short display a tall stack can have zoom-in withdrawn from the start — including at first launch, with main, EQ and playlist all open — and **closing a panel can bring the step back**. A withdrawn step paints its button disabled rather than failing silently, because a button that does nothing and looks live reads as a defect. The step already in force is never withdrawn, or a layout restored onto a smaller display would have no way out of it. Changes via the main title bar’s zoom-in / zoom-out buttons, which are the only way to change it. Scales main/EQ canvases and the playlist’s stored logical size; does not replace playlist free resize.
_Premise_ (2026-08-21, accepted without evidence): that 75% is the right size to ship at, while fidelity is mockup-absolute at 100% and 75% is now the floor of the ladder — zoom-out at the default does nothing. Revisit if first-run size arrives unprompted in an issue or a store review, or a crispness defect needs 75% special-cased in drawing code — `docs/premises.md` §3.
_Avoid_: DPI scale (OS setting), continuous zoom, maximize (as a window-size control), per-panel zoom (product model is global), stretching main/EQ via panel drag, 50% / 200% / 250% / 300% (retired steps), zoom menu row / zoom keyboard shortcut (never built — see Accessibility in the v1 spec)

**Options cog**:
The cog at the top of the gutter left of the main player's **display well**. Under it sit **Skins** (comedy-and-tragedy mask glyph; latches while the Skins panel is up) and **Track info** (circled lowercase-i information glyph; dead until a track is loaded). The cog opens overflow: always-on-top (a check, applied to the **host window**), Open files… (the same enqueue as the eject glyph), Settings…, About Tramp, Quit. One cog plus two named buttons — the mockup's clutterbar is not the product chrome.
_Avoid_: clutterbar, clutter rail, **O** / **A** / **I** as product controls (all retired — the mockup's vertical letter strip, replaced by the cog); toolbar (when this control is meant); doublesize button (**D**), viz button (**V**); always-on-top as a per-panel or per-group control; Track info as a cog-menu row

**Skins panel**:
Freestanding panel (shade + close, no snap) that holds a **2-column matrix** of main-player preview PNGs, install zip/folder, skins folder, reset folder, and the install-error strip. Opened from the gutter Skins button. Click a preview to apply it; a trashcan on a non-active, non-Tramp, non-extended pack confirms then deletes that pack's files. Not a Settings tab.
_Avoid_: Skins tab (retired home), look packs dialog

**Settings**:
Freestanding panel with side tabs **General** | **Audio**. Audio is an empty stake. **Reset Settings** restores factory `settings.json` except `activeSkinId` and `skinsDirectory`.
_Avoid_: Skins as a Settings tab; Reset Settings as a skins-catalog action

**Phosphor**:
The **cyan** colour of the mockup chrome’s “screen glow” — used for lit LCD text, spectrum bars, and other live readouts (`#3de7ff` / hot variants). Not chartreuse.
_Avoid_: neon green, lime, acid chartreuse (retired graphite accent), LCD green (the older pure-green token)

**Rail**:
The cyan→magenta grip rails in the title bar. The mockup also filled the slack in the transport row and the playlist footer with brushed **rail** and **plate** strips; both are retired, so a button row now sits on the bare shell.
_Avoid_: rule, divider, separator (when meaning those accents); warm yellow rails (retired graphite title-bar accent); transport rail, footer plate (retired)

**Well**:
A recessed inset surface — groove, slider track, or `.screen` glass — cut into the chrome.
_Avoid_: trough, channel, inset (alone)

**Windowshade**:
The collapsed equalizer or playlist state that shows only the title bar, matching the classic shade gesture. Docking layout uses shaded height.
_Avoid_: minimize (window chrome), collapse (alone when shade is meant)

**Display well**:
The large recessed LCD glass area on the main player that holds spectrum, track title, times, and format metadata.
_Avoid_: LCD (alone), screen, display (when the inset glass region is meant)

**Failure surface**:
How Tramp tells the listener something went wrong — three tiers, and no queue. **modal** is a decision they must make now: the altered-playlist prompt, a playlist-save failure, and removing a skin; nothing else joins that list without a choice they can only make in the dialog. **persistent indicator** is a condition that stays true: no audio engine keeps the panel subtitle and also a durable mark in the **display well**; a settings or state-file write that has not yet succeeded is a Settings-row mark that stays until that file writes. **transient notice** is something that failed once and they can carry on: a skin install error stays on the Skins-panel strip and nowhere else; an unmeasured spectrum — a failed decode or one that ran past its 120 s deadline — is a display-well mark read from `Spectrogram::synthetic` on the session spectrogram. An unplayable track or an engine `open()` failure already stops and puts the reason on the subtitle; that is the surface, not a fourth tier. The Wayland file-chooser greying is a platform hole, not a product surface.
_Avoid_: a notification queue; painting a notice for an unmapped Wayland picker; reading per-frame `AudioLevels::synthetic` for the spectrum mark

**Synthetic levels**:
`AudioLevels` frames marked `synthetic: true` — a hard-fail / development signal when real spectrum cannot be measured. Not the product end-state; normal play must use real analyser levels.
_Avoid_: treating synthetic levels as the shipped spectrum design, fake levels (pejorative), mock levels, placeholder spectrum (as a planned deliverable)

**Shuffle**:
Playing the current playlist in a random order instead of top to bottom. The order is drawn one full pass at a time: every enabled row gets exactly one turn and **disabled tracks** are skipped. A fresh order is drawn each time shuffle is switched on and each time repeat-all wraps the list, and a new pass never opens on the track that just finished — so the same starting track does not deal the same evening twice. The order is unpredictable by design: it is not reproducible, not shareable, and not persisted.
_Avoid_: random (as the feature name), seeded shuffle, reproducible order, treating the order as something the listener can go back to

**Collection figures**:
The readouts in the About panel's stats well, under the heading ON THIS MACHINE. PLAYLISTS counts every entry in the **playlist collection**, a **disabled playlist** included. TRACKS and TOTAL TIME count each distinct track the collection references that was on the disk the last time Tramp looked — and it looks at launch, and when a playlist is added, refreshed or saved, never from a repaint, because a question per track is a stall on a share that has dropped. So ON THIS MACHINE means *as of the last time we looked*: a track deleted while Tramp is open keeps counting until the next add, Refresh or restart, and nothing on the panel says when the count was taken. SPINS is a lifetime tally of **spins**, not a reading of the disk.
_Avoid_: library figures (there is no **Library**), live counts / real-time figures (they are neither), treating a figure that has not caught up with a deletion as a bug

**Spin**:
One track played through to the end — the unit the About stats well counts. Earned when the track ends *and* at least 90% of its running time actually played, so dragging the seek bar past unheard audio does not buy one. The 10% slack is the fade and the applause, not a shortcut. A track that never reported a running time is taken at the end of its file — no length to measure against is not a reason to never count one.
_Avoid_: play (ambiguous with the transport verb), listen, stream

**Maker's plate**:
The brushed strip along the bottom of the About panel carrying the company mark, name, copyright, and website — named after the plate riveted to real hardware.
_Avoid_: footer, credits bar, branding strip
