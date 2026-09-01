# Distribution and CI

How Aoide is built and handed to listeners. Product page: `https://aoide.music`. GitHub Releases on a version tag are a **mirror**, not the product surface.

## Workflows

| Workflow | When | What |
|----------|------|------|
| [`.github/workflows/open-pr.yml`](../.github/workflows/open-pr.yml) | Push to a feature branch | Opens a PR against `main` if one is missing (`research/*`, `spike/*`, `wip/*` skipped) |
| [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) | PR and `main` | CMake build + `ctest`, paint budget, stage and staged-bundle smoke on Ubuntu, Windows and macOS; no signing, DMG wrap or artifact; PR review comment with the result |
| [`.github/workflows/merge-if-green.yml`](../.github/workflows/merge-if-green.yml) | CI completed | Squash-merges a same-repo, non-draft PR at that SHA when CI is green. Skips forks, drafts, and `do-not-merge` |
| [`.github/workflows/release.yml`](../.github/workflows/release.yml) | Tag `v*` or **Run workflow** | A metadata pre-flight, then Windows / Linux packages, a required macOS DMG job (smokes the staged app) and a required Flatpak job, then assemble the downloads; a tag also publishes them as a GitHub Release |

Merging happens **only** in `merge-if-green.yml`. It runs from the default branch
on `workflow_run`, so the branch being judged cannot rewrite the gate that judges
it — which is exactly what a `merge` job inside `ci.yml` would allow, so there
isn't one.

Every Windows and Linux packaging job **runs the thing it packaged** before it
is published: the staged binary, the AppImage, the extracted tarball and the
installed `.flatpak` each get `--bench-chrome` (links, finds its assets and
fonts, is optimised) and a `AOIDE_AUTO_QUIT=1` session start. `ctest` runs
against the build tree, which still resolves Qt and libmpv from the runner —
only these runs prove the artifact carries its own, so they clear the runner's
Qt out of the environment first. Libraries and **plugins** are found by
different mechanisms, so both have to go: Linux unsets `LD_LIBRARY_PATH` and
`QT_PLUGIN_PATH`, and Windows drops Qt's `bin` from `PATH` *and* unsets
`QT_PLUGIN_PATH`. Deleting one library or one platform plugin from the staging
directory fails the job. That is why `stage.ps1` deploys `qoffscreen` alongside
`qwindows` and asserts both landed: the smoke test runs the stage headless, and
it is no longer allowed to borrow the runner's copy. The Flatpak run proves the
opposite property, that it can get Qt from its runtime, and the job also asserts
no `libQt6*` reached its staging directory. That run is `flatpak run`, not
`sudo flatpak run`: newer flatpak refuses the sudo form so the sandbox does not
inherit root's environment.

Pull-request CI stages on every matrix host and smokes that stage the same
way: `--bench-chrome` and `AOIDE_AUTO_QUIT=1` after the runner's Qt is gone.
macOS unsets `QT_PLUGIN_PATH`, `DYLD_FRAMEWORK_PATH` and `DYLD_LIBRARY_PATH`.
That run proves the staged bundle starts offscreen with the Qt, libmpv, icns,
assets and skins it carries. It does not open a DMG and it does not verify
Gatekeeper. Pull-request CI does not sign, notarize, wrap a DMG or upload an
artifact — those steps are release CI, and only macOS has a notary. A failed
stage or smoke fails that host the same way a compile or `ctest` failure does;
`CI passed` exits 1 unless every leg is green.

Uploads use `if-no-files-found: error`, and the assemble job then requires an
EXE, an MSIX, an AppImage, a tarball, a DMG and a Flatpak to be present before
anything is published — a packaging step that quietly produced nothing used to
make a green job and a release short one download. Assembly also requires all
four packaging jobs to have **succeeded**, which closes the other way a download
goes missing: not a job that produced nothing, but a job that failed outright.
Assembling runs on **every** release run, not only on a tag, so the download
and the completeness check are exercised by **Run workflow** rather than first
attempted during a real release.

## Desktop metadata

Three files describe the app to a Linux desktop, all under `packaging/linux/`
and all installed by `CMakeLists.txt`, which makes `cmake --install` — and so
`stage_bundle.sh` — the single place that decides what ships. The tarball, the
AppImage and the Flatpak therefore carry the same metadata without three lists
agreeing to.

