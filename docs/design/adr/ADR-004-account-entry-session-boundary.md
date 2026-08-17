# ADR-004：账户入口、游客模式与会话边界

- 状态：已提议，待 G2 审批
- 日期：2026-08-13
- 关联：CR-004、CR-003

## 背景

现有应用已具备官方 Steam Web 登录和 Cookie 内存桥，但启动时没有明确身份选择，UI 还通过若干布尔值组合推断会话。CR-004 要求登录入口、游客模式、统一账户态与更好的启动体验，同时明确不保存 Steam 凭据、不跨重启恢复认证。

## 决策

1. 扩展 `SteamSessionService` 为唯一身份状态源，使用显式 `IdentityState` 和原子 `IdentitySnapshot`。
2. 新增 `StartupExperienceController` 负责编排欢迎层、权限门和一次性待执行意图，不再创建另一套认证服务。
3. 官方 Steam WebView 是唯一登录入口；应用不提供用户名、密码、验证码输入控件。
4. WebView 登录前清理遗留认证 Cookie，退出时再次清理；当前进程内的认证 Cookie 只驻留内存桥。
5. SQLite 仅保存 `startupIdentityMode=ask|guest`，不保存 Cookie、身份快照或认证成功标记。
6. 游客是正式身份态，不等同于“steamId 为空”；公开库存是独立受限态。
7. 权限判定集中在纯函数 `AccessGate`，页面和后台命令不得各自复制判断。

## 后果

正面：

- 启动、登录、失效和注销行为可测试且可解释；
- 防止 UI 与后端认证状态漂移；
- 游客流程不依赖 Steam 网络或 WebView2；
- 保持 CR-003 安全边界，并减少 G3 改造面。

代价：

- 需要迁移现有 `hasSession/authenticated` 布尔判断；
- WebView 首次登录前增加异步清理步骤；
- 页面必须通过能力门访问受限功能。

## 被否决方案

- 新建 AuthService：造成双状态源和注销不同步。
- 直接复用空 SteamID 表示游客：无法表达选择前与会话失效。
- 默认恢复 WebView2 Cookie：与隐私约束冲突。
- 启动即强制登录：破坏离线浏览与 smoke 测试。

## 验证

G3 必须以状态机单测、权限矩阵表驱动测试、残留 Cookie 清理测试、无网络游客 smoke 测试验证本决策。若无法在导航前可靠清理旧 Cookie，则必须阻断登录并报告安全错误，不得继续加载官方登录页。
