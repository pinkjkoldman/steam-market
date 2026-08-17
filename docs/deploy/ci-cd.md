# CI/CD 与打包流水线

## 1. 本地流水线（Windows + Qt 6.11 MinGW）

```powershell
# 1) 构建（应用 + 测试）
scripts/build.ps1 -Test

# 2) 发布打包（windeployqt 自包含 + ZIP + 自解压安装包）
scripts/package.ps1
```

`package.ps1` 固定执行顺序：

1. 编译 Release 主程序；
2. 复制 exe 到 `release/SteamMarketTerminal/`；
3. `windeployqt` 收集 Qt 运行库与插件（platforms、sqldrivers、tls、imageformats、styles 等）；
4. 生成 `qt.conf`（`Prefix=.`、`Plugins=.`），保证无 Qt 环境可运行；
5. 精简 sqldrivers（仅保留 SQLite）；
6. 附带 `使用说明.txt` 与 `样例比价数据.csv`；
7. 产出 `SteamMarketTerminal-portable.zip` 与 `SteamMarketTerminal-Setup.exe`（7z 自解压）。

## 2. 门禁

- 构建失败即终止（`$LASTEXITCODE` 校验）；
- 发布前必须通过 `tests.exe`（退出码 0）与 `--smoke-test`（退出码 0）。

## 3. 环境差异

- 应用无服务器依赖，无环境变量密钥；开发/生产同一构建产物；
- 数据目录 `%APPDATA%/Personal/SteamMarketTerminal/` 与安装目录解耦，升级安装不影响用户数据。
