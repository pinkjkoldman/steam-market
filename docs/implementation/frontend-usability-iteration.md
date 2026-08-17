# 前端可用性迭代实现说明

## 改动范围

- `src/app/MainWindow.*`：应用壳层、页面标题、统一导航状态机、来源返回、状态栏、快捷键。
- `src/ui/pages/MarketPage.*`：搜索主操作、加载锁定、状态反馈、单选表格、键盘激活。

## 运行方式

```powershell
./scripts/build.ps1 -Test
./bin/SteamMarketTerminal.exe
```

## 验证结果

- Release 构建：通过。
- 现有单元测试：全部通过。
- 内置 smoke-test：通过；截图输出 `steam-ui-optimized-v1.png`，1680×1080。
- 编译器启用 `-Wall -Wextra`，本轮改动无新增警告。

## 沙箱诊断

默认 Windows 沙箱在执行任何命令前稳定报错：`windows sandbox: helper_unknown_error: setup refresh had errors`。同一只读命令在获批的非沙箱环境稳定成功，文件 ACL 也允许工作区账户修改，因此排除项目权限和补丁内容问题，定位为 Codex 桌面沙箱 helper 初始化/挂载刷新故障。

本轮安全回退方式：仅在已授权工作区内，通过 `git apply` 应用统一 diff；每个逻辑块后读取校验，并以完整构建、单元测试和内置冒烟测试收口。内置 `view_image` 同样受 helper 故障影响，但生成的 PNG 已通过文件头、尺寸和 System.Drawing 解码验证。

## 后续建议

- 为 `MainWindow` 增加 Qt Test 导航用例，覆盖“来源页→详情→返回来源”和 Alt 快捷键。
- 将全局 QSS 从构造函数迁移到独立资源文件，便于主题维护。
