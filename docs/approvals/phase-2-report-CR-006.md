# CR-006 阶段 2 系统设计报告

## 阶段信息

- 阶段编号/名称：G2 / 系统设计
- 负责角色：系统架构师、数据库工程师、安全工程师、UI/UX 设计师、总设计师
- 日期：2026-08-17
- 输入门禁：CR-006 G1 已批准（用户“进入G2”）

## 交付物清单

- [x] `docs/design/architecture-CR-006-real-market-trends.md`：逻辑/数据/部署边界、状态机、流程、性能与可观测性
- [x] `docs/design/adr/ADR-005-typed-on-demand-market-history.md`：类型化单品按需历史模块决策
- [x] `docs/design/api-contract-CR-006-real-market-trends.yaml`：HistoryDataset、provenance、错误与刷新契约
- [x] `docs/design/data-model-CR-006-real-market-trends.md`：HistoryKey/Point/Dataset/Provenance/Quality/Kline 语义
- [x] `docs/design/database-design-CR-006-real-market-trends.md`：v7 表重建、同步状态、索引、备份恢复与保留策略
- [x] `docs/design/security-CR-006-real-market-trends.md`：STRIDE、会话、校验、日志、fixture 与真实测试安全基线
- [x] `docs/design/ux-CR-006-real-market-trends.md`：用户流程、状态恢复、文案与可访问性
- [x] `docs/design/ui-spec-CR-006-real-market-trends.md`：详情线框、双子图、组件契约、状态与截图矩阵
- [x] `docs/design/design-tokens-CR-006-real-market-trends.md`：历史状态与图表增量令牌

## 关键决策

1. **独立历史深模块**：采用 `MarketHistoryService + SteamHistoryParser + PriceHistoryRepository + HistoryDataset`，不继续扩张字符串错误的 `MarketService::detailReady`。
2. **双图共享连续时间范围**：价格使用 `QLineSeries/QScatterSeries`，成交量使用独立 `QAreaSeries` 子图；不再把 `QBarSeries` 挂到 `QDateTimeAxis`。
3. **v7 重建历史表**：移除 0001 遗留三列唯一约束，新增 `source_kind/fetched_at` 与 `history_sync_state`；旧数据统一标 `legacy_unknown`。
4. **登录恢复由 AppController 编排**：历史服务不持有 Cookie、不弹 UI；登录成功只自动重试当前 key 一次。
5. **错误不猜测认证**：429、一般 403/400 与明确会话失效分离；只有登录重定向/认证 Cookie 明确失效等证据才标 Expired。
6. **精确行情语义**：页签使用“24h 历史点”“日 K（聚合）”；断档不插值、单点不补造。

## 验收对照

| 需求/故事 | 设计落实 | 证据 |
| --- | --- | --- |
| CR6-F1 / US-45 | 单品按需、缓存优先、纯 Parser、事务写入、类型化数据集 | 架构 3～6；API；数据模型 |
| CR6-F2 / US-47 | online/cache/empty + typed terminal state；provider/source/time/pointCount | API `HistoryDataset`；UX 状态表 |
| CR6-F3 / US-46 | 保存当前 key/周期/generation；登录成功一次性重试 | 架构 5.2；安全 4；UX 4 |
| CR6-F4/F5 | 状态/Content-Type/schema/有效率校验；UTC、排序、去重 | 架构 6；安全 5；数据库 2～4 |
| CR6-F6 / US-48 | 上下双图共享时间范围、单点/断档/volume null 处理 | UI 规格 2.1 |
| CR6-F7 / US-49 | 24h 与日 K 聚合文案；sampleCount/sparse | 数据模型 7；UI 规格 2.2 |
| CR6-F8 | 缓存保留 + warning；失败不显示在线更新 | 架构状态机；UX/UI 状态 |
| CR6-F9 | `smoke_fixture` 路径防护，旧数据 `legacy_unknown` | 数据模型不变量；安全 8；数据库迁移 |
| CR6-F10 / US-51 | 用户主动官方登录、日志/截图脱敏、真实联网矩阵 | 安全 7～10；UI 规格 7 |
| CR6-F11～F13 / US-50 | tooltip、刷新防重、限流倒计时、质量摘要 | UI 组件契约；API quality/error |

## 契约冻结清单

G3 不得擅自改变：

- `HistoryKey`、`HistoryPoint`、`HistoryDataset`、`HistoryProvenance` 与错误码；
- 状态含义及 online/cache/empty 不变量；
- 单 key 单并发、登录成功仅一次自动重试、旧 generation 不覆盖当前 UI；
- v7 `source_kind/fetched_at` 与 `history_sync_state` 物理字段；
- 旧数据为 `legacy_unknown`、普通模式拒绝 `smoke_fixture`；
- 双子图共享时间范围与“24h 历史点/日 K（聚合）”文案；
- 真实联网测试不得记录凭据或身份信息。

## 设计自检

- 架构/API/数据/UI 字段一致性：通过（HistoryKey、状态、来源、错误码逐项对照）；
- Must/Should 追溯：CR6-F1～F13 均有设计落点；
- 数据库：旧唯一约束、迁移、索引、保留、备份与恢复已覆盖；
- 安全：高危凭据泄漏、伪来源、高频重试均有设计缓解；
- 可访问性/响应式：3 分辨率 × 3 DPI × 8 状态矩阵已定义；
- 文档 diff whitespace 检查：通过；
- 产品代码：G2 未修改 `src/`、`tests/` 或 `scripts/`。

## 风险与待决问题

- R-021～R-024：设计缓解已完成，保持开放至 G3/G4 实现和真实联网验证；
- 新增 R-025：`items` 仍是单列 `market_hash_name` 主键，跨 appid 同名物品属于既有 schema 债务；CR-006 历史唯一键已包含 appid，但全表复合主键重构留待独立变更；
- Steam 网页历史端点无正式客户端契约，仍是外部高风险，不能通过设计完全消除；
- 无 G3 设计阻塞。

## 下一阶段计划

进入 G3 开发实现，按依赖顺序：

1. 核心模型、错误码、纯 Parser 与单元测试；
2. v7 迁移、备份恢复、仓储与迁移测试；
3. MarketHistoryService、会话/限流编排与模拟 HTTP 集成测试；
4. DetailPage、状态横幅、双子图、K 线语义与多尺寸冒烟；
5. 全量构建、测试、迁移恢复和契约一致性验证。

真实 Steam 账户联网验收属于 G4，由用户主动登录完成。

## 审批请求

- 请批准 CR-006 G2 并进入 G3 开发；如需修改，请列出修改意见。
