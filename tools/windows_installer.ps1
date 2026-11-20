# Compile an Inno Setup VST3 installer. -Signed requires a signed VST3 and PFX.
# Without -Signed, the setup EXE is for internal testing ONLY.
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string] $Version,
    [switch] $Signed
)
$ErrorActionPreference = 'Stop'
$root = (Get-Location).Path
$sourceDir = Join-Path $root 'build\GuitarBiosphere_artefacts\Release\VST3'
$bundle = Join-Path $sourceDir 'Guitar Biosphere.vst3'
if (!(Test-Path -LiteralPath $bundle -PathType Container)) { throw 'VST3 bundle missing' }
$pluginBinary = @(Get-ChildItem -LiteralPath $bundle -Filter '*.vst3' -File -Recurse)
if ($pluginBinary.Count -ne 1) { throw "Expected one VST3 binary, got $($pluginBinary.Count)" }
$iscc = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Inno Setup 6\ISCC.exe'
if (!(Test-Path -LiteralPath $iscc -PathType Leaf)) { throw 'Inno Setup 6 ISCC.exe not installed' }
$iss = Join-Path $root 'tools\GuitarBiosphere.iss'
$out = Join-Path $root 'release-output'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$suffix = if ($Signed) { 'SIGNED' } else { 'UNSIGNED-TEST' }
if ($Signed) {
    if ([string]::IsNullOrWhiteSpace($env:WINDOWS_SIGNING_PFX_BASE64) -or
        [string]::IsNullOrWhiteSpace($env:WINDOWS_SIGNING_PFX_PASSWORD)) {
        throw 'Windows signing secrets missing: no signed installer'
    }
    $kits = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Windows Kits\10\bin'
    $signTools = @(Get-ChildItem -LiteralPath $kits -Filter 'signtool.exe' -File -Recurse |
        Where-Object { $_.FullName -match '[\\/]x64[\\/]signtool.exe$' } |
        Sort-Object FullName -Descending)
    if ($signTools.Count -eq 0) { throw 'Microsoft SDK SignTool missing' }
    $signTool = $signTools[0].FullName
    & $signTool verify /pa /v $pluginBinary[0].FullName
    if ($LASTEXITCODE -ne 0) { throw 'VST3 is not authenticode verified' }
}
& $iscc "/DMyAppVersion=$Version" "/DMySourceDir=$sourceDir" "/DMyOutputDir=$out" "/DMyFileSuffix=$suffix" $iss
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed' }
$setup = Join-Path $out "Guitar-Biosphere-v$Version-Windows-x64-$suffix-Setup.exe"
if (!(Test-Path -LiteralPath $setup -PathType Leaf)) { throw 'Installer EXE missing' }
if ((Get-Item -LiteralPath $setup).Length -lt 100000) { throw 'Installer EXE unexpectedly small' }
if ($Signed) {
    $pfx = Join-Path $env:RUNNER_TEMP 'guitar-biosphere-installer-sign.pfx'
    try {
        [System.IO.File]::WriteAllBytes($pfx, [Convert]::FromBase64String($env:WINDOWS_SIGNING_PFX_BASE64))
        & $signTool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /f $pfx /p $env:WINDOWS_SIGNING_PFX_PASSWORD $setup
        if ($LASTEXITCODE -ne 0) { throw 'Setup.exe signing failed' }
        & $signTool verify /pa /v $setup
        if ($LASTEXITCODE -ne 0) { throw 'Setup.exe signature verification failed' }
    } finally {
        Remove-Item -LiteralPath $pfx -Force -ErrorAction SilentlyContinue
    }
}
$hash = (Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash
"Windows Setup SHA256: $hash" | Set-Content -Path (Join-Path $out 'Windows-Setup-SHA256.txt')
Write-Host "Installer: $setup"
if ($Signed) { Write-Host 'Authenticode signed EXE verified' }
else { Write-Host 'UNSIGNED TEST INSTALLER: not approved for customers' }
