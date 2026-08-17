# CR-003 数据库实现

## 迁移

数据库版本从 v4 升级至 v5，迁移文件为 `src/data/migrations/0005_v4_inventory_assistant.sql`，已在资源文件和 `DatabaseManager` 中注册。

新增表：

- `steam_accounts`：账户标识与公开/已认证模式状态，不保存密码。
- `inventory_syncs`：库存同步批次、分页进度与失败原因。
- `inventory_descriptions`：以 appid/classid/instanceid 去重的物品描述。
- `inventory_assets`：以账户、appid、contextid、assetid 标识的库存资产。
- `listing_drafts`、`listing_draft_items`：本地定价草稿。
- `handoff_batches`：官方批量出售页面交接批次审计。

## 一致性策略

- 分页期间写入同步批次；只有全部分页成功后才在事务内删除旧快照并完成切换。
- 网络失败、解析失败或限流时保留上一份完整库存。
- 金额使用整数最小货币单位，避免浮点误差。
- 描述与资产按 `classid + instanceid` 关联，避免同名物品误合并。

## 回滚

迁移为加法迁移，不修改 v4 业务表。应用版本回滚时优先恢复升级前备份；若仅回退程序，新表可保留且旧版本不会访问。生产数据不执行自动 DROP，避免不可恢复删除。
