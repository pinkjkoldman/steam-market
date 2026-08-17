# 数据库设计（物理）

> 数据库工程师交付物。逻辑模型见 data-model.md，本文件描述 SQLite 物理实现、迁移、索引与数据保障。

## 1. 物理选型

- 引擎：SQLite 3（Qt SQL `QSQLITE` 驱动）；
- 文件：`%APPDATA%/SteamMarketTerminal/steam_market.db`；
- PRAGMA：`journal_mode=WAL`（并发读写友好）、`foreign_keys=ON`、`synchronous=NORMAL`；
- 编码：UTF-8。

## 2. 表结构与约束

表结构严格对应 data-model.md 的 Schema，额外的物理约定：

- 全部时间字段存 ISO-8601 文本（UTC），展示层转本地时区；
- 金额统一 REAL（元），币种由 `currency` 字段标识；不做跨币种自动换算（v1）；
- 布尔用 INTEGER 0/1；
- `market_hash_name` 为全局物品标识，`items` 为各表外键目标，启用外键约束；
- 唯一约束即上文 Schema 所列。

## 3. 索引设计

| 表 | 索引 | 用途 |
| --- | --- | --- |
| price_snapshots | `idx_snap_hash_time ON (market_hash_name, created_at DESC)` | 详情页最新快照、涨跌幅计算 |
| price_snapshots | `idx_snap_time ON (created_at)` | 定时清理旧快照 |
| price_history | `idx_history_hash_time ON (market_hash_name, recorded_at ASC)` | 走势图顺序读取 |
| price_history | `idx_history_time ON (recorded_at)` | 保留策略清理 |
| watchlist | `idx_watch_sort ON (sort_order)` | 列表排序 |
| alerts | `idx_alerts_enabled_hash ON (enabled, market_hash_name)` | 刷新时查询启用提醒 |
| portfolio_items | `idx_portfolio_hash ON (market_hash_name)` | 估值刷新关联 |
| platform_prices | 唯一约束自动建索引 `(market_hash_name, platform)` | 比价 upsert |

避免在 `price` 等高频写入列建多余索引；快照表按天级保留策略控制体积（见 §6）。

## 4. 迁移策略

- 版本化迁移：`schema_migrations(version INTEGER PRIMARY KEY, applied_at DATETIME)`；
- 迁移脚本：`src/data/migrations/0001_init.sql`、`0002_xxx.sql`（后续按需递增），只增不改；
- 启动时按序执行未应用的迁移，单事务包裹；失败则回滚并记录错误，应用进入只读降级模式（可选）；
- 回滚：SQLite 迁移原则上不回退文件结构；需要回滚的变更采用"新增表/列 + 数据迁移 + 停用旧结构"的向前兼容方式，旧版本应用不受影响；
- 开发/测试/生产均为同一套脚本，禁止手工改库。

## 5. 数据访问约定

- 仓储层（`data/repositories/`）封装全部 SQL；服务层不得出现裸 SQL；
- 一律 `QSqlQuery::prepare` 参数绑定，禁止字符串拼接 SQL；
- 批量写入（如历史点入库）使用事务，一次提交 ≥ 100 行时分批（每批 ≤ 1000 行）；
- 列表查询分页或限制行数（默认 ≤ 500），避免全表加载。

## 6. 数据保障

- **保留策略**：price_snapshots 保留最近 90 天（每日冗余合并后可保留 365 天，v1 从简：90 天）；price_history 保留最近 5 年（与接口数据量匹配）；清理任务在启动时执行一次；
- **备份**：应用退出时（或每日首次启动）将数据库复制为 `steam_market.db.bak`（WAL 模式下先 `checkpoint`）；提供设置项"立即备份"；
- **恢复**：将 `.bak` 覆盖主库后重启即可，界面提供说明（v1 不做图形化恢复向导）；
- **一致性**：外键约束 + 唯一约束兜底；upsert 用 `INSERT ... ON CONFLICT DO UPDATE`；
- **敏感数据**：本库不含账号密码；如未来引入 Steam API Key，密钥改存系统凭据存储（Windows Credential Manager），不入库。

## 7. 性能方案

- 关键路径均为索引命中：详情页 1 次索引查询取最新快照 + 1 次范围查询取历史点；
- 历史走势图按接口返回量直接入库，绘图读取 ≤ 2 年数据点内存绘制；
- 启动加载仅读取自选/持仓/提醒/设置（数据量小），行情表不预载；
- 监控慢查询（>100ms）并记日志。

---

# v2 变更（CR-001）

## 8. 迁移 0002（新增表）

| 版本 | 文件 | 变更 | 回滚 |
| --- | --- | --- | --- |
| 0002 | src/data/migrations/0002_v2.sql | 新增 `orderbook_snapshots`、`trades` | 新增表，向前兼容；回滚仅影响新功能 |

- `orderbook_snapshots`：主键 market_hash_name（每物品一份最新盘口），upsert；
- `trades`：模拟交易流水，外键关联 items；
- 费率配置不入表结构，走 settings（fee_steam_rate=0.05、fee_game_rate=0.10 默认）。

## 9. 索引与约束

- `trades(traded_at)` 索引（按时间排序展示）；
- `trades(market_hash_name)` 索引（按物品过滤）；
- 盘口表为主键 upsert，无额外索引。

## 10. 数据保障

- 盘口快照仅保留最新，防膨胀；
- 模拟交易记录保留策略：默认永久保留，提供清空入口（后续可加）。

---

# v3 变更（CR-002）

## 11. 迁移记录

| 版本 | 文件 | 变更 | 状态 |
| --- | --- | --- | --- |
| 0003 | src/data/migrations/0003_v3.sql | price_history/price_snapshots 唯一索引加入 appid | 已应用 |
| 0004（规划） | src/data/migrations/0004_v3_autotrade.sql | sessions、trading_rules、orders | 待实现 |

## 12. 自动化交易数据保障

- sessions：Cookie 字段存储 DPAPI 密文（不可读明文）；过期自动清理；
- orders：保留全部执行历史用于审计；风控字段（daily_limit、cooldown）在引擎层强制校验。
