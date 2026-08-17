# CR-003 后端实现

## 库存读取

- `SteamInventoryClient` 只调用 Steam 社区库存 GET 接口，支持 app/context、`count=5000`、`start_assetid`、32 MB 响应上限和 429 分类。
- `InventoryParser` 将 assets 与 descriptions 正确关联，识别交易卡牌分类，并解析 `more_items/last_assetid`。
- `InventoryService` 最多读取 50 页，页间隔 2 秒，发布进度、完成和失败事件。

## 会话与安全

- `WebView2SessionHost` 仅嵌入 Steam 官方 HTTPS 页面，限制顶层导航域名，禁用开发者工具、默认右键菜单与状态栏。
- 应用不接收或保存 Steam 密码；只从 WebView2 CookieManager 向内存会话桥接必要 Cookie。
- 支持公开 SteamID64 只读库存模式；私有库存需要官方页面登录。
- 已移除编译路径中的旧 RSA 登录、手工 Cookie、市场 POST 和自动执行交易能力。

## 草稿与交接

- `PricingDraftService` 生成整数金额固定价草稿，并给出费用估算。
- `MultiSellHandoffService` 按最多 40 组和 URL 1800 字节分批，目标仅为 Steam 官方 `market/multisell`。
- 最终价格、物品和提交动作必须由用户在 Steam 官方页面确认。
