# CR-003 集成与自测记录

日期：2026-08-03

## 构建

- Qt 6.11.0 / MinGW 13.1 Release 构建通过。
- `SteamMarketTerminal.exe`、`tests.exe`、`WebView2Loader.dll` 均已生成。
- `scripts/build.ps1 -Test` 会自动把 Qt bin 加入 PATH，开发机测试不再依赖调用者环境。
- `scripts/package.ps1` 使用 `windeployqt` 收集 Qt 运行库，并显式复制 WebView2 Loader。

## 测试

- 标准聚合测试脚本退出码：0。
- CR-003 新增测试：7 passed、0 failed、0 skipped、0 warnings。
- 覆盖：资产/描述关联、非法载荷、原子快照、分组、整数金额、手续费、41 组拆成 40+1 两批。
- 应用冒烟：退出码 0；完成 v5 临时数据库迁移、服务装配、主窗口启动及库存助手截图。

## 安全扫描

源代码扫描未发现 `createbuyorder`、`cancelbuyorder`、`sellitem`、旧 RSA 登录端点或市场 POST 调用，结果：PASS。

## 环境说明

未设置 Qt PATH 时曾出现 `0xC0000135`，确认属于冒烟进程缺少 Qt DLL；补齐 PATH 后同一二进制通过。发布包由 `windeployqt` 解决该前置条件。
