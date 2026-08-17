# v4 SQLite 物理设计（CR-003）

## 1. 迁移策略

- 新迁移：`0005_v4_inventory_assistant.sql`（G3 实现）；
- 只新增表和索引，不删除/修改 v3 表，确保旧版本可回滚运行；
- 迁移在单事务内执行，由现有 `DatabaseManager` 标记版本 5；
- 回滚方案：停止 v4 应用，备份数据库，按外键逆序删除 v4 表；v1~v3 数据不受影响；
- v3 `sessions.cookie_encrypted` 不导入新表；首次启动 v4 必须通过官方页面重新登录。

## 2. 表设计

### steam_accounts

```sql
CREATE TABLE steam_accounts (
    steam_id          TEXT PRIMARY KEY,
    display_name      TEXT,
    session_state     TEXT NOT NULL CHECK (session_state IN ('anonymous','authenticated','expired','unavailable')),
    last_verified_at  TEXT,
    created_at        TEXT NOT NULL,
    updated_at        TEXT NOT NULL
);
```

### inventory_syncs

```sql
CREATE TABLE inventory_syncs (
    sync_id        TEXT PRIMARY KEY,
    steam_id       TEXT NOT NULL REFERENCES steam_accounts(steam_id) ON DELETE CASCADE,
    appid          INTEGER NOT NULL,
    context_id     TEXT NOT NULL,
    state          TEXT NOT NULL CHECK (state IN ('running','completed','failed','cancelled')),
    cursor         TEXT,
    page_count     INTEGER NOT NULL DEFAULT 0 CHECK (page_count >= 0),
    asset_count    INTEGER NOT NULL DEFAULT 0 CHECK (asset_count >= 0),
    error_code     TEXT,
    started_at     TEXT NOT NULL,
    completed_at   TEXT
);
CREATE INDEX idx_inventory_syncs_context_time
    ON inventory_syncs(steam_id, appid, context_id, started_at DESC);
```

### inventory_descriptions

```sql
CREATE TABLE inventory_descriptions (
    appid              INTEGER NOT NULL,
    class_id           TEXT NOT NULL,
    instance_id        TEXT NOT NULL,
    market_hash_name   TEXT,
    display_name       TEXT NOT NULL,
    icon_url           TEXT,
    item_type          TEXT,
    category           TEXT NOT NULL DEFAULT 'unknown',
    marketable         INTEGER NOT NULL DEFAULT 0 CHECK (marketable IN (0,1)),
    tradable           INTEGER NOT NULL DEFAULT 0 CHECK (tradable IN (0,1)),
    tags_json          TEXT NOT NULL DEFAULT '[]' CHECK (json_valid(tags_json)),
    updated_at         TEXT NOT NULL,
    PRIMARY KEY (appid, class_id, instance_id)
);
CREATE INDEX idx_inventory_desc_hash
    ON inventory_descriptions(appid, market_hash_name);
CREATE INDEX idx_inventory_desc_category
    ON inventory_descriptions(category, marketable);
```

### inventory_assets

```sql
CREATE TABLE inventory_assets (
    steam_id           TEXT NOT NULL REFERENCES steam_accounts(steam_id) ON DELETE CASCADE,
    appid               INTEGER NOT NULL,
    context_id          TEXT NOT NULL,
    asset_id            TEXT NOT NULL,
    class_id            TEXT NOT NULL,
    instance_id         TEXT NOT NULL,
    amount              INTEGER NOT NULL DEFAULT 1 CHECK (amount > 0),
    last_seen_sync_id   TEXT NOT NULL REFERENCES inventory_syncs(sync_id),
    marketable_after    TEXT,
    last_seen_at        TEXT NOT NULL,
    PRIMARY KEY (steam_id, appid, context_id, asset_id),
    FOREIGN KEY (appid, class_id, instance_id)
        REFERENCES inventory_descriptions(appid, class_id, instance_id)
);
CREATE INDEX idx_inventory_assets_group
    ON inventory_assets(steam_id, appid, context_id, class_id, instance_id);
CREATE INDEX idx_inventory_assets_sync ON inventory_assets(last_seen_sync_id);
```

