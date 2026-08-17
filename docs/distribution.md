# Distribution and CI

How Tramp is built and handed to listeners. Decisions: [ADR 0010](adr/0010-open-source-website-download.md)–[0014](adr/0014-ci-and-architectures.md), [ADR 0016](adr/0016-qt-for-v1.md). Product page: `https://tramp.music`. GitHub Releases on a version tag are a **mirror**, not the product surface.

## Workflows

| Workflow | When | What |
|----------|------|------|
| [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) | PR and `main` | CMake build + `ctest` on Ubuntu and Windows |
| [`.github/workflows/release.yml`](../.github/workflows/release.yml) | Tag `v*` or **Run workflow** | Test, then Windows / Linux packages; tags also attach a GitHub Release |

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
# Linux
./tool/stage_linux_libmpv.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./packaging/linux/stage_bundle.sh
./packaging/linux/make_appimage.sh
```

Windows (on a Windows host): `tool/fetch_full_libmpv.ps1`, CMake Release build, then `packaging/windows/stage.ps1`, Inno (`packaging/windows/tramp.iss`) and `packaging/windows/make_msix.ps1`. The EXE installer runs `vc_redist.x64.exe` when `MSVCP140.dll` / `VCRUNTIME140.dll` are missing. The MSIX declares `Microsoft.VCLibs.140.00.UWPDesktop` so the Store supplies that runtime.
