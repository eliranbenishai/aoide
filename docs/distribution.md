# Distribution and CI

How Aoide is built and handed to listeners. Product page: `https://aoide.music`. GitHub Releases on a version tag are a **mirror**, not the product surface.

## Workflows

| Workflow | When | What |
|----------|------|------|
| [`.github/workflows/open-pr.yml`](../.github/workflows/open-pr.yml) | Push to a feature branch | Opens a PR against `main` if one is missing (`research/*`, `spike/*`, `wip/*` skipped) |
| [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) | PR and `main` | CMake build + `ctest` on Ubuntu, Windows and macOS; macOS also stages and uploads a DMG artifact; PR review comment with the result |
| [`.github/workflows/merge-if-green.yml`](../.github/workflows/merge-if-green.yml) | CI completed | Squash-merges a same-repo, non-draft PR at that SHA when CI is green. Skips forks, drafts, and `do-not-merge` |
| [`.github/workflows/release.yml`](../.github/workflows/release.yml) | Tag `v*` or **Run workflow** | Test, then Windows / Linux packages and a macOS DMG job (`continue-on-error`; smokes the staged app), then assemble the downloads; a tag also publishes them as a GitHub Release |

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

The macOS CI job now smokes the staged `Aoide.app` the same way: `--bench-chrome`
and `AOIDE_AUTO_QUIT=1` after unsetting `QT_PLUGIN_PATH`, `DYLD_FRAMEWORK_PATH`
and `DYLD_LIBRARY_PATH`. That run proves the staged bundle starts offscreen with
the Qt, libmpv, icns, assets and skins it carries. It does not open the DMG and
it does not verify Gatekeeper. Those packaging steps (stage, smoke, sign/notarize,
upload) are `continue-on-error` so Apple's notary cannot block a merge; a compile
or `ctest` failure still does. The release job stays `continue-on-error` as a
whole so a packaging failure cannot fail a Windows/Linux release.

Uploads use `if-no-files-found: error`, and the assemble job then requires an
EXE, an MSIX, an AppImage and a tarball to be present before anything is
published — a packaging step that quietly produced nothing used to make a green
job and a release short one platform. A DMG is copied into the upload set when
the macOS job produced one; it is not required. Assembling runs on **every**
release run, not only on a tag, so the download and the completeness check are
exercised by **Run workflow** rather than first attempted during a real release.

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
`test` job that fronts every packaging job), and the Flatpak smoke test asserts
that the *installed* app's name is `Aoide`. Validation is `--no-net`, so a moved
screenshot host cannot fail a build over an input that is not in the repo.

`StartupWMClass=Aoide`, not the app ID: Qt builds X11 `WM_CLASS` from argv[0]'s
basename and `applicationName`, giving `"aoide", "Aoide"`, so the app ID matched
neither field and the key did nothing there. Wayland does not need it — `app_id`
already equals the desktop file's basename, which is what xdg-shell asks for and
what GNOME and KWin match on.

### Screenshots

Still to do, and the one step that cannot be done from the repo. Flathub
requires screenshots and AppStream `<image>` must be a URL a store can fetch, so
they cannot ride inside the bundle. The pictures are generated, not committed:

```bash
QT_QPA_PLATFORM=offscreen ./build/aoide --dump-chrome /tmp/shots
```

Take `main_player_window.png`, `playlist_window.png` and
`equalizer_window.png`, upload them under
`https://aoide.music/screenshots/<version>/`, then uncomment the `<screenshots>`
block in the metainfo file. `check-metainfo.sh --check-urls` — which the release
`test` job runs — fetches every declared `<image>` and fails on anything that is
not a 200. While the block stays commented it reports that none are declared, so
the guard is in place before it is needed rather than added after the first
broken listing.

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
`org.kde.Platform` 6.10 ships it, and `look.cpp` already declines a zip it
cannot unpack instead of failing.

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
`./build.sh` compiles against. It is **6.10.3**. `QT_RUNTIME` is its
major.minor (`6.10`), which is what
[`packaging/flatpak/com.proximamagnifica.aoide.yml`](../packaging/flatpak/com.proximamagnifica.aoide.yml)
and `org.kde.Platform` use. KDE's 6.8 runtime is end-of-life; 6.10 is the
supported line that still matches a current official `6.10.x` kit (the
runtime currently ships 6.10.3). Workflows read the file via
[`tool/export-qt-pin.sh`](../tool/export-qt-pin.sh).

The Flatpak manifest is the one file that must repeat the pin as a literal,
because it is also what a human PRs to Flathub. `make_flatpak.sh` therefore
compares its `runtime-version` against `QT_VERSION` and refuses to build on a
mismatch, so a Qt bump surfaces there instead of as a `flatpak-builder` error
about a runtime nobody installed.

