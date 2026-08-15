#Requires -Version 5.1
# Microsoft Visual C++ 2015-2022 x64 redistributable (for the website EXE).
# Not committed; Inno compiles it in from packaging/windows/.
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Out = Join-Path $Root "packaging\windows\vc_redist.x64.exe"
$Url = "https://aka.ms/vs/17/release/vc_redist.x64.exe"
New-Item -ItemType Directory -Force -Path (Split-Path $Out) | Out-Null
Write-Host "Fetching $Url"
Invoke-WebRequest -Uri $Url -OutFile $Out -UseBasicParsing
if ((Get-Item $Out).Length -lt 1MB) {
  throw "vc_redist.x64.exe looks too small ($((Get-Item $Out).Length) bytes)"
}
Write-Host "Wrote $Out"
