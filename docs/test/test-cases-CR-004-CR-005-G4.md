# CR-004 + CR-005 G4 测试用例

## 1. 账户与权限

| ID | 验收点 | 执行方式 | 结果 |
| --- | --- | --- | --- |
| G4-A01 | 首次启动可选择游客或登录；smoke 不被欢迎页阻塞 | 三档应用冒烟 + UI 审查 | 通过 |
| G4-A02 | 六种身份 × 七种能力统一由 AccessGate 判定 | 42 行数据驱动测试 | 通过 |
| G4-A03 | WebView2/清理失败时阻断登录且保持原身份 | `freshSessionFailureBlocksNavigation` | 通过 |
| G4-A04 | 新鲜会话成功后，Cookie 与 SteamID 原子发布认证态 | `authenticatesAtomicallyAfterFreshSession` | 通过 |
| G4-A05 | 取消登录后丢弃迟到回调 | `cancelDiscardsLatePreparation` | 通过 |
| G4-A06 | 信号 SteamID 与 Cookie SteamID 不一致时拒绝认证 | `rejectsMismatchedSteamIdSignal` | 通过 |
| G4-A07 | 注销调用 WebView 清理、清网络 Cookie 并回到游客 | `logoutClearsBothCookieStores` | 通过 |
| G4-A08 | 认证拒绝后变为 Expired，私有能力要求重新登录 | `sessionRejectionExpiresIdentity` | 通过 |
| G4-A09 | 真实 Steam 官方登录、Steam Guard、本人库存、注销 | 用户真实账户 UAT | 待用户确认 |

## 2. Steam 全市场与数据

| ID | 验收点 | 执行方式 | 结果 |
| --- | --- | --- | --- |
| G4-M01 | 固定 Steam fixture 解析名称、appid、价格、挂单量和总数 | `parsesFixedSteamFixture` | 通过 |
| G4-M02 | 字段缺失/负数时整页拒绝，不部分渲染 | `rejectsChangedSchemaAtomically` | 通过 |
| G4-M03 | 仅 HTTPS 精确主机/路径、count=10、输入/币种白名单 | `validatesQueryAndUrl` + 源码审计 | 通过 |
| G4-M04 | Retry-After 缺失、负数、正常值、超大值有界且不溢出 | `boundsRetryAfterDelay` | 通过 |
| G4-M05 | 查询、appid、分页、币种、语言使用不同 SHA-256 缓存键 | `cacheKeySeparatesDimensions` | 通过 |
| G4-M06 | 页面、物品、范围快照事务保存；同小时快照去重 | `repositoryRoundtripAndSnapshotDeduplication` | 通过 |
| G4-M07 | v5→v6 保留旧数据；升级前副本可恢复 v5 | `migrationUpgradesExistingSingleKeyItemsTable` | 通过 |
| G4-M08 | 官方页面端点返回 200 JSON、10 条和全站 total_count | 单次匿名联网验证 | 通过 |
| G4-M09 | 无市场写操作、无后台全量循环、无登录 Cookie 桥接 | `rg` 安全扫描 + 代码审查 | 通过 |

## 3. UI、构建与回归

| ID | 验收点 | 执行方式 | 结果 |
| --- | --- | --- | --- |
| G4-U01 | 1040×680、1280×800、1920×1080 核心内容可达 | 原生三档 smoke | 通过 |
| G4-U02 | 主按钮正文对比度 ≥4.5:1，焦点可见 | 令牌计算 + 重建截图 | 通过（5.37:1；hover 4.66:1） |
| G4-U03 | 表格双击与键盘 Enter 使用同一激活路径 | `QTableView::activated` 审查 + 构建 | 通过 |
| G4-R01 | 既有 11 类能力无回归 | Qt Test 合并日志 | 通过：104/104 |
| G4-R02 | 完整 Qt/WebView2 应用可编译链接并运行 | 强制重建 + 增量回归 | 通过 |

## 4. 联网观测

- 时间：2026-08-14T07:43:58Z；
- HTTP 200，`application/json; charset=utf-8`，8716 bytes；
- `success=true`，`start=0`，`pagesize=10`，`total_count=523504`，返回 10 条；
- 首项：appid 440，`Mann Co. Supply Crate Key`；
- 该数字是当次 Steam 响应快照，不是固定产品常量或实时全量镜像承诺。
