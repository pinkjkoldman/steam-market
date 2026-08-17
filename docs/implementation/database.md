# Steam 全市场数据库实现（CR-004 / CR-005）

## 迁移 0006

`0006_cr004_market_catalog.sql` 已登记到 `migrations.qrc` 和 `DatabaseManager`：

- 新增 `market_catalog_pages`，以规范化查询 SHA-256 为主键；
- 新增 `market_scope_snapshots`，保存 `all` / `appid:<正整数>` 总数快照；
- 为现有 `items` 增加六个可空目录列；
- 保留 `items(market_hash_name)` 单列主键，不在本轮重建；仓储写入前检测跨 appid 同名冲突并整页回滚。

迁移由 `schema_migrations(version=6)` 单次执行。SQL 中的 `ALTER TABLE ADD COLUMN` 不设计成脱离迁移器重复执行；重复启动由版本表防止二次应用。

## 事务与查询

每个实时页在同一事务中 upsert 已访问物品、upsert 页缓存、写入范围快照。失败时整页回滚，网络失败从不覆盖旧缓存。同一范围、同一毫秒的快照使用 `INSERT OR REPLACE`，避免重复响应触发唯一键失败。

页面读取直接反序列化 `result_json`，不会按 `total_count` 分配内存。缓存键包含 `query/appid/filters/sort/direction/currency/language/offset/limit`。缓存 JSON 上限为 2 MiB。

## 保留策略

`MarketCatalogRepository::prune`：

- 删除超过 30 天的目录页；
- 按 `last_accessed_at` 保留最多 5,000 页；
- 范围快照保留 180 天；
- 不触碰关注、提醒、持仓、库存和价格历史。

调用清理前仍应按现有发布流程备份数据库。

## 回滚与恢复

旧版应用会忽略新增表和可空列，可直接回滚应用二进制。需要完全物理回滚时，先备份，再经用户确认删除 `market_catalog_pages`、`market_scope_snapshots`；SQLite 不便安全移除 `items` 新列，因此不在回滚中重建 `items`。恢复使用升级前 `.bak`。

## 验证

`test_full_market` 覆盖迁移可见性、缓存维度隔离、实时页事务保存、缓存 origin/freshness、同毫秒快照去重和固定 JSON 严格解析。测试完全离线。
