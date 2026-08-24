# Copy tramp.exe, Qt runtime, assets, skins, and optional libmpv into build/windows/stage.
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Candidates = @(
  (Join-Path $Root "build\Release\tramp.exe"),
  (Join-Path $Root "build\tramp.exe")
)
$Exe = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Exe) {
  throw "stage: missing tramp.exe under build/; cmake --build build first"
}
$Stage = Join-Path $Root "build\windows\stage"
if (Test-Path $Stage) { Remove-Item $Stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Stage | Out-Null
Copy-Item -Force $Exe (Join-Path $Stage "tramp.exe")
Copy-Item -Recurse -Force (Join-Path $Root "assets") (Join-Path $Stage "assets")
Copy-Item -Recurse -Force (Join-Path $Root "skins") (Join-Path $Stage "skins")
Copy-Item -Force (Join-Path $Root "LICENSE") $Stage
Copy-Item -Force (Join-Path $Root "THIRD-PARTY-NOTICES.md") $Stage

$Mpv = Join-Path $Root "third_party\libmpv\windows\x86_64\libmpv-2.dll"
if (Test-Path $Mpv) {
  Copy-Item -Force $Mpv $Stage
}

$Windeploy = Get-Command windeployqt -ErrorAction SilentlyContinue
if (-not $Windeploy) {
  $QtPrefix = $env:IQTA_TOOLS
  $Guess = @(
    $env:QTDIR,
    (Join-Path ${env:ProgramFiles} "Qt")
  ) | Where-Object { $_ }
  foreach ($g in $Guess) {
    $found = Get-ChildItem -Path $g -Filter windeployqt.exe -Recurse -ErrorAction SilentlyContinue |
      Select-Object -First 1
    if ($found) { $Windeploy = $found; break }
  }
}
if ($Windeploy) {
  $deployBin = if ($Windeploy.PSObject.Properties.Name -contains 'Source' -and $Windeploy.Source) {
    $Windeploy.Source
  } else {
    $Windeploy.FullName
  }
  & $deployBin (Join-Path $Stage "tramp.exe") --release --no-translations
} else {
  throw "stage: windeployqt not found; refusing to ship tramp.exe without Qt DLLs"
}

Write-Host "Staged $Stage"
