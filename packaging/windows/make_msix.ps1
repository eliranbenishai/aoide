# Packs the Windows Release folder into an unsigned MSIX for Store submission.
# Microsoft re-signs Store MSIX after certification (ADR 0011).
param(
  [string]$Version = "0.1.0.0",
  [string]$Publisher = "CN=Proxima Magnifica",
  [string]$IdentityName = "ProximaMagnifica.trampmusic"
)

$ErrorActionPreference = "Stop"
if ($Version -notmatch '^\d+\.\d+\.\d+\.0$') {
  throw "Store MSIX version must be Major.Minor.Build.0 (got '$Version'). The Store reserves the revision digit."
}
$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Release = Join-Path $Root "build\windows\stage"
$Stage = Join-Path $Root "build\windows\msix_stage"
$OutDir = Join-Path $Root "build\windows\msix"
$Png = Join-Path $Root "packaging\linux\icons\hicolor\256x256\apps\com.tramp.tramp.png"

if (-not (Test-Path (Join-Path $Release "tramp.exe"))) {
  throw "Missing $Release\tramp.exe — run packaging/windows/stage.ps1 first"
}

$makeappx = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\makeappx.exe" -ErrorAction SilentlyContinue |
  Sort-Object FullName -Descending |
  Select-Object -First 1
if (-not $makeappx) {
  throw "makeappx.exe not found (Windows 10 SDK)"
}

if (Test-Path $Stage) { Remove-Item $Stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Stage, $OutDir | Out-Null
Copy-Item -Recurse -Force "$Release\*" $Stage
Copy-Item -Force (Join-Path $Root "LICENSE") $Stage
Copy-Item -Force (Join-Path $Root "THIRD-PARTY-NOTICES.md") $Stage

$assets = Join-Path $Stage "Assets"
New-Item -ItemType Directory -Force -Path $assets | Out-Null
if (Test-Path $Png) {
  Copy-Item $Png (Join-Path $assets "StoreLogo.png")
  Copy-Item $Png (Join-Path $assets "Square150x150Logo.png")
  Copy-Item $Png (Join-Path $assets "Square44x44Logo.png")
}

$manifest = @"
<?xml version="1.0" encoding="utf-8"?>
<Package
  xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
  xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10"
  xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities"
  IgnorableNamespaces="uap rescap">
  <Identity Name="$IdentityName" Publisher="$Publisher" Version="$Version" ProcessorArchitecture="x64" />
  <Properties>
    <DisplayName>tramp.music</DisplayName>
    <PublisherDisplayName>Proxima Magnifica</PublisherDisplayName>
    <Logo>Assets\StoreLogo.png</Logo>
  </Properties>
  <Dependencies>
    <TargetDeviceFamily Name="Windows.Desktop" MinVersion="10.0.17763.0" MaxVersionTested="10.0.22621.0" />
    <PackageDependency Name="Microsoft.VCLibs.140.00.UWPDesktop" MinVersion="14.0.24217.0" Publisher="CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US" />
  </Dependencies>
  <Resources>
    <Resource Language="en-US" />
  </Resources>
  <Applications>
    <Application Id="Tramp" Executable="tramp.exe" EntryPoint="Windows.FullTrustApplication">
      <uap:VisualElements
        DisplayName="tramp.music"
        Description="Desktop music player"
        BackgroundColor="transparent"
        Square150x150Logo="Assets\Square150x150Logo.png"
        Square44x44Logo="Assets\Square44x44Logo.png">
        <uap:DefaultTile />
      </uap:VisualElements>
      <Extensions>
        <uap:Extension Category="windows.fileTypeAssociation">
          <uap:FileTypeAssociation Name="trampaudio">
            <uap:SupportedFileTypes>
              <uap:FileType>.mp3</uap:FileType>
              <uap:FileType>.m4a</uap:FileType>
              <uap:FileType>.aac</uap:FileType>
              <uap:FileType>.flac</uap:FileType>
              <uap:FileType>.wav</uap:FileType>
              <uap:FileType>.ogg</uap:FileType>
              <uap:FileType>.opus</uap:FileType>
            </uap:SupportedFileTypes>
          </uap:FileTypeAssociation>
        </uap:Extension>
        <uap:Extension Category="windows.fileTypeAssociation">
          <uap:FileTypeAssociation Name="trampplaylist">
            <uap:SupportedFileTypes>
              <uap:FileType>.m3u</uap:FileType>
              <uap:FileType>.m3u8</uap:FileType>
            </uap:SupportedFileTypes>
          </uap:FileTypeAssociation>
        </uap:Extension>
      </Extensions>
    </Application>
  </Applications>
  <Capabilities>
    <rescap:Capability Name="runFullTrust" />
  </Capabilities>
</Package>
"@
Set-Content -Path (Join-Path $Stage "AppxManifest.xml") -Value $manifest -Encoding ascii

$out = Join-Path $OutDir "Tramp-$Version-windows-x64.msix"

& $makeappx.FullName pack /d $Stage /p $out /o
if ($LASTEXITCODE -ne 0) { throw "makeappx failed ($LASTEXITCODE)" }
Write-Host "Wrote $out"