Linux installs that Qt through `install-qt-action` / `./tool/fetch_qt.sh`
rather than apt or Homebrew: the runner's `qt6-base-dev` is 6.4.2, and
Homebrew's `qtbase` moves ahead of the pin. The install is the official
desktop base sliced to `qtbase`, `qtwayland`, and `icu`. `qtwayland` is a
base *archive* (the client QPA plugin the AppImage stages); it is not an aqt
module. The module of that name does not exist on 6.10 — `qtwaylandcompositor`
does, and that is a compositor SDK, not the plugin. `icu` is the bundled ICU
the official Linux `qtbase` links against.

`./tool/fetch_qt.sh` is **Linux only** — it exits 1 on Darwin. The macOS CI
and release jobs install the same pin with `jurplel/install-qt-action` (`host: mac`,
`target: desktop`, `arch: clang_64`, archives `qtbase qttools`). `qttools` is
there for `macdeployqt`; on Windows that tool lives in `qtbase`, which is why
the Windows job can slice further. A local Mac host places the official desktop
kit under `.local/qt/<QT_VERSION>/macos` or points `CMAKE_PREFIX_PATH` at it.

[`build.sh`](../build.sh) fetches the pin into `.local/qt/` if it is missing
and refuses to link any other version. CMake does the same check.

Cut a release by bumping [`VERSION`](../VERSION), committing, then:

```bash
git tag v1.0
git push origin v1.0
```

The tag name without `v` must equal the `VERSION` file.

## Artifacts

| File | Channel |
|------|---------|
| `Aoide-<ver>-windows-x64.exe` | Official download (unsigned Inno; SmartScreen click-through) |
| `Aoide-<ver>-windows-x64.msix` | Microsoft Store listing **aoide.music** (unsigned here; Store re-signs). Identity version is four-part `x.y.z.0` from `VERSION` (`1.0` → `1.0.0.0`); the fourth number must be **0** or Partner Center rejects the package. Bump `VERSION` for each Store upload. |
| `Aoide-<ver>-linux-x86_64.AppImage` | Official download |
| `Aoide-<ver>-linux-x86_64.tar.gz` | Input for a Flathub recipe |
| `Aoide-<ver>-linux-x86_64.flatpak` | Optional CI bundle (job may fail without blocking the rest) |
| `Aoide-<ver>-macos-universal.dmg` | Official download in **1.1**. Every CI run now produces one; one has been installed on a MacBook and played audio. The smoke proves the staged bundle starts offscreen — not that a listener can open the DMG past Gatekeeper. |

Partner Center and Flathub submit stay **human**. Packaging scripts live under `packaging/`.

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
| Ruleset **main** | Active, admin bypass | Blocks deletion and force-push, and requires a PR whose `Qt (ubuntu-24.04)`, `Qt (windows-latest)` and `CI passed` checks are green. Squash only, matching `merge-if-green` |

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

The MSIX **display name** is `aoide.music` (the reserved Store listing). Paste Publisher and Identity Name from Partner Center into those variables as soon as the app exists there; a mismatch fails certification. The website EXE and in-app chrome stay **Aoide**.

### Secrets (macOS notarization)

All five signing and notary secrets are **set** on the repository as of
2026-08-28, from a Developer ID Application certificate on the G2 chain valid to
2031 ([`premises.md`](premises.md) §7). A release run has now signed and notarized
a DMG rather than skipping, and one of those images has been installed on a
MacBook and played audio. The release job stays `continue-on-error` so a
packaging failure cannot fail a Windows/Linux release, not because the lane is
unrun. In CI the same packaging steps are `continue-on-error` so Apple's notary
cannot block a merge. Read the log; a green tick on those steps is not proof
the image is what a listener would get past Gatekeeper.

Skip any of these and the Mac job still uploads an unsigned DMG;
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

Windows (on a Windows host): `tool/fetch_full_libmpv.ps1`, CMake Release build, then `packaging/windows/stage.ps1`, Inno (`packaging/windows/aoide.iss`) and `packaging/windows/make_msix.ps1`. The EXE installer runs `vc_redist.x64.exe` when `MSVCP140.dll` / `VCRUNTIME140.dll` are missing. The MSIX declares `Microsoft.VCLibs.140.00.UWPDesktop` so the Store supplies that runtime. Keep the `.ps1` files ASCII: Windows PowerShell 5.1 (what `powershell` is on the runner) reads UTF-8 source as ANSI, and an em-dash inside a string is decoded as a closing quote.

macOS (on a Mac — CI builds, smokes the staged bundle, and wraps a DMG; one
image has been installed and played):

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
