param(
    [switch]$SkipBuild,
    [string]$Qmake = 'D:\Qt\6.11.0\mingw_64\bin\qmake.exe',
    [string]$Make = 'D:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe',
    [string]$Windeployqt = 'D:\Qt\6.11.0\mingw_64\bin\windeployqt.exe',
    [string]$SevenZip = 'C:\Program Files\7-Zip\7z.exe'
)

# Release packaging: build -> windeployqt self-contained dir -> portable ZIP + SFX installer.
# NOTE: keep this file ASCII-only; Windows PowerShell 5.1 parses BOM-less UTF-8 as ANSI.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$release = "$root\release"
$stage = "$release\SteamMarketTerminal"
$assetsDir = "$root\packaging"

if (-not (Test-Path -LiteralPath $release -PathType Container)) {
    New-Item -ItemType Directory -Path $release | Out-Null
}

if (-not $SkipBuild) {
    Push-Location "$root\src"
    try {
        & $Qmake src.pro
        if ($LASTEXITCODE -ne 0) { throw 'qmake(src) failed' }
        & $Make -j4
        if ($LASTEXITCODE -ne 0) { throw 'make(src) failed' }
    } finally {
        Pop-Location
    }
}

# Recreate a clean staging dir (guard against path escapes).
if ([System.IO.Path]::GetFullPath($stage) -ne
    [System.IO.Path]::GetFullPath((Join-Path $release 'SteamMarketTerminal'))) {
    throw "Stage path out of scope: $stage"
}
if (Test-Path -LiteralPath $stage) {
    $stageItem = Get-Item -LiteralPath $stage -Force
    if (($stageItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing to replace reparse-point stage: $stage"
    }
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Path $stage | Out-Null

Copy-Item -LiteralPath "$root\bin\SteamMarketTerminal.exe" -Destination $stage
$webViewLoader = "$root\bin\WebView2Loader.dll"
if (-not (Test-Path -LiteralPath $webViewLoader)) {
    throw 'WebView2Loader.dll missing; build the application before packaging'
}
Copy-Item -LiteralPath $webViewLoader -Destination $stage
& $Windeployqt --release --no-translations --dir $stage "$stage\SteamMarketTerminal.exe"
if ($LASTEXITCODE -ne 0) { throw 'windeployqt failed' }

# qt.conf: let Qt find plugins next to the exe on machines without Qt installed.
@(
    '[Paths]',
    'Prefix=.',
    'Plugins=.'
) | Set-Content -LiteralPath "$stage\qt.conf" -Encoding ASCII

# Keep only the SQLite driver.
Get-ChildItem "$stage\sqldrivers" -Filter '*.dll' |
    Where-Object { $_.Name -ne 'qsqlite.dll' } |
    Remove-Item -Force

# Bundle user docs from a source-controlled directory.
$assets = Get-ChildItem -LiteralPath $assetsDir -File | Where-Object {
    $_.Extension -in @('.txt', '.csv')
}
if ($assets.Count -lt 2) {
    throw 'Packaging assets missing: keep a .txt usage note and a .csv sample under packaging/'
}
$assets | Copy-Item -Destination $stage -Force

# A release is valid only if it runs without Qt or MinGW on PATH and from another cwd.
$runtimeSmoke = Join-Path $release 'runtime-verification.png'
& "$root\scripts\verify_runtime.ps1" -AppDir $stage -SmokePath $runtimeSmoke
if ($LASTEXITCODE -ne 0) { throw 'runtime verification failed' }

# Build artifacts.
Remove-Item -LiteralPath "$release\SteamMarketTerminal-portable.zip", "$release\SteamMarketTerminal-Setup.exe" `
    -Force -ErrorAction SilentlyContinue
if (Test-Path -LiteralPath $SevenZip -PathType Leaf) {
    Push-Location $release
    try {
        & $SevenZip a -tzip -y 'SteamMarketTerminal-portable.zip' '.\SteamMarketTerminal' | Out-Null
        if ($LASTEXITCODE -ne 0) { throw 'zip failed' }
        $sfxModule = Join-Path (Split-Path $SevenZip -Parent) '7z.sfx'
        if (Test-Path -LiteralPath $sfxModule -PathType Leaf) {
            & $SevenZip a -t7z -y "-sfx$sfxModule" 'SteamMarketTerminal-Setup.exe' '.\SteamMarketTerminal' | Out-Null
            if ($LASTEXITCODE -ne 0) { throw 'sfx failed' }
        }
    } finally {
        Pop-Location
    }
} else {
    Compress-Archive -LiteralPath $stage -DestinationPath "$release\SteamMarketTerminal-portable.zip"
}

Get-ChildItem $release -Filter 'SteamMarketTerminal-*' | Select-Object Name, Length, LastWriteTime
Write-Host 'Packaging done: portable ZIP and SFX setup generated under release/'
