# Fail closed: Windows Authenticode-signed VST3 ZIP only.
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($env:WINDOWS_SIGNING_PFX_BASE64) -or
    [string]::IsNullOrWhiteSpace($env:WINDOWS_SIGNING_PFX_PASSWORD)) {
    throw 'Windows signing certificate secrets missing: release NOT published'
}
$version = $args[0]
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'Expected X.Y.Z version' }
$root = 'build/GuitarBiosphere_artefacts/Release/VST3'
$bundle = Join-Path $root 'Guitar Biosphere.vst3'
if (!(Test-Path -LiteralPath $bundle -PathType Container)) { throw 'VST3 bundle missing' }
$binary = @(Get-ChildItem -LiteralPath $bundle -Filter '*.vst3' -File -Recurse)
if ($binary.Count -ne 1) { throw "Expected one VST3 binary, got $($binary.Count)" }
$kits = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Windows Kits/10/bin'
$tools = @(Get-ChildItem -LiteralPath $kits -Filter 'signtool.exe' -File -Recurse | Where-Object { $_.FullName -match '[\\/]x64[\\/]signtool.exe$' } | Sort-Object FullName -Descending)
if ($tools.Count -eq 0) { throw 'Microsoft SDK signtool missing' }
$signTool = $tools[0].FullName
$pfx = Join-Path $env:RUNNER_TEMP 'guitar-biosphere-signing.pfx'
try {
    [System.IO.File]::WriteAllBytes($pfx, [Convert]::FromBase64String($env:WINDOWS_SIGNING_PFX_BASE64))
    & $signTool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /f $pfx /p $env:WINDOWS_SIGNING_PFX_PASSWORD $binary[0].FullName
    if ($LASTEXITCODE -ne 0) { throw 'Authenticode signing failed' }
    & $signTool verify /pa /v $binary[0].FullName
    if ($LASTEXITCODE -ne 0) { throw 'Authenticode verification failed' }
    New-Item -ItemType Directory -Force -Path 'release-output' | Out-Null
    $archive = "release-output/Guitar-Biosphere-v$version-Windows-x64-SIGNED-VST3.zip"
    Compress-Archive -LiteralPath $bundle -DestinationPath $archive -CompressionLevel Optimal -Force
    if ((Get-Item -LiteralPath $archive).Length -lt 100000) { throw 'Signed ZIP unexpectedly small' }
    $hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
    "Windows signed VST3 SHA256: $hash" | Out-File -FilePath 'release-output/SHA256.txt'
    Write-Host 'PASS: Authenticode-signed and verified Windows VST3'
} finally {
    Remove-Item -LiteralPath $pfx -Force -ErrorAction SilentlyContinue
}
