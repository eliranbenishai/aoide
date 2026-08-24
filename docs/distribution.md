# Distribution and CI

How Tramp is built and handed to listeners. Product page: `https://tramp.music`. GitHub Releases on a version tag are a **mirror**, not the product surface.

## Workflows

| Workflow | When | What |
|----------|------|------|
| [`.github/workflows/open-pr.yml`](../.github/workflows/open-pr.yml) | Push to a feature branch | Opens a PR against `main` if one is missing (`research/*`, `spike/*`, `wip/*` skipped) |
| [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) | PR and `main` | CMake build + `ctest` on Ubuntu and Windows; PR review comment with the result |
| [`.github/workflows/merge-if-green.yml`](../.github/workflows/merge-if-green.yml) | CI completed | Squash-merges a same-repo, non-draft PR at that SHA when CI is green. Skips forks, drafts, and `do-not-merge` |
| [`.github/workflows/release.yml`](../.github/workflows/release.yml) | Tag `v*` or **Run workflow** | Test, then Windows / Linux packages; tags also attach a GitHub Release |

Every packaging job **runs the thing it packaged** before it is published: the
staged binary, the AppImage, the extracted tarball and the installed `.flatpak`
each get `--bench-chrome` (links, finds its assets and fonts, is optimised) and
a `TRAMP_AUTO_QUIT=1` session start. `ctest` runs against the build tree, which
still resolves Qt and libmpv from the runner — only these runs prove the
artifact carries its own, so they clear the runner's Qt out of the environment
first. Deleting one library from the staging directory fails the job. The
Flatpak run proves the opposite property, that it can get Qt from its runtime,
and the job also asserts no `libQt6*` reached its staging directory.

## Qt version

The [`QT_VERSION`](../QT_VERSION) file is the authority: one official desktop
kit, built and tested against everywhere something ships, and the same kit
`./build.sh` compiles against. It is **6.10.3**. `QT_RUNTIME` is its
major.minor (`6.10`), which is what
[`packaging/flatpak/com.proximamagnifica.tramp.yml`](../packaging/flatpak/com.proximamagnifica.tramp.yml)
and `org.kde.Platform` use. KDE's 6.8 runtime is end-of-life; 6.10 is the
supported line that still matches a current official `6.10.x` kit (the
runtime currently ships 6.10.3). Workflows read the file via
[`tool/export-qt-pin.sh`](../tool/export-qt-pin.sh).

Linux installs that Qt through `install-qt-action` / `./tool/fetch_qt.sh`
rather than apt or Homebrew: the runner's `qt6-base-dev` is 6.4.2, and
Homebrew's `qtbase` moves ahead of the pin. The install is the official
desktop base sliced to `qtbase`, `qtwayland`, and `icu`. `qtwayland` is a
base *archive* (the client QPA plugin the AppImage stages); it is not an aqt
module. The module of that name does not exist on 6.10 — `qtwaylandcompositor`
does, and that is a compositor SDK, not the plugin. `icu` is the bundled ICU
the official Linux `qtbase` links against.

[`build.sh`](../build.sh) fetches the pin into `.local/qt/` if it is missing
and refuses to link any other version. CMake does the same check.

Cut a release by bumping [`VERSION`](../VERSION), committing, then:

```bash
git tag v0.1.0
git push origin v0.1.0
```

The tag name without `v` must equal the `VERSION` file.

## Artifacts

| File | Channel |
|------|---------|
| `Tramp-<ver>-windows-x64.exe` | Official download (unsigned Inno; SmartScreen click-through) |
| `Tramp-<ver>-windows-x64.msix` | Microsoft Store listing **tramp.music** (unsigned here; Store re-signs). Identity version is `x.y.z.0` from `VERSION` `x.y.z`; the fourth number must be **0** or Partner Center rejects the package. Bump `x.y.z` for each Store upload. |
| `Tramp-<ver>-linux-x86_64.AppImage` | Official download |
| `Tramp-<ver>-linux-x86_64.tar.gz` | Input for a Flathub recipe |
| `Tramp-<ver>-linux-x86_64.flatpak` | Optional CI bundle (job may fail without blocking the rest) |
| `Tramp-<ver>-macos-universal.dmg` | Official download once the Qt Mac host exists (notarized when secrets are set) |

Partner Center and Flathub submit stay **human**. Packaging scripts live under `packaging/`.

## GitHub configuration

Actions must be allowed to open PRs: **Settings → Actions → General → Workflow permissions → Allow GitHub Actions to create and approve pull requests**. Without that, you still open PRs by hand; CI and merge-if-green still apply.

Label a PR `do-not-merge` (or convert it to draft) to keep it open after a green CI.

### Variables (optional)

| Variable | Purpose |
|----------|---------|
| `MSIX_PUBLISHER` | Store identity `CN=...` from Partner Center **Identity details**. Default `CN=Proxima Magnifica`. |
| `MSIX_IDENTITY_NAME` | Package identity name from those same details. Default `ProximaMagnifica.trampmusic` until Partner Center shows the real one. |

The MSIX **display name** is `tramp.music` (the reserved Store listing). Paste Publisher and Identity Name from Partner Center into those variables as soon as the app exists there; a mismatch fails certification. The website EXE and in-app chrome stay **Tramp**.

### Secrets (macOS notarization)

Skip any of these and the Mac job still uploads a DMG; it will not be notarized. The Mac job itself waits on the Qt Mac host.

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
the GL driver.

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

Windows (on a Windows host): `tool/fetch_full_libmpv.ps1`, CMake Release build, then `packaging/windows/stage.ps1`, Inno (`packaging/windows/tramp.iss`) and `packaging/windows/make_msix.ps1`. The EXE installer runs `vc_redist.x64.exe` when `MSVCP140.dll` / `VCRUNTIME140.dll` are missing. The MSIX declares `Microsoft.VCLibs.140.00.UWPDesktop` so the Store supplies that runtime. Keep the `.ps1` files ASCII: Windows PowerShell 5.1 (what `powershell` is on the runner) reads UTF-8 source as ANSI, and an em-dash inside a string is decoded as a closing quote.
