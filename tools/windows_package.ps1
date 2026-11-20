# Package the complete native Windows VST3 folder as a real installer.
# Unsigned builds are intentionally labeled TEST; signed release uses CANDIDATE
# and only renames after Authenticode verification.
$ErrorActionPreference = 'Stop'
$version = $args[0]
$label = if ($args.Count -gt 1) { $args[1] } else { 'UNSIGNED-TEST' }
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'Expected package version X.Y.Z' }
if ($label -notin @('UNSIGNED-TEST', 'CANDIDATE')) { throw 'Unknown package label' }
$bundle = 'build/GuitarBiosphere_artefacts/Release/VST3/Guitar Biosphere.vst3'
if (!(Test-Path -LiteralPath $bundle -PathType Container)) { throw 'Compiled VST3 bundle missing' }
$native = @(Get-ChildItem -LiteralPath $bundle -File -Recurse -Filter '*.vst3')
if ($native.Count -ne 1) { throw 'VST3 bundle must have exactly one native executable' }
$commands = @(Get-Command ISCC.exe -ErrorAction SilentlyContinue)
$compiler = if ($commands.Count -gt 0) { $commands[0].Source } else { $null }
if (!$compiler) {
    $compiler = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Inno Setup 6/ISCC.exe'
}
if (!(Test-Path -LiteralPath $compiler -PathType Leaf)) {
    throw 'Inno Setup 6 not installed. Install it in the CI runner before packaging.'
}
$out = Join-Path (Get-Location).Path 'release-output'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$env:GUITAR_BIOSPHERE_VERSION = $version
$env:GUITAR_BIOSPHERE_VST3_SOURCE = (Resolve-Path -LiteralPath $bundle).Path
$env:GUITAR_BIOSPHERE_OUTPUT = $out
$env:GUITAR_BIOSPHERE_PACKAGE_LABEL = $label
& $compiler 'installers/windows/GuitarBiosphere.iss'
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup installer compilation failed' }
$setup = Join-Path $out "Guitar-Biosphere-v$version-Windows-x64-$label.exe"
if (!(Test-Path -LiteralPath $setup -PathType Leaf)) { throw 'Windows EXE installer missing' }
if ((Get-Item -LiteralPath $setup).Length -lt 100000) { throw 'Installer unexpectedly small' }
$hash = (Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash
Write-Host "PASS: Windows VST3 EXE installer, $label, SHA256=$hash"
