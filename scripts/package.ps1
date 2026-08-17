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
if (-not $stage.StartsWith($release + '\')) {
    throw "Stage path out of scope: $stage"
}
if (Test-Path $stage) {
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

# Bundle user docs (txt/csv kept at release root as packaging assets).
$assets = Get-ChildItem -LiteralPath $release -File | Where-Object {
    $_.Extension -in @('.txt', '.csv')
}
if ($assets.Count -lt 2) {
    throw 'Packaging assets missing: keep a .txt usage note and a .csv price sample under release/'
}
$assets | Copy-Item -Destination $stage -Force

# Build artifacts.
Remove-Item -LiteralPath "$release\SteamMarketTerminal-portable.zip", "$release\SteamMarketTerminal-Setup.exe" `
    -Force -ErrorAction SilentlyContinue
Push-Location $release
try {
    & $SevenZip a -tzip -y 'SteamMarketTerminal-portable.zip' '.\SteamMarketTerminal' | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'zip failed' }
    $sfxModule = Join-Path (Split-Path $SevenZip -Parent) '7z.sfx'
    & $SevenZip a -t7z -y "-sfx$sfxModule" 'SteamMarketTerminal-Setup.exe' '.\SteamMarketTerminal' | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'sfx failed' }
} finally {
    Pop-Location
}

Get-ChildItem $release -Filter 'SteamMarketTerminal-*' | Select-Object Name, Length, LastWriteTime
Write-Host 'Packaging done: portable ZIP and SFX setup generated under release/'