| File | Installs to | Supplies |
|---|---|---|
| `com.proximamagnifica.aoide.desktop` | `share/applications` | Launcher entry, `MimeType=` associations, `Exec=aoide %F` |
| `com.proximamagnifica.aoide.metainfo.xml` | `share/metainfo` | The **name**, summary and description an installer shows |
| `icons/hicolor/*/apps/com.proximamagnifica.aoide.png` | `share/icons` | Eight sizes, 16 through 512 |

The metainfo file is not optional decoration. A `.desktop` file's `Name=` never
reaches `flatpak`, GNOME Software or KDE Discover; AppStream data is what they
read, and without it they fall back to printing the app ID — so the app
installed as `com.proximamagnifica.aoide` rather than `Aoide`. Nothing in the
build said so: `flatpak-builder` only composes AppStream data when it finds
`/app/share/metainfo/<app-id>.metainfo.xml`, and when it does not it skips the
step silently, after which `flatpak build-bundle` embeds neither the name nor
the icon. The Flatpak manifest copies `share/` **whole** for the same reason —
it used to name `share/applications` and `share/icons` one by one, which is how
`share/metainfo` came to be dropped without an error.

Two gates hold it: `tool/check-metainfo.sh` runs `appstreamcli validate` and
checks the newest `<release>` against `VERSION` (in `ci.yml` and in the release
pre-flight that fronts every packaging job), and the Flatpak smoke test asserts
that the *installed* app's name is `Aoide`. Validation is `--no-net`, so a moved
screenshot host cannot fail a build over an input that is not in the repo.

`StartupWMClass=Aoide`, not the app ID: Qt builds X11 `WM_CLASS` from argv[0]'s
basename and `applicationName`, giving `"aoide", "Aoide"`, so the app ID matched
neither field and the key did nothing there. Wayland does not need it — `app_id`
already equals the desktop file's basename, which is what xdg-shell asks for and
what GNOME and KWin match on.

### Submitting to Flathub

