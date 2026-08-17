# CR-004 + CR-005 G3 集成与自测记录

日期：2026-08-14

## 集成结果

- 应用入口已重组为欢迎页与主工作台两层体验；游客可直接进入公开市场，Steam 登录成功后进入库存助手。
- 侧栏已集成账户卡片与市场概览、物品浏览、价格分析、自选、持仓、排行、规则、库存助手、设置九个入口。
- `SteamSessionService` 是唯一身份状态源，公开 SteamID 不等于认证；`AccessGate` 负责受限能力判断。
- WebView2 每进程首次登录前异步清理浏览数据，清理失败阻断登录但不影响游客模式。
- 全市场目录链路为 `SteamMarketCatalogClient -> FullMarketService -> MarketCatalogRepository -> MarketOverviewPage/MarketBrowserPage`。
- 物品选中后按需请求单品价格概览；关注写入既有自选服务，提醒按钮进入详情页配置阈值。

## Steam 数据边界

- UI 统一表述为“Steam 当前可检索全市场”与“Steam Community Market 公开页面数据”。
- 未将内部网页端点描述为 Valve 面向普通第三方的正式全市场 API。
- 目录客户端仅访问精确 HTTPS 主机与路径，匿名 CookieJar、单并发、至少 1.5 秒间隔、每页固定 10 条。
- 不自动遍历全市场；只响应用户查询/分页，支持强缓存、过期降级、429 `Retry-After` 与有限 5xx 重试。
- 2026-08-13 单次低频探针：`success=true`、`total_count=527982`、`pagesize=10`、返回 10 条；该数字仅为时点观测，不写死到生产 UI。

## 数据库与恢复

- 迁移 0006 新增目录页缓存与范围快照，并以可空列扩展旧 `items`，不重建旧单列主键。
- 空库、v5 升级库均通过测试；同小时范围快照去重与事务原子性通过。
- 回滚采用升级前数据库副本恢复；自动测试已验证恢复后的数据库仍为 v5，且旧物品数据完整。

## 构建与测试

- 环境：Qt 6.11.0、MinGW 13.1、C++17、Release。
- 应用使用 `src/src.pro` 强制全量重编译并成功链接；仅有 WebView2 官方头文件的未知 `#pragma warning` 提示。
- 统一 `tests/tests.pro` 编译链接成功，当前全套 100 passed / 0 failed；本轮再次运行退出码 0。
- 账户模块独立验证：47 passed / 0 failed；全市场模块独立验证：8 passed / 0 failed。
- 离线 GUI 冒烟三档均退出 0：1040×680、1280×800、1920×1080。

## 视觉证据

- `docs/test/smoke-g3-overview-1040x680.png`
- `docs/test/smoke-g3-overview-1280x800.png`
- `docs/test/smoke-g3-overview-1920x1080.png`
- 三档均无横向溢出或按钮重叠；1040×680 为紧凑下限，1920×1080 保持主内容聚合而非无限拉伸。

## 开发环境说明

- Codex Windows 沙箱刷新持续出现 `helper_unknown_error`，影响内置补丁读取和 `view_image`，但不影响产品运行。
- 未扩大 ACL 或批量修改所有者；源码修改采用仓库内标准 unified diff，经 Git `patch.exe --dry-run` 后应用。
- 截图视觉检查通过只读缩略图管道完成；原始 PNG 未修改。

## 已知边界

- WebView2 真实 Steam Guard 登录、跨进程异常退出后的 Cookie 清理、真实 429/结构变化场景留到 G4 环境验证。
- 持续全量镜像、代理绕过、登录 Cookie 采集不在范围内；若未来需要，必须先获得 Valve 许可并重新走变更门禁。
