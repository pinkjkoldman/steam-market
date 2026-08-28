# CR-006 数据库设计：历史来源、同步状态与 v7 迁移

## 1. 现状问题

当前 `price_history` 在 `0001_init.sql` 中仍有表级唯一约束：

```sql
UNIQUE (market_hash_name, currency, recorded_at)
```

`0003_v3.sql` 只是额外创建包含 `appid` 的唯一索引，没有移除旧约束。因此同名、同币种、同时间但不同 appid 的记录仍可能被旧约束拒绝。此外表中没有来源和获取时间，UI 无法证明在线/缓存状态。

## 2. 物理表

### 2.1 `price_history` v7

```sql
CREATE TABLE price_history_v7 (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    market_hash_name TEXT NOT NULL REFERENCES items(market_hash_name) ON DELETE CASCADE,
    appid            INTEGER NOT NULL CHECK (appid > 0),
    currency         TEXT NOT NULL CHECK (length(currency) = 3),
    source_kind      TEXT NOT NULL CHECK (
        source_kind IN ('steam_history','legacy_unknown','smoke_fixture')
    ),
    price            REAL NOT NULL CHECK (price > 0),
    volume           INTEGER CHECK (volume IS NULL OR volume >= 0),
    recorded_at      TEXT NOT NULL,
    fetched_at       TEXT,
    CHECK (
        (source_kind = 'legacy_unknown' AND fetched_at IS NULL)
        OR (source_kind <> 'legacy_unknown' AND fetched_at IS NOT NULL)
    ),
    UNIQUE (market_hash_name, appid, currency, source_kind, recorded_at)
);
```

索引：

```sql
CREATE INDEX idx_history_v7_key_time
ON price_history (market_hash_name, appid, currency, source_kind, recorded_at ASC);

CREATE INDEX idx_history_v7_retention
ON price_history (source_kind, recorded_at);
```

### 2.2 `history_sync_state`

```sql
CREATE TABLE history_sync_state (
    market_hash_name TEXT NOT NULL REFERENCES items(market_hash_name) ON DELETE CASCADE,
    appid            INTEGER NOT NULL CHECK (appid > 0),
    currency         TEXT NOT NULL CHECK (length(currency) = 3),
    source_kind      TEXT NOT NULL CHECK (source_kind = 'steam_history'),
    last_attempt_at  TEXT NOT NULL,
    last_success_at  TEXT,
    retry_after_at   TEXT,
    point_count      INTEGER NOT NULL DEFAULT 0 CHECK (point_count >= 0),
    first_point_at   TEXT,
    last_point_at    TEXT,
    last_error_code  TEXT CHECK (last_error_code IS NULL OR last_error_code IN (
        'AUTH_REQUIRED','SESSION_EXPIRED','RATE_LIMITED','NETWORK_UNAVAILABLE',
        'SOURCE_REJECTED','SOURCE_SCHEMA_CHANGED','DATABASE_ERROR','CANCELLED'
    )),
    PRIMARY KEY (market_hash_name, appid, currency, source_kind)
);
```

不保存 `last_error_message`、HTTP body、URL、Cookie 或身份信息。用户文案由错误码映射。

## 3. 数据访问契约

`PriceHistoryRepository` 提供：

```text
load(key, sourcePreference, fromUtc?, toUtc?, limit=50000) -> StoredHistoryDataset
upsertValidated(key, sourceKind, fetchedAtUtc, points, quality) -> WriteResult
recordAttempt(key, attemptedAtUtc, errorCode?, retryAfterUtc?) -> bool
syncState(key) -> HistorySyncState
```

- `upsertValidated` 在一个事务中写点、更新成功元数据和清除错误；
- 每批使用准备语句，禁止逐行重新 prepare；
- 读取必须显式 `ORDER BY recorded_at ASC`；
- 默认优先 `steam_history`；仅无 Steam 来源时读取 `legacy_unknown`，且 provenance 保持未知；
- 50,000 点上限由仓储强制，超限返回可恢复错误，不静默截断为“全部”。

