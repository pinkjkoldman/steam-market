# Steam Community Market 全市场数据能力核查

- 核查日期：2026-08-13（Asia/Shanghai）
- 核查范围：仅 Steam / Valve 一手页面、Steamworks 官方文档、Steam 官方端点的低频即时响应
- 目的：判断 Steam Market Terminal 能否把“市场概览”定义为“来自 Steam 官方 API 的 Steam 全市场数据”
- 结论等级：产品与工程决策依据；不构成法律意见

## 1. 执行结论

### 1.1 可以确认的事实

1. **没有发现面向普通第三方桌面客户端、可公开枚举 Steam Community Market 全部物品或全部 listing 的正式 Steam Web API。** Steamworks Web API Reference 自称列出当前受支持的全部 Web API；其中市场接口 `IEconMarketService` 被 Valve 明确定义为“为合作伙伴提供受限市场访问”，其方法需要带 Economy 权限的 publisher key，并明确要求从安全服务器调用、不得直接由客户端调用。[Steamworks Web API Reference](https://partner.steamgames.com/doc/webapi?language=english)；[IEconMarketService](https://partner.steamgames.com/doc/webapi/IEconMarketService)
2. Steam Community Market 网站自身存在可返回 JSON 的网页端点，例如 `search/render`、`priceoverview`、`pricehistory`、`itemordershistogram`。**它们位于 `steamcommunity.com/market/...`，不在 Steamworks 文档规定的公共 Web API 主机与 URI 契约 `api.steampowered.com/<interface>/<method>/v<version>/` 中。** 因此应称为“Steam 官方网页内部端点”或“社区市场网页端点”，不能称为受支持的 Steam Web API。[Web API Overview](https://partner.steamgames.com/doc/webapi_overview?language=english)
3. 低频匿名实测显示，`search/render` 当前能分页返回全站或按 `appid` 筛选的“可搜索市场结果”，并给出瞬时 `total_count`；但一次请求把 `count=100` 传入后，响应仍为 `pagesize=10` 且只返回 10 条。该行为没有正式文档保证。
4. Valve 当前 Steam Subscriber Agreement 对使用脚本、机器人、宏或其他非人工控制系统与 Steam Content and Services 交互作了广泛禁止；另禁止未经授权的软件修改 Subscription Marketplace 流程或 Steam UI/交互过程。[Steam Subscriber Agreement，第 4.B–4.C 节](https://store.steampowered.com/subscriber_agreement/english/?l=english)

### 1.2 对项目的直接判断

用户希望“市场概览就是 Steam 全市场数据，信息来自 Steam 的 API”，产品意图可以保留，但现阶段不能把它承诺为：

- 由 Valve 正式支持的公开全市场 API 提供；
- 可持续、无上限地抓取全部约 52.8 万条结果；
- 具备官方稳定性、SLA、固定字段或固定分页规模；
- 获得 Valve 对自动化采集的许可。

如果没有 Valve 的书面授权或适用的 Steamworks 合作伙伴权限，建议产品文字改为：

> **Steam 市场全局概览（Steam 官方市场公开页面数据）**  
> 覆盖 Steam Community Market 当前可搜索目录；数据为缓存快照，可能受 Steam 可用性、分页、地区、货币与访问限制影响。此功能不是 Valve 提供的正式全市场 API 服务。

## 2. “公开 Web API”与“网页内部端点”的边界

### 2.1 Steamworks 官方 Web API

**事实：** Valve 的 Web API Overview 规定：公共 Web API 访问 `api.steampowered.com`，合作伙伴专用接口访问 `partner.steam-api.com`；后者要求 publisher Web API key。官方 Reference 表示其页面是“当前支持的 Web API 接口和方法的完整列表”。[Web API Overview](https://partner.steamgames.com/doc/webapi_overview?language=english)；[Web API Reference](https://partner.steamgames.com/doc/webapi?language=english)

与市场最接近的正式接口：

| 正式接口 | 官方能力 | 调用边界 | 能否供本桌面客户端枚举全市场 |
|---|---|---|---|
| `IEconMarketService.GetPopular` | 获取热门物品；参数含 `rows`、`start`、`filter_appid`、`ecurrency` | publisher key + Economy 权限；安全服务器调用；禁止直接由客户端调用 | 否。它是合作伙伴受限接口，且文档只承诺“热门物品”，不是全量枚举 |
| `IEconMarketService.GetMarketEligibility` | 检查某账户是否允许使用市场 | 同上 | 否 |
| `ISteamEconomy.GetMarketPrices` | 取得某 Steam Economy app 的市场价格 | publisher key；安全服务器；`appid` 必须是关联的 economy app | 否。是发行商自己的应用域，不是跨 Steam 全市场公共目录 |
| `ISteamEconomy.GetAssetPrices` | 返回用户可购买物品的价格和分类 | 需要 Web API user key、指定 economy `appid` | 否。不是 Community Market 全站 listing 枚举 |

来源：[IEconMarketService](https://partner.steamgames.com/doc/webapi/IEconMarketService)；[ISteamEconomy](https://partner.steamgames.com/doc/webapi/isteameconomy?language=english)

**推断：** 即使项目未来拥有 Steamworks publisher key，也不能据此假定可访问其他发行商的全部市场数据；官方接口的 `appid`、publisher group 与 Economy 权限形成明确边界。

### 2.2 Steam Community Market 网页端点

**事实：** 以下端点由 `steamcommunity.com` 官方站点实际提供，并被 Market 网页使用或可直接响应，但未出现在 Steamworks Web API Reference 中：

- `https://steamcommunity.com/market/search/render/`
- `https://steamcommunity.com/market/priceoverview/`
- `https://steamcommunity.com/market/pricehistory/`
- `https://steamcommunity.com/market/itemordershistogram`

**结论：** “由 Steam 官方域名返回”是真的；“是 Valve 面向第三方发布并支持的 API”没有官方依据。公开可访问也不等于获得自动化采集许可。

## 3. 官方网页端点逐项核查

### 3.1 `market/search/render`

示例：

```text
GET https://steamcommunity.com/market/search/render/
  ?query=
  &start=0
  &count=10
  &search_descriptions=0
  &sort_column=popular
  &sort_dir=desc
  &norender=1
```

#### 实测事实

- 匿名请求返回 HTTP 200、`application/json`。
- 响应顶层观测到：`success`、`start`、`pagesize`、`total_count`、`searchdata`、`results`。
- 每个结果观测到：`name`、`hash_name`、`sell_listings`、`sell_price`、`sell_price_text`、`sale_price_text`、`app_name`、`app_icon`、`asset_description`。
- `asset_description` 观测到：`appid`、`classid`、`market_name`、`market_hash_name`、`commodity`、图标和显示属性等。
- `start=10` 返回下一页且响应 `start=10`，说明 `start` 当前作为结果偏移量生效。
- `appid=730` 当前能按应用筛选。
- 实测传入 `count=100` 时，响应仍是 `pagesize=10`、`results.length=10`。不能沿用旧资料中“最大 100”的假设。
- `norender=1` 返回结构化 `results`；网页本身也提供“Showing 1–10 of N results”。[Steam Community Market Search](https://steamcommunity.com/market/search?l=english)

#### 2026-08-13 瞬时计数

以下都是低频请求返回的瞬时值，不是固定总量，刷新期间已观察到小幅变化：

| 范围 | `total_count` |
|---|---:|
| 全站，不设 `appid` | 527,988（本代理相邻请求观测 527,985–527,987） |
| Counter-Strike 2，`appid=730` | 35,237 |
| Dota 2，`appid=570` | 33,703 |
| Team Fortress 2，`appid=440` | 41,314 |
| Steam Community Items，`appid=753` | 321,575 |

#### 参数性质

| 参数 | 当前观察 | 契约状态 |
|---|---|---|
| `query` | 搜索词；空串可浏览全部当前可搜索结果 | 未正式文档化 |
| `start` | 零基结果偏移量 | 实测生效，未正式文档化 |
| `count` | 请求页大小 | 实测 `100` 仍只返回 `10`；服务端可忽略/限制 |
| `appid` | 按游戏/应用过滤 | 实测生效 |
| `search_descriptions` | 是否搜索描述 | 从网页查询形式观察，未单独验证语义 |
| `sort_column` / `sort_dir` | 排序列与方向 | 从网页查询形式观察，具体枚举不受官方契约保证 |
| `norender` | 请求结构化 JSON 结果 | 实测 `1` 可用 |

#### 限制与工程推断

- `total_count` 表示当前搜索条件的结果总数，不等同于唯一资产数量、在售 listing 总数或成交总数。
- 当前 10 条/页，完整扫描 527,988 个搜索结果约需 **52,799 次请求**。这是算术推断，不是 Valve 建议的访问方式。
- `sell_listings` 是结果级在售数量；不能把所有页面的 `sell_listings` 简单表述为 Steam 官方发布的“市场总成交量”。
- 响应内容、字段和页大小可能随 Market beta/UI 改版改变；官方 Market 页面当前明确提示正在测试新的 item pages 和 advanced search。[Steam Community Market](https://steamcommunity.com/market/)

### 3.2 `market/priceoverview`

示例：

```text
GET https://steamcommunity.com/market/priceoverview/
  ?appid=730
  &currency=23
  &market_hash_name=Dreams%20%26%20Nightmares%20Case
```

#### 实测事实

- 匿名请求返回 HTTP 200、JSON。
- 本次响应为：`success=true`，并有本地化的 `lowest_price`、`volume`、`median_price`。
- 参数 `appid` 与 `market_hash_name` 共同标识物品；`currency` 影响返回币种。`country` 常见于网页请求，但本次未证明其为必需。
- 返回的是单个物品的摘要，不提供物品枚举、分页或全市场汇总。

#### 限制

- 价格文本包含货币符号与本地化格式；业务层应保留 currency/locale 上下文，不能只解析显示字符串后假设币种。
- `volume` 的统计窗口未在正式 Steamworks 文档中定义；不能在 UI 中擅自写成“24h 成交量”，除非另有可验证依据。
- 该端点没有官方第三方 API 契约、配额或稳定性承诺。

可直接核查：[官方 `priceoverview` 响应](https://steamcommunity.com/market/priceoverview/?appid=730&currency=23&market_hash_name=Dreams%20%26%20Nightmares%20Case)

### 3.3 `market/pricehistory`

示例：

```text
GET https://steamcommunity.com/market/pricehistory/
  ?country=US
  &currency=1
  &appid=730
  &market_hash_name=P250%20%7C%20Red%20Rock%20%28Battle-Scarred%29
```

#### 实测事实

- 2026-08-13 无登录 Cookie 的低频请求返回 HTTP 400。
- Steam 官方 listing 页面确实展示“Median Sale Prices”的 Week / Month / Year / Lifetime 图表区，因此 Steam 网页产品具备历史展示能力。[官方 listing 页面示例](https://steamcommunity.com/market/listings/730/Dreams%20%26%20Nightmares%20Case)

#### 认证判断

- **能确认：** 本次匿名直连不可用。
- **不能从官方文档确认：** Valve 没有为该网页端点发布正式认证说明、参数契约或错误码说明。
- **工程推断：** 实际网页调用可能依赖有效 Steam Community 会话、区域/货币上下文或正在变化的新 Market 页面流程。实现不得把 400 自动解释成“物品不存在”，也不得靠持久化用户 Cookie 做后台全市场采集。

#### 预期数据性质

旧网页行为通常返回 `success` 与 `prices` 时间序列，但这不是当前 Steamworks 正式文档中的保证。本项目若使用它，必须通过登录态实测、契约探针与显式降级确认，不能把旧返回结构当固定协议。

### 3.4 `market/itemordershistogram`

示例：

```text
GET https://steamcommunity.com/market/itemordershistogram
  ?norender=1
  &country=US
  &language=english
  &currency=1
  &item_nameid=175896332
  &two_factor=0
```

#### 实测事实

- 匿名低频请求返回 HTTP 200、JSON。
- 不带 `norender=1` 时，观测到 `sell_order_table` / `buy_order_table` 为 HTML 片段，并有摘要与图数据。
- 带 `norender=1` 时，观测到结构化字段：`sell_order_count`、`sell_order_price`、`sell_order_table` 数组、`buy_order_count`、`buy_order_price`、`buy_order_table` 数组、`highest_buy_order`、`lowest_sell_order`、`buy_order_graph`，以及后续卖单图数据。
- `item_nameid` 是站内数字标识，不是公开 Steamworks API 中正式定义的 `market_hash_name` 替代物。

#### 参数与限制

| 参数 | 当前观察 |
|---|---|
| `item_nameid` | 必需的网页内部数字标识；新 Market 页面可能不再在初始 HTML 中直接暴露旧 `Market_LoadOrderSpread(...)` 形式 |
| `country` | 国家/地区上下文 |
| `language` | 返回文本与 HTML 的语言 |
| `currency` | 价格币种 |
| `two_factor` | 网页内部参数；没有正式第三方契约解释 |
| `norender=1` | 实测可得到更结构化的 table 数组，而非主要为 HTML 字符串 |

- 这是单品订单簿快照，不负责枚举全市场。
- 最优买卖价以整数最小货币单位和本地化文本混合返回；须携带币种解释。
- 图数据是当时累计订单分布，不是成交历史。
- 无官方第三方速率上限或 SLA 文档；任何经验性“每分钟 N 次”都不能当作 Valve 授权。

可直接核查：[官方 `itemordershistogram` 响应](https://steamcommunity.com/market/itemordershistogram?norender=1&country=US&language=english&currency=1&item_nameid=175896332&two_factor=0)

## 4. 分页、全量性与“全市场”的准确含义

### 4.1 当前能得到什么

`search/render` 的空查询可以在当前网站状态下返回跨应用的可搜索结果数与分页结果，因此产品可以把它作为“Steam Community Market 可搜索目录的官方来源”。每条结果还可提供物品名、应用、当前在售数量、一个当前价格字段和图标引用。

### 4.2 不能声称什么

- 不能声称这是 Steam 所有虚拟资产；它是 Community Market 当前可搜索、可展示的 marketable 结果集合。
- 不能声称每次刷新都得到原子一致的全市场快照；扫描过程中总数、价格和 listing 数持续变化。
- 不能声称覆盖所有逐条卖单。可互换商品的官方页面明确说明单个 listing 不可访问，而是由买单/最低卖单自动匹配。[官方 commodity listing 示例](https://steamcommunity.com/market/listings/730/Dreams%20%26%20Nightmares%20Case)
- 不能仅凭 `priceoverview.volume` 构造官方“全市场日成交额”或“全市场指数”，因为时间窗口和汇总口径未正式定义。

### 4.3 规模推断

按本次全站约 527,988 个结果和实测每页 10 条：

```text
ceil(527,988 / 10) = 52,799 个分页请求
```

之后如再为每个物品请求 `priceoverview`、历史或订单簿，请求规模会再成倍增加。这种方案不适合桌面客户端启动即执行，也没有获得 Valve 自动化许可。

## 5. 认证与会话边界

| 端点 | 本次匿名实测 | 是否为正式 Web API | 建议 |
|---|---|---|---|
| `search/render` | 200 JSON | 否 | 仅低频、按需、强缓存；准备随网页改版降级 |
| `priceoverview` | 200 JSON | 否 | 单品按需；不要高频轮询全目录 |
| `pricehistory` | 400 | 否 | 视为会话/契约受限能力；登录后也只能按用户动作加载并可降级 |
| `itemordershistogram` | 200 JSON | 否 | 单品详情按需；`item_nameid` 解析失败时隐藏订单簿 |
| `IEconMarketService.*` | 未以无权密钥调用 | 是，partner-only | 只有获得相应 publisher/Economy 权限且使用安全后端时采用 |

**安全事实：** Valve 对 publisher key 明确要求安全服务器调用，并写明“can never be used directly by clients”。所以即使未来获得密钥，也不得嵌入 Qt 桌面程序。[IEconMarketService](https://partner.steamgames.com/doc/webapi/IEconMarketService)

## 6. robots、条款与速率限制

### 6.1 robots.txt

2026-08-13 实测 [steamcommunity.com/robots.txt](https://steamcommunity.com/robots.txt) 返回：

```text
Host: steamcommunity.com
User-agent: *
Disallow: /actions/
Disallow: /linkfilter/
Disallow: /tradeoffer/
Disallow: /trade/
Disallow: /email/
```

`/market/` 当前没有出现在 `Disallow` 中。

**边界：** robots.txt 是爬虫抓取提示，不是 API 授权、数据许可、服务契约或对 Subscriber Agreement 的豁免。

### 6.2 Steam Subscriber Agreement

**官方事实：** 当前协议第 4.C 节禁止使用 scripts、bots、macros 或其他非人工控制系统与 Steam Content and Services 进行交互；第 4.B 节也禁止未经授权的软件修改 Subscription Marketplace 流程或 Steam 的交互/UI 流程。[Steam Subscriber Agreement](https://store.steampowered.com/subscriber_agreement/english/?l=english)

**工程/合规推断：** 自动遍历五万余页、周期性刷新并长期构建全市场镜像，存在显著条款风险。即使匿名 GET 技术上返回 200，也不能据此推断 Valve 允许这种采集。若产品必须做完整市场镜像，应先取得 Valve 的书面许可或正式合作接口。

### 6.3 速率限制

- Steamworks 正式 Web API 文档定义 HTTP 429 为“Too Many Requests”，403 失败请求在 partner host 上还会触发严格 IP 限流。[Error Codes & Responses](https://partner.steamgames.com/doc/webapi_overview/responses)；[Web API Overview](https://partner.steamgames.com/doc/webapi_overview?language=english)
- Valve 没有在上述官方资料中公布 Community Market 网页端点的可用请求频率、每日配额或批量抓取额度。
- 因此不能把社区经验值或“尚未收到 429”当作许可。客户端必须识别 429/403/5xx，指数退避、停止后台扫描并展示陈旧时间。

## 7. 对 G2 设计的建议

### 7.1 建议接受的产品目标

将市场概览设计为“跨 Steam Community Market 应用的全局视角”，数据字段全部来自 Steam 官方市场网页响应；界面明确数据覆盖与更新时间。

推荐概览内容：

- 当前可搜索市场物品数（`search/render.total_count`，标记为快照）；
- 按游戏分布（对明确选择或有限的热门 app 查询，不宣称一次获得完整官方聚合）；
- 当前热门/高在售物品（`sort_column=popular` 的网页排序结果，标题用“Steam 市场热门排序”，不自行定义为涨幅榜）；
- 单品最低售价与在售量；
- 用户点击后按需加载单品订单簿；
- 历史数据不可用时明确显示“Steam 当前未向此会话提供历史数据”。

### 7.2 不建议在未授权状态下实施

- 首次启动或定时扫描全部 52,799 页；
- 对 50 万级物品逐个请求 `priceoverview`、历史和订单簿；
- 使用登录 Cookie 做无人值守后台采集；
- 绕过 429/403、验证码、会话要求或网页风控；
- 在产品文字中使用“Steam 官方全市场 API”“Valve 授权数据”“实时全量”等未经支持的表述。

### 7.3 可落地的分级方案

1. **G3 可实现：全局入口 + 当前目录元数据。** 用户打开页面时只取首页、`total_count` 和少量指定 app 的计数，缓存并展示来源、币种与时间。
2. **按需浏览：服务端分页即用户分页。** 用户滚动/翻页才请求下一页，禁止后台预抓全目录。
3. **按需详情：** 用户选中物品后才请求 `priceoverview` 和订单簿；历史图需有效会话且必须可降级。
4. **正式全量能力门禁：** 只有取得 Valve 书面许可或可用的 partner 接口后，才设计安全后端、全量同步任务与数据库快照。

## 8. 证据分类和局限

### 官方明文事实

- Steamworks Web API 的主机、URI、认证和完整 Reference 声明。
- `IEconMarketService` 是 partner restricted，publisher key 不得放入客户端。
- Steam Subscriber Agreement 的自动化限制。
- 官方市场网页显示的搜索总数、商品页、订单深度和历史图区域。
- robots.txt 当前内容。

### 2026-08-13 低频实测

- `search/render`、`priceoverview`、`itemordershistogram` 的匿名状态码与 JSON 字段。
- `pricehistory` 匿名请求为 HTTP 400。
- `count=100` 被响应收敛为 10 条。
- `total_count` 的全站及部分 app 瞬时值。

### 推断

- 五万余次分页扫描不适合作为客户端刷新方案。
- 全量自动镜像存在明显协议风险，应要求 Valve 许可。
- 网页内部端点随 Market beta 改版发生字段、身份与页面大小变化的风险很高。

### 本次未证明

- 登录后 `pricehistory` 的当前完整返回结构及其长期稳定性；
- Valve 对 Community Market 网页端点的明确数值速率上限；
- Steam 是否会向本项目授予全市场合作权限；
- `total_count` 是否在所有地区、语言、账号状态下完全一致。

## 9. 官方来源索引

1. [Steamworks Web API Reference](https://partner.steamgames.com/doc/webapi?language=english)
2. [Steamworks Web API Overview](https://partner.steamgames.com/doc/webapi_overview?language=english)
3. [IEconMarketService](https://partner.steamgames.com/doc/webapi/IEconMarketService)
4. [ISteamEconomy](https://partner.steamgames.com/doc/webapi/isteameconomy?language=english)
5. [Web API Error Codes & Responses](https://partner.steamgames.com/doc/webapi_overview/responses)
6. [Steam Community Market](https://steamcommunity.com/market/)
7. [Steam Community Market Search](https://steamcommunity.com/market/search?l=english)
8. [官方商品页示例](https://steamcommunity.com/market/listings/730/Dreams%20%26%20Nightmares%20Case)
9. [官方 `search/render` 示例响应](https://steamcommunity.com/market/search/render/?query=&start=0&count=10&search_descriptions=0&sort_column=popular&sort_dir=desc&norender=1)
10. [官方 `priceoverview` 示例响应](https://steamcommunity.com/market/priceoverview/?appid=730&currency=23&market_hash_name=Dreams%20%26%20Nightmares%20Case)
11. [官方 `itemordershistogram` 示例响应](https://steamcommunity.com/market/itemordershistogram?norender=1&country=US&language=english&currency=1&item_nameid=175896332&two_factor=0)
12. [steamcommunity.com robots.txt](https://steamcommunity.com/robots.txt)
13. [Steam Subscriber Agreement](https://store.steampowered.com/subscriber_agreement/english/?l=english)
