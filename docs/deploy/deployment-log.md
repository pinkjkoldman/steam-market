# 部署记录

## 发布信息

- 版本：v1.0.0（build 2026-08-01）
- 目标环境：Windows 10/11 x64（个人单机）
- 部署方式：本地目录部署（无服务器）

## 部署步骤（已执行）

1. 构建 Release 主程序与测试：`scripts/build.ps1` ✓
2. 运行单元测试：14/14 通过 ✓
3. 冒烟自测：`bin\SteamMarketTerminal.exe --smoke-test` 退出码 0 ✓
4. windeployqt 打包自包含目录到 `release/SteamMarketTerminal/` ✓
5. 生成 `qt.conf`、精简 SQL 驱动、附带说明与样例 ✓
6. 干净 PATH（移除 Qt 目录）下运行发布目录 exe 冒烟：退出码 0 ✓
7. 生成制品：
   - `release/SteamMarketTerminal-portable.zip`（25.7 MB）
   - `release/SteamMarketTerminal-Setup.exe`（18.3 MB，7z 自解压）
8. 安装包解压回归：解压后程序干净 PATH 冒烟退出码 0 ✓

## 健康检查

- 启动自检：数据库迁移、自选/持仓/提醒服务初始化（冒烟覆盖）✓
- 运行日志：`%APPDATA%/Personal/SteamMarketTerminal/logs/app.log` ✓
- 已知限制：真实 Steam 接口与托盘通知需联网/托盘环境人工确认。

## v2 部署补充（CR-001）

- 2026-08-01 生成 v2.0.0 制品：
  - `release/SteamMarketTerminal-portable.zip`（25.8 MB）
  - `release/SteamMarketTerminal-Setup.exe`（18.3 MB）
- 打包后干净 PATH 冒烟退出码 0，截图生成；
- 迁移 0002 随首次启动自动应用，老库向前兼容。