## 4. 迁移 0007 设计

### 4.1 升级前备份

`DatabaseManager` 在发现 v7 未应用时必须：

1. `PRAGMA wal_checkpoint(FULL)`；
2. 使用 SQLite backup API 或关闭写连接后的可靠复制生成 `steam_market.db.pre-v7.bak`；
3. 校验备份文件存在且大小 >0；失败则不执行迁移。

不得先删除已有 `.pre-v7.bak`；若同名存在，使用带 UTC 时间戳的后缀。

### 4.2 事务内表重建

`0007_cr006_real_history.sql`：

1. 创建 `price_history_v7`；
2. 从旧表复制合法行，`source_kind='legacy_unknown'`、`fetched_at=NULL`；
3. 将负数/非法 price 行计入迁移诊断并拒绝启动，不静默丢弃；volume `<=0` 的旧 null/0 语义保留为 null 或 0，以原值为准；
4. 验证复制前后合法记录数量一致；
5. 删除旧表并将新表改名；
6. 创建索引与 `history_sync_state`；
7. 由迁移器统一写 schema 版本 7。

由于当前迁移器只按分号拆分脚本，数量验证和备份前置检查由 `DatabaseManager` C++ 代码执行；SQL 脚本不使用触发器或含分号的复杂块。

### 4.3 回滚

SQLite 表重建不提供线上 down migration。回滚流程：

1. 关闭应用并保留失败库为 `.failed-v7`；
2. 用 `.pre-v7.bak` 恢复主库；
3. 打开只读连接执行 `PRAGMA integrity_check`；
4. 使用 v6 程序验证 schema 版本与关键查询；
5. 不尝试把 v7 新数据反向合并到 v6。

G3 必须自动验证 v6→v7、重复升级幂等、迁移失败回滚和备份恢复。

## 5. 保留策略

- `steam_history`：用户动作获取的官方历史点不按 5 年自动删除；直到用户清理对应物品或全部历史；
- `legacy_unknown`：继续保留 5 年；
- `smoke_fixture`：仅临时数据库，进程退出后整个临时目录删除；
- `history_sync_state`：随物品级历史删除；不单独保存无限错误历史；
- `price_snapshots`：保持 90 天策略。

理由：Steam 历史按需获取且去重，删除后下一次请求会重复下载；“全部”周期也要求保留可追溯历史。

## 6. 一致性与性能

- 打开数据库后执行 `PRAGMA foreign_keys=ON`、WAL、`synchronous=NORMAL`；迁移/备份阶段提升为 FULL；
- 关键区间查询使用 `idx_history_v7_key_time`，目标执行计划为索引范围扫描；
- 单次写入最多 50,000 点，事务失败全部回滚，同步状态不得先标成功；
- `history_sync_state.point_count/first/last` 在成功事务内由查询结果计算，不能信任远端声明；
- 数据库时间统一保存带 `Z` 的 UTC ISO-8601；显示层转换本地时区。

## 7. 已知边界

`items` 当前以 `market_hash_name` 单列为主键，仍不能完整表达跨 appid 同名物品。这是既有跨模块 schema 债务；CR-006 的历史唯一键已经包含 appid，但不在本变更中重建全部引用 `items` 的表。登记为后续独立迁移风险，G3 必须至少验证历史表不会被旧三列唯一约束阻塞。

## 8. 自评审

- [x] 表级旧唯一约束通过重建移除；
- [x] 来源、发生时间、获取时间语义分离；
- [x] 迁移前备份和恢复步骤明确；
- [x] 写入事务、索引、保留策略和敏感数据排除明确；
- [x] 迁移可自动验证且不伪造旧数据来源；
- [ ] `items` 复合主键债务未在 CR-006 解决，已明确边界。

