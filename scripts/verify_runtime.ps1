param(
    [Parameter(Mandatory = $true)]
    [string]$AppDir,
    [Parameter(Mandatory = $true)]
    [string]$SmokePath
)

# Verify that a directory is a self-contained runtime, not merely compiler output.
$ErrorActionPreference = 'Stop'
$resolvedDir = (Resolve-Path -LiteralPath $AppDir).Path
$app = Join-Path $resolvedDir 'SteamMarketTerminal.exe'
if (-not (Test-Path -LiteralPath $app -PathType Leaf)) {
    throw "Application not found: $app"
}

$required = @(
    'SteamMarketTerminal.exe',
    'WebView2Loader.dll',
    'Qt6Charts.dll',
    'Qt6Core.dll',
    'Qt6Gui.dll',
    'Qt6Network.dll',
    'Qt6OpenGL.dll',
    'Qt6OpenGLWidgets.dll',
    'Qt6Sql.dll',
    'Qt6Widgets.dll',
    'libgcc_s_seh-1.dll',
    'libstdc++-6.dll',
    'libwinpthread-1.dll',
    'platforms\qwindows.dll',
    'sqldrivers\qsqlite.dll',
    'tls\qschannelbackend.dll',
    'qt.conf'
)
$missing = @($required | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $resolvedDir $_) -PathType Leaf)
})
if ($missing.Count -gt 0) {
    throw "Runtime is incomplete. Missing: $($missing -join ', ')"
}

$resolvedSmoke = [System.IO.Path]::GetFullPath($SmokePath)
$smokeParent = Split-Path -Parent $resolvedSmoke
if (-not (Test-Path -LiteralPath $smokeParent -PathType Container)) {
    New-Item -ItemType Directory -Path $smokeParent | Out-Null
}

$cleanPath = $env:SystemRoot + '\System32;' + $env:SystemRoot
$originalPath = $env:PATH
try {
    $env:PATH = $cleanPath
    $process = Start-Process -FilePath $app `
        -WorkingDirectory ([System.IO.Path]::GetTempPath()) `
        -ArgumentList @('--smoke-test', $resolvedSmoke, '--smoke-size', '1040x680',
                        '--smoke-scene', 'detail') `
        -WindowStyle Hidden -Wait -PassThru
} finally {
    $env:PATH = $originalPath
}
if ($process.ExitCode -ne 0) {
    throw "Clean-PATH smoke failed with exit code $($process.ExitCode)"
}
if (-not (Test-Path -LiteralPath $resolvedSmoke -PathType Leaf)) {
    throw "Clean-PATH smoke did not create screenshot: $resolvedSmoke"
}

Write-Output "Runtime verification passed: $resolvedDir"
Write-Output "Smoke screenshot: $resolvedSmoke"
