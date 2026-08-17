param(
    [string]$Shot = "$PSScriptRoot\..\smoke.png"
)

# 冒烟自测：核心逻辑校验 + 主窗口截图
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $env:PATH = 'D:\Qt\6.11.0\mingw_64\bin;' + $env:PATH
    $shot = Join-Path (Resolve-Path $root) (Split-Path $Shot -Leaf)
    & ".\bin\SteamMarketTerminal.exe" --smoke-test $shot
    if ($LASTEXITCODE -ne 0) { throw "smoke test failed" }
    Write-Host "冒烟测试通过，截图：$shot"
} finally {
    Pop-Location
}
