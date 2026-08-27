# Copy aoide.exe, Qt runtime, assets, skins, and optional libmpv into build/windows/stage.
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Candidates = @(
  (Join-Path $Root "build\Release\aoide.exe"),
  (Join-Path $Root "build\aoide.exe")
)
$Exe = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Exe) {
  throw "stage: missing aoide.exe under build/; cmake --build build first"
}
$Stage = Join-Path $Root "build\windows\stage"
if (Test-Path $Stage) { Remove-Item $Stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Stage | Out-Null
Copy-Item -Force $Exe (Join-Path $Stage "aoide.exe")
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
  # qoffscreen is not a dependency windeployqt can see: it is what the release
  # smoke test runs the staged tree under, and that test clears QT_PLUGIN_PATH so
  # a stage missing its plugins cannot borrow the runner's. Same reason
  # stage_bundle.sh takes the whole platforms group on Linux.
  & $deployBin (Join-Path $Stage "aoide.exe") --release --no-translations `
    --include-plugins qoffscreen
} else {
  throw "stage: windeployqt not found; refusing to ship aoide.exe without Qt DLLs"
}

# Assert rather than trust the flag above: a stage with no platform plugin starts
# nowhere, and windeployqt reports success either way.
foreach ($plugin in "qwindows.dll", "qoffscreen.dll") {
  $path = Join-Path $Stage "platforms\$plugin"
  if (-not (Test-Path $path)) {
    throw "stage: windeployqt did not deploy platforms\$plugin"
  }
}

Write-Host "Staged $Stage"
