# CR-004 账户与权限内部契约（G2）

本变更不新增远程 HTTP API，因此不扩展 OpenAPI 路径；Qt/C++ 内部契约是 G3 联调依据。Steam 网络接口继续沿用 CR-003 契约。

## 1. SteamSessionService

```cpp
class SteamSessionService : public QObject {
    Q_OBJECT
public:
    IdentitySnapshot snapshot() const;
    Result beginOfficialLogin();
    Result cancelOfficialLogin();
    Result chooseGuest();
    Result usePublicInventory(const QString &steamId64);
    Result clearPublicIdentity();
    Result logout();

signals:
    void identityChanged(const IdentitySnapshot &snapshot);
    void loginSurfaceRequested();
    void loginSurfaceClosed();
    void operationFailed(AccountError error);
};
```

约束：

- 所有命令仅在 Qt 主线程调用；
- `identityChanged` 只携带完整快照；
- 同值快照不重复发布；
- `logout()` 可幂等调用，完成后状态为 `Guest`；
- `beginOfficialLogin()` 不接收用户名、密码或 Cookie。

## 2. AccessGate

```cpp
enum class Capability {
    PublicMarket,
    LocalWatchlist,
    EnterPublicSteamId,
    PublicInventory,
    PrivateInventory,
    AccountAction,
    OpenOfficialCommunity
};

enum class AccessDecision {
    Allow,
    RequireChoice,
    RequireLogin,
    RequirePublicId,
    Busy,
    Reauthenticate
};

AccessDecision evaluate(Capability capability,
                        const IdentitySnapshot &identity);
```

函数必须纯粹、确定、无日志副作用；矩阵以架构文档第 7 节为准。

## 3. StartupExperienceController

```cpp
class StartupExperienceController : public QObject {
    Q_OBJECT
public:
    void start(const StartupOptions &options);
    void request(Capability capability, const PendingIntent &intent);
    void dismissNotice();

signals:
    void welcomeVisibilityChanged(bool visible);
    void accessPromptRequested(AccessDecision reason,
                               Capability capability);
    void navigateRequested(IntentTarget target);
};
```

- `start()` 只调用一次；重复调用返回稳定错误。
- 待执行意图容量固定为 1，后请求替换前请求并递增 `generation`。
- 登录成功仅重放当前代次一次。
- smoke 选项的优先级高于设置。

## 4. IWebSessionHost 扩展

```cpp
enum class FreshSessionResult { Ready, Unavailable, ClearFailed };
void prepareFreshLoginSession(
    std::function<void(FreshSessionResult)> completion);
```

契约：

- 创建 WebView 环境后、任何登录导航前删除遗留 Cookie；
- 清理成功才允许导航至登录 URL；
- 回调恰好一次并回到 Qt 主线程；
- 日志不得包含 Cookie 内容；
- 当前进程内后续官方页面可复用当前会话，除非用户注销或重新登录。

## 5. 错误模型

```cpp
enum class AccountErrorCode {
    InvalidTransition,
    InvalidSteamId,
    WebViewUnavailable,
    FreshSessionClearFailed,
    LoginCancelled,
    LoginNavigationBlocked,
    SteamUnavailable,
    SessionRejected,
    PublicInventoryPrivate,
    InternalError
};
```

`AccountError` 包含 `code`、可本地化的 `messageKey`、`recoverable` 和可选 `retryAfterMs`；不得携带凭据、Cookie 或完整远端正文。

## 6. UI 组件契约

| 组件 | 输入 | 输出事件 | 不得承担 |
|---|---|---|---|
| AccountEntryOverlay | snapshot、WebView 能力、busy | guestRequested、loginRequested、rememberGuestChanged | 登录判定、网络调用 |
| AccountCard | snapshot、最后同步摘要 | loginRequested、logoutRequested、manageRequested | 直接清 Cookie |
| AccessPrompt | decision、capability | loginRequested、publicIdRequested、cancelled | 自行重放动作 |
| InventoryRestrictedState | decision、公开身份摘要 | loginRequested、publicIdSubmitted | 绕过 AccessGate |

## 7. 兼容性

现有 `hasSession()` 与 `isAuthenticated()` 在 G3 可保留为只读适配器一个版本，但实现必须委托给 `IdentitySnapshot`；新代码禁止继续使用组合布尔值决策。
