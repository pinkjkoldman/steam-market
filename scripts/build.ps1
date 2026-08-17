param(
    [switch]$Test
)

# 使用本机 Qt 6.11 + MinGW 工具链构建
$ErrorActionPreference = 'Stop'
$qmake = 'D:\Qt\6.11.0\mingw_64\bin\qmake.exe'
$make = 'D:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe'
$root = Split-Path -Parent $PSScriptRoot
$qtBin = Split-Path -Parent $qmake
$env:PATH = "$qtBin;$env:PATH"

Push-Location $root
try {
    Push-Location src
    try {
        & $qmake src.pro
        if ($LASTEXITCODE -ne 0) { throw "qmake(src) failed" }
        & $make -j4
        if ($LASTEXITCODE -ne 0) { throw "make(src) failed" }
    } finally {
        Pop-Location
    }
    Write-Host "构建完成：$root\bin\SteamMarketTerminal.exe"
    if ($Test) {
        Push-Location tests
        try {
            & $qmake tests.pro
            if ($LASTEXITCODE -ne 0) { throw "qmake(tests) failed" }
            & $make -j4
            if ($LASTEXITCODE -ne 0) { throw "make(tests) failed" }
        } finally {
            Pop-Location
        }
        & "$root\bin\tests.exe"
        if ($LASTEXITCODE -ne 0) { throw "tests failed" }
        Write-Host "单元测试全部通过"
    }
} finally {
    Pop-Location
}