### listing_drafts / listing_draft_items

```sql
CREATE TABLE listing_drafts (
    draft_id             TEXT PRIMARY KEY,
    steam_id             TEXT NOT NULL REFERENCES steam_accounts(steam_id) ON DELETE CASCADE,
    status               TEXT NOT NULL CHECK (status IN ('draft','quoting','ready','handed_off','stale','abandoned')),
    currency             TEXT NOT NULL,
    strategy_type        TEXT NOT NULL CHECK (strategy_type IN ('fixed','lowest_sell_minus_tick','highest_buy')),
    strategy_value_minor INTEGER,
    keep_per_group       INTEGER NOT NULL DEFAULT 0 CHECK (keep_per_group >= 0),
    inventory_synced_at  TEXT NOT NULL,
    created_at           TEXT NOT NULL,
    updated_at           TEXT NOT NULL
);

CREATE TABLE listing_draft_items (
    id                    INTEGER PRIMARY KEY AUTOINCREMENT,
    draft_id              TEXT NOT NULL REFERENCES listing_drafts(draft_id) ON DELETE CASCADE,
    appid                  INTEGER NOT NULL,
    context_id             TEXT NOT NULL,
    market_hash_name       TEXT NOT NULL,
    inventory_quantity     INTEGER NOT NULL CHECK (inventory_quantity > 0),
    selected_quantity      INTEGER NOT NULL CHECK (selected_quantity > 0 AND selected_quantity <= inventory_quantity),
    buyer_pays_minor       INTEGER CHECK (buyer_pays_minor > 0),
    fee_minor              INTEGER CHECK (fee_minor >= 0),
    seller_receives_minor  INTEGER CHECK (seller_receives_minor >= 0),
    state                  TEXT NOT NULL CHECK (state IN ('included','excluded','quote_failed','stale')),
    exclusion_reason       TEXT,
    quote_at               TEXT,
    UNIQUE (draft_id, appid, context_id, market_hash_name)
);
CREATE INDEX idx_listing_draft_items_state ON listing_draft_items(draft_id, state);
```

### handoff_batches

```sql
CREATE TABLE handoff_batches (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    draft_id       TEXT NOT NULL REFERENCES listing_drafts(draft_id) ON DELETE CASCADE,
    batch_no       INTEGER NOT NULL CHECK (batch_no > 0),
    appid          INTEGER NOT NULL,
    context_id     TEXT NOT NULL,
    official_url   TEXT NOT NULL,
    status         TEXT NOT NULL CHECK (status IN ('ready','opened','abandoned')),
    opened_at      TEXT,
    UNIQUE (draft_id, batch_no)
);
```

## 3. 一致性与查询

- 每页同步事务顺序：upsert descriptions → upsert assets → update sync cursor/count；
- 完整同步结束：在事务中把 sync 置 `completed`，再删除相同上下文中 `last_seen_sync_id != current` 的资产；
- 失败同步绝不删除旧资产；缓存查询返回上次成功同步时间并置 `stale=true`；
- 聚合查询按 `market_hash_name/category` 分组，数量使用 `SUM(amount)`；
- 所有列表分页使用稳定排序 `(category, display_name, market_hash_name)`，游标为最后一行复合键；禁止大 OFFSET；
- 草稿创建、项目插入、汇总校验在一个事务中完成。

## 4. 性能目标

| 查询 | 目标 | 索引依据 |
| --- | --- | --- |
| 1000 项库存筛选/分页 | <150ms | assets group + desc category |
| 同步单页 5000 资产写入 | <2s（本机 SSD） | 事务批量预编译语句 |
| 草稿 100 组汇总 | <100ms | draft_id/state 索引 |

## 5. 备份、恢复与敏感数据

- 延续现有 SQLite 在线备份；迁移前自动创建 `.bak`；
- WebView2 UDF 与 SQLite 分开备份，默认不备份会话目录；
- 数据库不包含密码、Cookie、Steam Guard、恢复码；SteamID 属于账号标识，日志脱敏；
- “退出并清除本机登录”清理 WebView2 UDF，不删除库存缓存；“删除本地账户数据”再级联删除账号库存和草稿。
