# ADR-003：使用 WebView2 承载 Steam 官方登录与上架页面

- 状态：建议批准
- 日期：2026-08-03
- 关联：CR-003、US-24、US-27

## 背景

应用需要访问用户私密库存，但不得直接收集 Steam 密码；同时必须保留 Steam 官方市场提交与确认。当前工程是 Qt 6.11 MinGW，Qt WebEngine 未安装且官方说明其不支持 MinGW。

## 方案

1. Qt WebEngine：工具链不兼容，淘汰；
2. 默认浏览器/OpenID：只能证明身份，无法复用私密库存/市场会话，作为降级；
3. WebView2 Win32：采用。用户只在 Steam 官方页面输入凭据，登录态保存在应用专属 UDF，同一容器打开官方 `market/multisell`。

## 决策

- SDK 固定稳定版 `1.0.4078.44`，运行时使用 Evergreen；
- 使用 `%LOCALAPPDATA%/SteamMarketTerminal/WebView2` 专属 UDF；
- 仅允许 Steam 官方登录/社区/商店域名导航；其他 URL 交给系统浏览器；
- 不向网页注入自动提交脚本，不暴露通用原生桥；
- Cookie 只按需复制到原生内存 CookieJar，不落 SQLite、不写日志；
- G3 先执行 MinGW COM 兼容性编译冒烟；若失败，使用独立登录代理进程实现相同 `IWebSessionHost` 接口。

## 影响

- 新增 WebView2 SDK/Loader 依赖与运行时检查；
- 移除 v3 用户名/密码和 Cookie 粘贴 UI；
- 发布包增加 Loader DLL，但不捆绑 Chromium 固定运行时。

## 官方依据

- Qt WebEngine MinGW 限制：<https://doc.qt.io/qt-6/qtwebengine-platform-notes.html>
- WebView2 UDF：<https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/user-data-folder>
- Evergreen Runtime：<https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/evergreen-vs-fixed-version>