Two manifests, two build models. The local
[`packaging/flatpak/com.proximamagnifica.aoide.yml`](../packaging/flatpak/com.proximamagnifica.aoide.yml)
wraps an already-staged tree; Flathub rejects that shape outright, because `dir`
sources and prebuilt binaries are both refused and everything must be
[built from source](https://docs.flathub.org/docs/for-app-authors/requirements).
[`packaging/flatpak/flathub/com.proximamagnifica.aoide.yml`](../packaging/flatpak/flathub/com.proximamagnifica.aoide.yml)
is the file that is submitted. It compiles from a pinned tag with no network
during the build, which is why every source carries a `sha256` or a `tag` **and**
the commit that tag resolves to.

The submission is a PR against [flathub/flathub](https://github.com/flathub/flathub)
based on the **`new-pr`** branch — a PR against `master` is closed
automatically — titled `Add com.proximamagnifica.aoide` and containing only that
manifest. The metainfo, desktop file and icons stay here and are picked up from
the build; Flathub explicitly does not want copies in the PR. The template also
asks for a video of the app running as a Flatpak. On approval Flathub creates
`flathub/com.proximamagnifica.aoide`, invites the submitter as a collaborator
(2FA required, accept within a week), and later releases are PRs against that
repo rather than another submission.

Rehearse it exactly as Flathub builds it before opening anything:

```bash
flatpak install -y flathub org.flatpak.Builder
flatpak run --command=flathub-build org.flatpak.Builder com.proximamagnifica.aoide.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest com.proximamagnifica.aoide.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
```

`appstreamcli validate` is necessary but not sufficient — the linter adds the ID,
screenshot and permission checks. Use the `flathub-build` wrapper rather than
plain `flatpak-builder`: it passes `--sandbox` and the screenshot-mirroring flags,
and without them the repo lint reports `appstream-external-screenshot-url` for a
manifest that is actually fine.

Two linter errors are expected and neither is fatal —
`finish-args-host-filesystem-access` and `finish-args-kwin-talk-name`, both
documented as *granted on sufficient explanation being provided*. The
justifications are below and above respectively; the filesystem one is the
argument reviewers will actually push on, and Haruna carries `host:ro` plus
`home` on the same runtime. Anything else the linter reports is a real defect.

Reviewers also open the domain in the app ID by hand to confirm it is the
submitter's, and the same domain later carries the verification token at
`/.well-known/org.flathub.VerifiedApps.txt`. That is **proximamagnifica.com**,
not the `github.com` source repo and not `aoide.music`; it should say somewhere
that it publishes Aoide.

### Screenshots

Done as of 1.2, and the one step that cannot be done from the repo. Flathub
requires screenshots and AppStream `<image>` must be a URL a store can fetch, so
they cannot ride inside the bundle. Three are live under
`https://aoide.music/screenshots/1.0/` and the `<screenshots>` block naming them
is no longer commented out, so `--check-urls` now has something to verify and
`flatpak-builder-lint` no longer reports `metainfo-missing-screenshots`. Flathub
**mirrors** them to `dl.flathub.org` when it composes an official build rather
than hotlinking, so the URLs have to resolve when a build runs, not only when a
listener looks. They still show 1.0 chrome; regenerate and upload under a new
version prefix when the chrome changes enough to misrepresent the app. The
pictures are generated, not committed:

```bash
QT_QPA_PLATFORM=offscreen ./build/aoide --dump-chrome /tmp/shots
```

Take `main_player_window.png`, `playlist_window.png` and
`equalizer_window.png` and upload them under
`https://aoide.music/screenshots/<version>/`, then point the `<screenshots>`
block at the new prefix. `check-metainfo.sh --check-urls` — which the release
pre-flight runs — fetches every declared `<image>` and fails on anything that is
not a 200, so a moved or deleted picture breaks the release rather than the
listing.

`--socket=pulseaudio` in the manifest is why installers warn about microphone
access. Flatpak has exactly one audio permission and it covers capture and
playback together, so there is nothing to narrow; every music player on Flathub
carries it. Aoide has no recording path.

`--filesystem=host` **stays**, decided rather than deferred. It is what rates the
app *potentially unsafe*, where the microphone row is only *probably safe*, so
the temptation to narrow it is real — but the label is the whole of what removing
it buys, and the cost is reach.

Narrowing means naming directories, and every path outside them becomes a dead
row until the user re-grants it through a picker. `xdg-music` is not where music
actually is: a download sits in `~/Downloads`, a collection often sits on an
external drive under `/run/media`. Aoide opens the files you already have, which
is the whole premise, so a permission that assumes they are filed tidily is the
wrong trade.

Drops are the part that *could* be fixed and is not the reason: Qt has never
implemented the receiving half of the FileTransfer portal ([QTBUG-91357][qtbug]),
so a dropped file arrives as a bare host path, and the D-Bus `RetrieveFiles` call
would have to be written by hand. Portal *picks* need nothing — they are
registered persistently and directories are exported too.

Revisit if Flathub starts gating on the rating, or if Qt ships the drop half. The
export fix below stands either way: it is a prerequisite for narrowing, not
something narrowing would undo.

[qtbug]: https://bugreports.qt.io/browse/QTBUG-91357

### Portal exports expire, so they are not what gets stored

Opening a file from a file manager does not hand the app that file's path. It
hands over an *export* of it under `$XDG_RUNTIME_DIR/doc`, and an export made for
a launch is held only in the document portal's memory — it is never written to
`~/.local/share/flatpak/db/documents`. That is what made this hard to catch: the
export **outlives the process it was handed to**, so opening a track, quitting
and relaunching restores it perfectly. It dies with the portal service, at
logout. The next day the row is dead while the file itself never moved.

`src/document_portal.cpp` reads the `user.document-portal.host-path` xattr the
portal sets on the export and hands `openPaths` the real path, so what reaches
the playlist and the state files is the durable one. Verified against a live
export rather than a mock, before and after:

```bash
# before: the export path is what got written down
"path": "/run/user/1000/doc/nbpeE9v4izUGiQ0CXz1BUw/openwith-test.mp3"
# after
"path": "/tmp/openwith-test.mp3"
```

Reproducing it needs the enqueue case, because only an *altered* list is
persisted: a bare "Open with" replaces the playlist and is not written down at
all, so seed `altered_playlist.json` with one track first, then pass an export
as argv.

The rewrite is deliberately conditional on the origin being **readable**. With
`--filesystem=host` it is; narrow the sandbox and it is not, and there the
expiring export is the only handle the app has — better than a host path nothing
can open. So this change is also a prerequisite for narrowing rather than
something narrowing would undo.

`unzip` for skin installs was the other Flatpak lead, and it is a non-issue:
`org.kde.Platform` 6.11 ships it, and `look.cpp` already declines a zip it
cannot unpack instead of failing. Windows is the one host with no `unzip`, and
it uses `%SystemRoot%\System32\tar.exe` instead — see
[`architecture.md`](architecture.md).

`--filesystem=xdg-run/aoide:create` exists for one file: the KWin script behind
always-on-top. KWin opens it by path from its own process, and a sandbox's
`$XDG_RUNTIME_DIR` is a private mount the host cannot see, so the script has to
live in a directory shared at the same absolute path on both sides. This shares
that one directory. It was found by reproducing the failure against a live KWin
from inside a sandbox: without it, `loadScript` returns a valid id and `run()`
answers `org.kde.kwin.Scripting.FileError: Could not open …`; with it, `run()`
returns cleanly.

## Qt version

The [`QT_VERSION`](../QT_VERSION) file is the authority: one official desktop
kit, built and tested against everywhere something ships, and the same kit
`./build.sh` compiles against. It is **6.11.1**. `QT_RUNTIME` is its
major.minor (`6.11`), which is what both Flatpak manifests and
`org.kde.Platform` use. Workflows read the file via
[`tool/export-qt-pin.sh`](../tool/export-qt-pin.sh).

Which line to sit on is not a free choice, because Flathub
[requires](https://docs.flathub.org/docs/for-app-authors/requirements) the
newest runtime at submission time and CMake fails the build unless the Qt it
finds equals the pin exactly. The runtime's Qt therefore sets the pin, not the
other way around: `org.kde.Platform` 6.11 ships 6.11.1, so the pin is 6.11.1,
and it moves when KDE's newest runtime moves. Picking a newer official kit than
the runtime carries — 6.11.2 exists — would build every bundled artifact fine
and fail the Flathub build.

Both Flatpak manifests repeat the pin as a literal. `make_flatpak.sh` compares
the local one's `runtime-version` against `QT_VERSION` and refuses to build on a
mismatch, so a Qt bump surfaces there instead of as a `flatpak-builder` error
about a runtime nobody installed. Nothing guards the Flathub manifest that way,
because a stale pin there fails loudly in the sandbox on the CMake check.

Linux installs that Qt through `install-qt-action` / `./tool/fetch_qt.sh`
rather than apt or Homebrew: the runner's `qt6-base-dev` is 6.4.2, and
Homebrew's `qtbase` moves ahead of the pin. The install is the official
desktop base sliced to `qtbase`, `qtwayland`, and `icu`. `qtwayland` is a
base *archive* (the client QPA plugin the AppImage stages); it is not an aqt
module. The module of that name does not exist on 6.11 — `qtwaylandcompositor`
does, and that is a compositor SDK, not the plugin. `icu` is the bundled ICU
the official Linux `qtbase` links against.

`./tool/fetch_qt.sh` is **Linux only** — it exits 1 on Darwin. The macOS CI
and release jobs install the same pin with `jurplel/install-qt-action` (`host: mac`,
`target: desktop`, `arch: clang_64`, archives `qtbase qttools`). `qttools` is
there for `macdeployqt`; on Windows that tool lives in `qtbase`, which is why
the Windows job can slice further. A local Mac host places the official desktop
kit under `.local/qt/<QT_VERSION>/macos` or points `CMAKE_PREFIX_PATH` at it.

**Windows needs an unreleased `aqtinstall` to install Qt 6.11 at all**, and both
Windows jobs pin one through the action's `aqtsource` input. Qt restructured that
repository at 6.11: 6.10.3 was a single nested `qt6_6103/qt6_6103`, while 6.11
splits into per-architecture directories (`qt6_6111/qt6_6111_msvc2022_64`). The
newest release on PyPI is 3.3.0 and only knows the old shape, so it fails with
`Failed to locate XML data for Qt version 6.11.1` before downloading anything —
which is how the 6.11 bump passed Linux and macOS and failed Windows in a minute.
The fix was merged upstream in March 2026 and is still unreleased, so the pin
names that commit. It is deliberately **Windows only**: the layout change does not
touch `linux_gcc_64` or `clang_64`, and the lanes that already work have no reason
to move onto an unreleased installer. Replace both with a plain `aqtversion` once a
release past 3.3.0 carries it. The consequence worth knowing before the next Qt
bump is that Windows is now the lane most likely to break on one.

[`build.sh`](../build.sh) fetches the pin into `.local/qt/` if it is missing
and refuses to link any other version. CMake does the same check.

Cut a release by bumping [`VERSION`](../VERSION) and adding a newest
`<release>` to the AppStream metainfo that carries the same number —
[`check-metainfo.sh`](../tool/check-metainfo.sh) compares the two and fails
every Linux CI run while they disagree, so a bump without the entry breaks
`main` rather than the tag. Commit both, then:

```bash
git tag v1.2
git push origin v1.2
```

The tag name without `v` must equal the `VERSION` file.

## Release notes

Notes are a **list of features**. Each entry names the capability a listener
gains; a very concise explanation follows a dash only where the name is not
self-evident. State what the listener can now do rather than the work that was
done — `Fixed a bug where…`, `Added support for…` and `Updated the…` read as a
commit log, which is the failure mode. No file names, symbol names, commit ids
or version numbers inside an entry; the release already carries its number.
Only what a listener can notice belongs here, so CI work, refactors, packaging
plumbing and doc updates do not appear. Release 1.2 is the reference:

```
- Resize the playlist from any edge or corner — not only the bottom-right grip.
- The player stays above its own panels — no equalizer, playlist, settings,
  about or skins window covers it.
```

**They live in one place**: the `<release>` description for that version in the
AppStream metainfo, as a `<ul>` of `<li>` features. That is what software
centres render, and [`release-notes.sh`](../tool/release-notes.sh) turns the
same list into the GitHub release body — the only derivation, so the notes
cannot be written twice and drift. It refuses to print an empty body, and
[`check-metainfo.sh`](../tool/check-metainfo.sh) calls it, so a bump whose
entry is missing features fails CI instead of publishing a release page with
nothing on it.

Standing facts — where downloads live, which channels exist, what notarization
means — are properties of the product, not of a release. They live once in the
publish step of [`release.yml`](../.github/workflows/release.yml), after the
features. A body that was only those facts is what shipped 1.1 with no feature
list at all.

## Artifacts

| File | Channel |
|------|---------|
| `Aoide-<ver>-windows-x64.exe` | Official download (unsigned Inno; SmartScreen click-through) |
| `Aoide-<ver>-windows-x64.msix` | Microsoft Store listing **Aoide** (unsigned here; Store re-signs). Identity version is four-part `x.y.z.0` derived from `VERSION` by [`tool/version.sh`](../tool/version.sh) (`x.y` → `x.y.0.0`); the fourth number must be **0** or Partner Center rejects the package. `make_msix.ps1` validates the shape it is handed; it does not derive it. Bump `VERSION` for each Store upload. |
| `Aoide-<ver>-linux-x86_64.AppImage` | Official download |
| `Aoide-<ver>-linux-x86_64.tar.gz` | Official download, portable layout |
| `Aoide-<ver>-linux-x86_64.flatpak` | Sideloadable bundle — a second Linux channel, not a fourth OS. It feeds **nothing** on Flathub: that build compiles from the source tag instead, so no release artifact is an input to it. Required like the rest, and `flatpak-builder` pulls `org.kde.Platform` over the network, so this is the job most likely to fail for a reason that is nobody's bug. Re-run it; a flake costing a re-run is cheaper than a release quietly short one download. |
| `Aoide-<ver>-macos-universal.dmg` | Official download since **1.1**. Release CI wraps and uploads one, notarized wherever the signing secrets reach. Pull-request CI does not upload a DMG. One has been installed on a MacBook and played audio. The smoke proves the staged bundle starts offscreen — not that a listener can open the DMG past Gatekeeper. |

Partner Center and Flathub submit stay **human**. Packaging scripts live under `packaging/`.

## Store listing

The Microsoft Store listing text is typed into Partner Center and nothing here validates it, so it is written down below instead. Edit it at Partner Center → the submission → **Store listings** → the language → the field; a listing filed under both `English` and `English (United States)` carries two copies of every field. Changing this text needs no rebuild and no `VERSION` bump — the package certifies separately from the listing.

Two policy limits shape the copy, and 1.2 was rejected on 2026-09-01 for the first of them.

**Keywords** — the field certification reports still call **Search terms**, under **Additional information**. Policy **10.1.3** caps them at seven unique terms, requires each to be relevant, forbids pricing terms, and forbids any term that is a product title the account does not publish. `Winamp` is such a title, and it alone failed 1.2: the package certified, the field did not. The limits the Partner Center UI displays are that tool's own, not the policy.

**Unshipped features** — 10.1.3 governs keywords only, so naming another product in the description is not itself a violation. What is: policy **10.1.1** requires metadata to describe a product's actual functions and limitations and forbids misleading customers about "features, functionality, or relationship to other products". The site's `Spotify and Last.fm support coming soon` is roadmap, and roadmap does not go in a listing — it describes functionality the package does not have and implies a relationship with two companies. Keep the listing to what the build in the package actually does.

Field limits, from the Partner Center listing docs: description **10,000** characters of plain text, required, no HTML and no URLs (links belong on the Properties page and are not clickable here); short description **1,000** but keep it under **270**, since longer text is truncated behind a link; product features up to **20** at **200** characters each, auto-bulleted, so do not type bullets; What's new **1,500**; keywords **7** terms of **40** characters and **21** words in total.

### Approved copy (1.2)

Short description:

```text
A music player for the music you already have on your disk. Open a folder, press play, and keep your playlists exactly the way you want them. No library to import, no account to make, and nothing phoning home. Free forever, and open source.
```

Description:

```text
Aoide is a music player for the music that lives on your own disk.

There is no library to import and no account to make. Open tracks and playlists from the file picker, drop a whole folder onto the window, or send them over from your file manager — from any drive, internal, external, or across the network. MP3, AAC and M4A, FLAC, WAV, Ogg Vorbis and Opus all play. Aoide reads the artist and album already written into your tracks, and leaves them alone.

Playlists are the heart of it. Drag tracks in, put them in the order you want, and save the list as M3U or M3U8 — a real file, in your folder, that any other player can open. Your saved playlists sit beside the songs inside them, so you can move between them without losing your place. Pick a handful of files and save them as a playlist of their own, and it takes over without cutting off the track you're on.

Aoide keeps references to your files. It never copies your music into a library of its own, and it won't quietly rewrite a playlist you wrote. Close it and open it tomorrow and everything comes back the way you left it — same playlist, same track, same place in the track.

There is a ten-band equaliser with a preamp and a shelf of presets, a twenty-bar spectrum that follows what's playing, and a MONO button for anyone who listens with one ear. The player, the equaliser and the playlist manager dock together into one tidy stack, or pull apart and sit wherever you like on screen. Scale the whole thing up or down to suit your monitor, and keep it above your other windows while you work.

Eight skins come with it, and it takes more from a folder or a zip — no store, no account, no converter. You can bring your own font while you are at it.

Aoide is free forever. No sign-up, no trial, nothing to unlock later, and no ads. It collects nothing and sends nothing anywhere. It is open source under the GPL, and it doesn't look like it.
```

Product features, one per line, no bullets:

```text
Plays the files on your disk — MP3, AAC, M4A, FLAC, WAV, Ogg Vorbis and Opus
No library to import, no account, no sign-up
A playlist manager that shows your saved playlists beside the songs inside them
Save playlists as M3U or M3U8 files you own and can open anywhere
Make a playlist from picked files without cutting off the track you're on
Drag and drop single files or a whole folder onto the window
Picks up where you left off — same playlist, same track, same spot
Ten-band equaliser with a preamp and presets
A twenty-bar spectrum that follows what's playing
A MONO button for listening with one ear
Player, equaliser and playlist dock together, or pull apart and sit where you like
Eight skins included, and more from any folder or zip
Bring your own font
Sits above your other windows while you work
Scales up or down to suit your screen
Free forever — no trial, no ads, nothing to unlock
Open source, and it collects nothing
```

Keywords — seven terms, fourteen words:

```text
music player
mp3 player
audio player
local music files
playlist manager
equalizer
offline music
```

Every claim above is checked against the build: no gapless, no crossfade, no scrobbling, no tag editing and no global hotkeys appear, because the product does none of them. The prose keeps the house spelling **equaliser**; the keyword uses **equalizer**, which is the spelling shoppers type.

## GitHub configuration

Actions must be allowed to open PRs: **Settings → Actions → General → Workflow permissions → Allow GitHub Actions to create and approve pull requests**. Without that, you still open PRs by hand; CI and merge-if-green still apply. That box stays ticked for `open-pr.yml`; the `passed` job in `ci.yml` leaves a `--comment` review, which is not an approval and does not need it.

Label a PR `do-not-merge` (or convert it to draft) to keep it open after a green CI.

### Outside contributors

The repo is public, so anyone can fork and open a PR. Three things stand between
that and the default branch, and they are settings rather than code — a fresh
clone does not carry them.

| Setting | Value | Why |
|---------|-------|-----|
| Actions → Fork pull request workflows | Require approval for **all** external contributors | Otherwise a stranger's PR runs its own code on the runners as soon as they have one merged contribution |
| Actions → Workflow permissions | Default `GITHUB_TOKEN` is **read** | Every workflow here declares its own `permissions:` block, so nothing relies on the write default |
| Ruleset **main** | Active, admin bypass | Blocks deletion and force-push, and requires a PR whose `Qt (ubuntu-24.04)`, `Qt (windows-latest)`, `Qt (macos-latest)` and `CI passed` checks are green. All three platforms are named, so a break on any host blocks the merge. Squash only, matching `merge-if-green` |

The ruleset requires **zero** approving reviews on purpose: `merge-if-green.yml`
merges as `GITHUB_TOKEN`, which cannot approve its own PR, so any non-zero count
would stop auto-merge dead. GitHub adds
`require_extra_approval_for_unattributed_changes` by default, which does demand
one review when a PR carries commits it cannot attribute to the author — rare on
a same-repo branch, and the admin bypass covers it.

Fork PRs never auto-merge regardless: `merge-if-green.yml` exits early when
`head_repository.full_name` differs from the repo. No workflow uses
`pull_request_target`, which is the usual way a fork PR reaches write scope, and
there are no repository secrets for one to reach.

[`.github/CODEOWNERS`](../.github/CODEOWNERS) names the owner for every path. The
ruleset does not require code-owner review yet, so today it only auto-requests it.

### Variables (optional)

| Variable | Purpose |
|----------|---------|
| `MSIX_PUBLISHER` | Store identity `CN=...` from Partner Center **Identity details**. Default `CN=Proxima Magnifica`. |
| `MSIX_IDENTITY_NAME` | Package identity name from those same details. Default `ProximaMagnifica.aoide`. |

The MSIX **display name** is `Aoide` — the name reserved in Partner Center, and the same word the website EXE and in-app chrome use. It must spell the reservation exactly: Partner Center rejects a package whose `Package/Properties/DisplayName` is a name the account has not reserved, which is how the inherited `aoide.music` title (see [`premises.md`](premises.md) §6) surfaced on the first real submission. Paste Publisher and Identity Name from Partner Center into those variables as soon as the app exists there; a mismatch fails certification.

The Store product must be created as an **MSIX or PWA app** (Partner Center → New product). The other type, **EXE or MSI app**, has no upload control at all — it takes a URL to an installer you host, accepts only `.exe` or `.msi`, and requires that binary to be Authenticode-signed by a CA in the Microsoft Trusted Root Program. Aoide's Inno EXE is deliberately unsigned, so that route would cost a code-signing certificate; the MSIX route is free because the Store re-signs. Picking the wrong type is not reversible in place — the product has to be recreated, and the name reservation moved with it.

### Secrets (macOS notarization)

All five signing and notary secrets are **set** on the repository as of
2026-08-28, from a Developer ID Application certificate on the G2 chain valid to
2031 ([`premises.md`](premises.md) §7). A release run has signed and notarized
a DMG rather than skipping, and one of those images has been installed on a
MacBook and played audio. The release job is required, so a packaging or notary
failure costs the release rather than one download. Pull-request CI does not
sign, wrap or upload a DMG. Read
the log; a green tick on those steps is not proof the image is what a listener
would get past Gatekeeper.

Skip any of these and the release Mac job still uploads an unsigned DMG;
`packaging/macos/notarize.sh` no-ops with a warning when the certificate pair
is unset.

| Secret | Purpose |
|--------|---------|
| `MACOS_CERTIFICATE_BASE64` | Developer ID Application `.p12`, base64 |
| `MACOS_CERTIFICATE_PASSWORD` | Password for that `.p12` |
| `APPLE_API_KEY_BASE64` | App Store Connect API `.p8`, base64 (preferred notary) |
| `APPLE_API_KEY_ID` | Key id |
| `APPLE_API_ISSUER` | Issuer UUID |
| `APPLE_ID` | Fallback notary: Apple ID email |
| `APPLE_APP_SPECIFIC_PASSWORD` | Fallback notary |
| `APPLE_TEAM_ID` | Fallback notary team id |

Create the `.p12` from a **Developer ID Application** certificate (not Apple Development). Prefer an App Store Connect API key over an app-specific password.

## Local packaging

Same scripts the workflows call, after a Release Qt build on that OS:

```bash
# Linux — needs patchelf as well as the Qt and libmpv development packages
./tool/stage_linux_libmpv.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./packaging/linux/stage_bundle.sh
./packaging/linux/make_appimage.sh
```

`stage_bundle.sh` deploys Qt into the bundle and rewrites every runpath to
`$ORIGIN`, so the tarball and the AppImage carry their own Qt and run on a
machine that has never installed one. It deploys the Qt the binary was **linked
against**, so build and package on the same machine, and on the oldest glibc you
intend to support — the host still supplies the loader, the C/C++ runtimes and
the GL driver. Any `DT_NEEDED` soname left neither staged nor on the
host-provided list **fails** the script: the smoke tests run on a machine that
still has that library installed, so they would pass and the listener would be
the one to find out.

The Flatpak is built from the same tree by the same script, and is the one
artifact that must **not** carry Qt — `org.kde.Platform` is a Qt runtime, so a
bundled Qt is dead weight that can shadow the runtime's:

```bash
./packaging/linux/make_flatpak.sh   # runs stage_bundle.sh --no-qt for you
```

`flatpak-builder` strips the staged binary with host `eu-strip` (`elfutils`).
Ubuntu only Recommends that package, so a `--no-install-recommends` install of
`flatpak-builder` is not enough — the release job installs `elfutils` next to
it. Locally: `elfutils` plus the KDE runtime/SDK that match `QT_RUNTIME`.

That is the only caller of `--no-qt`, and it stages into
`build/linux/flatpak-bundle` so `build/linux/bundle` keeps the Qt the tarball
and the AppImage need. One script and one code path for all three; do not give
the Flatpak its own staging script or strip Qt out afterwards, because then two
places have to agree on what Qt is.

Windows (on a Windows host): `tool/fetch_full_libmpv.ps1`, CMake Release build, then `packaging/windows/stage.ps1`, Inno (`ISCC /DMyAppVersion=<version> packaging\windows\aoide.iss`) and `packaging/windows/make_msix.ps1 -Version <msix>`. Take both fields from [`tool/version.sh`](../tool/version.sh); neither packager will run without one, because a default would name the artifact for a release it is not. The EXE installer runs `vc_redist.x64.exe` when `MSVCP140.dll` / `VCRUNTIME140.dll` are missing. The MSIX declares `Microsoft.VCLibs.140.00.UWPDesktop` so the Store supplies that runtime. Keep the `.ps1` files ASCII: Windows PowerShell 5.1 (what `powershell` is on the runner) reads UTF-8 source as ANSI, and an em-dash inside a string is decoded as a closing quote.

macOS (on a Mac — pull-request CI builds and smokes the staged bundle;
release CI wraps a DMG; one image has been installed and played):

```bash
# Qt is the official desktop kit at the QT_VERSION pin, not ./tool/fetch_qt.sh
./tool/fetch_full_libmpv.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./packaging/macos/stage_app.sh
# notarize.sh signs the staged Aoide.app, then calls make_dmg.sh so the
# sealed bundle is what gets wrapped. Without MACOS_CERTIFICATE_BASE64 /
# MACOS_CERTIFICATE_PASSWORD it no-ops and does not write a DMG.
./packaging/macos/notarize.sh
# only if that no-op'd:
./packaging/macos/make_dmg.sh
```

`stage_app.sh` honours `AOIDE_BUILD_DIR`, `AOIDE_BUNDLE_DIR`, and
`AOIDE_MAC_APP`. `make_dmg.sh` / `notarize.sh` honour `AOIDE_MAC_APP` and
`AOIDE_MAC_DMG`. The default image is
`build/macos/Aoide-<ver>-macos-universal.dmg`. The bundle is **`Aoide.app`**,
not `aoide.app`. `packaging/macos/aoide.entitlements` is the hardened-runtime
exceptions (`allow-jit`, `allow-unsigned-executable-memory`,
`disable-library-validation`) applied only when signing actually runs.
