# CR-004 Steam 全市场目录数据库设计（G2 修订）

## 1. 目标

SQLite 只缓存用户访问的 Steam 全市场分页和范围摘要，不构建 52 万物品的强制全量镜像。迁移预留增量采集能力，同时让缓存可以独立清除和回滚。

## 2. 迁移

G3 新增版本化迁移 `0006_cr004_market_catalog.sql`。本文件是设计基线，G2 不写迁移脚本。

### market_catalog_pages

| 字段 | 类型 | 约束 | 说明 |
|---|---|---|---|
| cache_key | TEXT | PK | 规范化查询 SHA-256 |
| query_json | TEXT | NOT NULL | 规范化非敏感查询 |
| appid | INTEGER | NULL | NULL 表示全站 |
| offset | INTEGER | NOT NULL CHECK >=0 | Steam 分页位置 |
| page_size | INTEGER | NOT NULL CHECK 1..100 | Steam 实际页大小；当前实测为 10，不能假定请求值生效 |
| total_count | INTEGER | NOT NULL CHECK >=0 | Steam 响应总数 |
| result_json | TEXT | NOT NULL | 当前页必要字段，不存 HTML |
| currency | TEXT | NOT NULL | CNY 等 |
| language | TEXT | NOT NULL | schinese 等 |
| fetched_at | DATETIME | NOT NULL | UTC |
| expires_at | DATETIME | NOT NULL | TTL |

索引：`(appid, fetched_at DESC)`、`(expires_at)`。

### market_scope_snapshots

| 字段 | 类型 | 约束 | 说明 |
|---|---|---|---|
| id | INTEGER | PK AUTOINCREMENT |  |
| scope_key | TEXT | NOT NULL | `all` / `appid:730` |
| total_count | INTEGER | NOT NULL CHECK >=0 | Steam total_count |
| fetched_at | DATETIME | NOT NULL | UTC |
| source | TEXT | CHECK = `steam_market_search` | 来源 |

唯一索引：`(scope_key, fetched_at)`；查询索引：`(scope_key, fetched_at DESC)`。

### items 扩展

建议增加：

- `localized_name TEXT`；
- `sell_listings INTEGER`；
- `lowest_sell_minor INTEGER`；
- `catalog_seen_at DATETIME`；
- `catalog_currency TEXT`；
- `catalog_language TEXT`。

现有 `market_hash_name` 单列主键在跨 appid 理论上可能冲突；迁移前必须扫描冲突。若存在冲突，G3 需执行表重建，将主键改为 `(appid, market_hash_name)` 并同步外键；若无冲突，本轮先保留兼容结构并登记技术债。

## 3. 写入一致性

每个分页响应在单一事务中：

1. upsert 当前页物品；
2. upsert/replace 页缓存；
3. 插入范围总数快照；
4. 提交后发布 UI 页面。

网络失败不覆盖旧缓存；JSON 解析不完整时整页回滚。请求代次不是数据库字段，防止旧响应覆盖当前 UI 由服务层负责。

## 4. 缓存键

对规范化 JSON 做 SHA-256，字段固定顺序：`query, appid, filters, sort, direction, currency, language, offset, limit`。空值必须规范化，防止语义相同查询产生多份缓存。

## 5. 保留与容量

- 普通页：最近访问 30 天或最多 5,000 页，按 LRU 清理；
- 热门页：保留 90 天范围快照用于展示数量变化，但首页只用最新有效值；
- `market_scope_snapshots` 每范围每小时最多一条，保留 180 天；
- 清理只删除目录缓存，不删关注、提醒、持仓、库存和深度价格历史；
- 单页 `result_json` 设大小上限 2 MB，超过视为结构异常。

## 6. 查询性能

浏览结果优先直接反序列化页 JSON，避免对 100 行做多表 N+1。关注状态通过一次 `IN (...)` 批量查询合并。若后续要求离线全目录筛选，再单独迁移为规范化目录表和 FTS5，不在当前切片提前复杂化。

## 7. 升级与回滚

升级前备份数据库；迁移只新增表/可空列时为向后兼容。回滚应用版本时旧程序忽略新表列；要完全回滚可在用户确认后删除 `market_catalog_pages`、`market_scope_snapshots`，不触碰业务数据。若发生 `items` 主键重建，则必须提供独立 forward-only 数据迁移与恢复备份，不能简单 drop。

## 8. 验证

- 同查询不同页/币种/语言不串缓存；
- 事务中途失败不留下半页；
- TTL、LRU 与容量边界；
- 52 万 `total_count` 不触发预分配或全表扫描；
- items 潜在跨 appid 同名冲突扫描；
- 回滚后旧版关注/持仓/价格数据仍可读取。
