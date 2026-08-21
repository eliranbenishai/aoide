#Requires -Version 5.1
<#
.SYNOPSIS
  Download and install the pinned full Windows libmpv build into third_party/.
#>
$ErrorActionPreference = 'Stop'

$Root = Resolve-Path (Join-Path $PSScriptRoot '..')
$PinsPath = Join-Path $Root 'third_party\libmpv\pins.json'
$Pins = Get-Content -Raw $PinsPath | ConvertFrom-Json
$Win = $Pins.windows.x86_64

$CacheDir = Join-Path $Root 'third_party\libmpv\.cache'
$OutDir = Join-Path $Root 'third_party\libmpv\windows\x86_64'
$ArchivePath = Join-Path $CacheDir $Win.archive
$ExtractDir = Join-Path $CacheDir 'extract-win-x64'
$OutDll = Join-Path $OutDir $Win.dll
# The archive's import library carries a GNU-style name but is a Microsoft
# short-import library. Without it on disk a Windows configure cannot link
# libmpv, and the build quietly loses its audio engine.
$ImportLib = 'libmpv.dll.a'
$OutImportLib = Join-Path $OutDir $ImportLib

New-Item -ItemType Directory -Force -Path $CacheDir, $OutDir | Out-Null

function Get-Sha256([string]$Path) {
  return (Get-FileHash -Algorithm SHA256 -Path $Path).Hash.ToUpperInvariant()
}

function Find-CMake {
  $fromPath = Get-Command cmake -ErrorAction SilentlyContinue
  if ($fromPath) { return $fromPath.Source }
  $vs = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
  if (Test-Path $vs) { return $vs }
  throw 'cmake not found. Install Visual Studio CMake tools or add cmake to PATH.'
}

if (-not (Test-Path $ArchivePath) -or (Get-Sha256 $ArchivePath) -ne $Win.archiveSha256.ToUpperInvariant()) {
  Write-Host "Downloading $($Win.url)"
  $ProgressPreference = 'SilentlyContinue'
  Invoke-WebRequest -Uri $Win.url -OutFile $ArchivePath
}

$archiveHash = Get-Sha256 $ArchivePath
if ($archiveHash -ne $Win.archiveSha256.ToUpperInvariant()) {
  throw "Archive SHA-256 mismatch: got $archiveHash expected $($Win.archiveSha256)"
}
Write-Host "Archive OK ($archiveHash)"

if (Test-Path $ExtractDir) { Remove-Item $ExtractDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $ExtractDir | Out-Null
$cmake = Find-CMake
Push-Location $ExtractDir
try {
  & $cmake -E tar xf $ArchivePath
  if ($LASTEXITCODE -ne 0) { throw "cmake -E tar failed ($LASTEXITCODE)" }
} finally {
  Pop-Location
}

$srcDll = Join-Path $ExtractDir $Win.dll
if (-not (Test-Path $srcDll)) {
  throw "Extracted archive missing $($Win.dll)"
}
$srcImportLib = Join-Path $ExtractDir $ImportLib
if (-not (Test-Path $srcImportLib)) {
  throw "Extracted archive missing $ImportLib"
}

Copy-Item -Force $srcDll $OutDll
Copy-Item -Force $srcImportLib $OutImportLib
$dllHash = Get-Sha256 $OutDll
if ($dllHash -ne $Win.dllSha256.ToUpperInvariant()) {
  throw "DLL SHA-256 mismatch: got $dllHash expected $($Win.dllSha256)"
}

$bytes = [System.IO.File]::ReadAllBytes($OutDll)
$text = [System.Text.Encoding]::ASCII.GetString($bytes)
if ($text.Contains('--disable-filters')) {
  throw 'Downloaded DLL looks slim (--disable-filters present). Refusing to install.'
}
if (-not ($text.Contains('aresample') -and $text.Contains('equalizer'))) {
  throw 'Downloaded DLL missing aresample/equalizer markers.'
}

$sizeMb = [math]::Round((Get-Item $OutDll).Length / 1MB, 1)
Write-Host "Installed full libmpv -> $OutDll ($sizeMb MiB, sha256=$dllHash)"
# No separate hash for the import library: the archive SHA-256 above already
# gates every file that came out of it.
Write-Host "Installed import library -> $OutImportLib"
