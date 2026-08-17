# BE-ACCOUNT 实现说明

## 范围

本模块按 CR-004 冻结契约实现单一身份状态源、纯权限门及 WebView2 新鲜会话边界。身份状态为
`ChoiceRequired / Guest / PublicInventory / Authenticating / Authenticated / Expired`；公开身份和游客均不视为认证。

## 接口

- `IdentitySnapshot` 一次发布 state、SteamID、显示名、状态键和 busy，禁止拆分更新。
- `SteamSessionService` 新命令返回 `AccountResult`；错误通过 `operationFailed(AccountError)` 同步发布。
- `hasSession()`、`isAuthenticated()` 和 `sessionChanged(...)` 作为一个版本的只读适配。
- `AccessGate::evaluate` 是无网络、无日志、无状态副作用的权限矩阵；`evaluateDetailed` 提供稳定原因码。
- `IWebSessionHost::prepareFreshLoginSession` 在登录导航前异步返回 `Ready / Unavailable / ClearFailed`。

## 安全行为

首次登录必须先清 WebView2 遗留 Cookie；只有 `Ready` 才打开官方登录页面。Cookie 清理失败恢复登录前身份并阻断导航。认证成功必须同时具备合法 17 位 SteamID 与
`steamLoginSecure` Cookie；Cookie 只写进当前进程 `QNetworkCookieJar`，不写设置和数据库。注销清 WebView 与内存 Cookie。

设置仅持久化 `startupIdentityMode=ask|guest`，未知值回退 `ask`，不能表达自动认证。

## 测试

专属测试不创建真实 WebView，使用 `IWebSessionHost` fake 验证：

- 合法和非法状态转换；
- 同值快照不重复发布、认证字段原子发布；
- fresh-session 清理失败时不导航；
- 过期回调被 generation 丢弃；
- AccessGate 全状态与全能力矩阵；
- 设置未知启动模式回退 ask。

## 集成注意

`src/src.pro` 需加入 `AccessGate.cpp` 与 Identity 头；`tests/tests.pro` 需加入专属测试和上述生产源。
应用启动时默认保留 `ChoiceRequired`，smoke 模式应显式调用 `chooseGuest()`，不得触发 WebView。

## 实现结果

- 六态状态机、原子 `IdentitySnapshot`、请求 generation 与登录表面代次防护已落地。
- 旧 `hasSession()`、`isAuthenticated()`、`sessionChanged(...)` 保留一版，只读委托快照。
- WebView2 通过 `ICoreWebView2Profile2::ClearBrowsingData(COOKIES)` 等待完成；失败或运行时不可用均阻断登录导航。注销恢复“未准备”状态并清 WebView/内存 Cookie。
- `startupIdentityMode` 仅序列化为 `ask|guest`；设置页提供“每次询问/直接以游客进入”，未知值回退 ask。

## 验证结果

2026-08-13 使用独立临时 qmake 工程编译生产源和 `test_account_entry.cpp`，不修改集成构建文件：

- 编译通过；
- 47 passed，0 failed，0 skipped；
- 覆盖 42 个状态×能力矩阵组合，以及 fresh-session 清理失败阻断、认证快照原子发布、取消后迟到准备回调丢弃；
- 测试不创建真实 WebView、不访问 Steam 网络。

主工程 WebView2SessionHost 已由集成构建验证链接成功。真实 WebView2/Steam 页面行为仍留给低频集成冒烟。