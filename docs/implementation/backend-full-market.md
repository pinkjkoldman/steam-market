# Steam 全市场后端实现（CR-004 / CR-005）

## 交付范围

- `MarketCatalog.h` 定义冻结契约要求的查询、目录项、页面、范围快照、来源和稳定错误码；
- `SteamMarketCatalogClient` 使用独立匿名 `QNetworkAccessManager`，固定访问 `https://steamcommunity.com/market/search/render/`；
- `FullMarketService` 负责页缓存优先、远端刷新、缓存降级和 generation 取消；
- `MarketCatalogRepository` 负责规范化查询哈希、事务写入、TTL 和范围快照；
- 单元测试使用固定 JSON fixture，不访问网络。

## 请求边界

- 每次 `requestPage` 只对应一个用户可见的分页动作，不预取相邻页，也不按 `total_count` 循环；
- Steam 请求 `count` 固定为 10，页面大小以响应 `pagesize` 为准；
- 同一客户端至多一个在途请求，新请求取消旧请求；
- 请求最小间隔不低于 1.5 秒；429 尊重 `Retry-After`，无值时冷却 30 秒；
- 5xx/网络失败最多两次有界重试，分别至少等待 30/60 秒；403、重定向、非 JSON 和结构变化不自动重试；
- 客户端拥有拒绝读写 Cookie 的独立 CookieJar，不接入账户会话。

## 严格解析与错误

正文上限 2 MiB；校验 `success/start/pagesize/total_count/results`、页内数量、字段类型、长度及安全整数范围。任一目录项结构无效即拒绝整页，不提交部分结果。图标仅接受 HTTPS Steam CDN 主机，其他 URL 置空。

稳定错误码：`MARKET_RATE_LIMITED`、`MARKET_SCHEMA_CHANGED`、`MARKET_PAGE_OUT_OF_RANGE`、`MARKET_OFFLINE_NO_CACHE`、`MARKET_CURRENCY_UNSUPPORTED`。

## 调用接口

应用集成层需构造 `MarketCatalogRepository`、`SteamMarketCatalogClient`、`FullMarketService`，监听：

- `pageReady(MarketCatalogPage)`：可能先发缓存，再发实时页；
- `pageFailed(MarketCatalogError)`：无缓存或缓存写入失败；
- `requestStateChanged(bool)`：驱动加载状态。

`MarketCatalogPage::origin` 明确为 `kSteamLive` 或 `kSteamCached`；缓存页同时携带 `fetchedAt` 和 `stale`。

## 未决与集成

- 本模块不修改 `src/src.pro`、`tests/tests.pro`、`tests/main.cpp`；集成层需登记新增源文件、头文件和 `TestFullMarket`；
- 本轮不实现真实 Steam 网络自动测试，G3 集成阶段仅允许一次低频人工契约探针；
- `search/render` 是 Steam 官方网页内部端点，不承诺 Valve 第三方 API 稳定性。
