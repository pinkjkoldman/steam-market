# ADR-005：类型化单品按需历史模块

- 状态：拟批准（CR-006 G2）
- 日期：2026-08-17
- 决策者：系统架构师、数据库工程师、安全工程师、UI/UX 设计师、总设计师

## 背景

现有 `MarketService::detailReady` 同时返回概览、历史向量和字符串错误，无法区分在线、缓存、认证、限流、真实空数据与响应结构变化。`PriceChart` 又无法得知来源和数据口径，导致“画出了线”不等于“可信的真实走势”。

## 候选方案

1. 扩展现有信号，继续增加布尔值和字符串；
2. 建立 `MarketHistoryService + HistoryDataset + PriceHistoryRepository + SteamHistoryParser` 深模块；
3. 使用 WebView DOM/JavaScript 直接复用 Steam 图表。

## 决策

采用方案 2：

- 以 `HistoryKey(marketHashName, appid, currency)` 作为缓存和并发边界；
- 历史结果使用类型化状态与 provenance；
- 网络传输、纯解析、缓存编排、持久化和 UI 渲染分层；
- 只允许用户主动打开单品/刷新触发；
- 登录由 AppController 编排，历史服务只消费身份快照并执行一次性重试。

## 理由

- 将易变的 Steam 网页响应封装在 Parser/Client 后方，UI 与数据库不依赖原始字段；
- 错误、缓存和来源成为契约的一部分，可测试且不会被字符串解析；
- 认证模块继续保持单一事实来源，避免历史服务自行管理 Cookie；
- 比 DOM 注入更安全、更可维护；比继续扩展旧信号更能降低后续复杂度。

## 影响

- G3 新增核心模型、Parser、HistoryService 和 HistoryRepository；
- `MarketService` 移除历史编排，保留搜索与概览；
- `DetailPage` 改为分别消费概览与历史状态；
- SQLite 升级 v7 并保存 provenance/sync state；
- 真实联网测试是 G4 Must，fixture 不能替代。

## 回退

若新模块在线读取失败，产品仍可显示 v7 中可追溯缓存；若 v7 迁移失败，使用升级前备份恢复 v6。不得回退为演示点或 DOM 抓取。

