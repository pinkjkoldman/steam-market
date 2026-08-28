# CR-004 + CR-005 前端工作台实现（FE-WORKBENCH）

## 交付范围

- `WelcomePage`：游客/Steam 登录双模式卡，游客偏好信号，登录忙碌、不可用和错误状态。
- `AccountCard`：`ChoiceRequired`、`Guest`、`PublicInventory`、`Authenticating`、`Authenticated`、`Expired` 完整身份快照渲染；发出登录、注销和管理意图，不处理 Cookie。
- `MarketOverviewPage`：Steam 全站及重点游戏 `total_count`、热门目录页、游戏分布、快速预览、个人摘要、数据来源/查询时间/缓存/限流/错误状态。
- `MarketBrowserPage`：Steam 远端查询、游戏与排序参数、300 ms 防抖、Enter 立即搜索、固定 10 条请求分页、`x-y/total`、快速预览和键盘入口。
- 公共组件：`ModeCard`、`ScopeNotice`、`QuickInspector`、工作台只读视图模型与局部统一主题。

概览和浏览页不展示 Steam 契约未提供的全市场涨跌、成交额、活跃人数，也不把本地关注或缓存范围称为全市场。

## UI 接口

### 渲染输入

- `MarketCatalogPageView` 对齐 `offset/pageSize/totalCount/items/fetchedAt/origin/stale/sourceLabel`。
- `MarketScopeSnapshotView` 对齐 `scopeKey/totalCount/fetchedAt/origin`。
- `MarketUiStatus` 覆盖 `Initial/Loading/Ready/Empty/OfflineNoCache/RateLimited/SchemaChanged/Error`。
- `MarketInspectionView` 是单物品按需深度行情展示快照；没有深度字段时明确显示不可用。
- `AccountCard::setSnapshot(const IdentitySnapshot &)` 直接消费账户冻结契约快照。

### 用户意图信号

- 欢迎页：`guestRequested`、`loginRequested`、`rememberGuestChanged`。
- 概览：`scopeSummaryRequested`、`popularPageRequested`、`itemInspectRequested`、`itemActivated`、`browseAllRequested`、重试/官方市场/个人区动作。
- 浏览：`catalogRequested(query, appid, offset, 10, sort, currency)`、`itemInspectRequested`、`itemActivated`、关注/提醒、重试/官方市场动作。
- 账户卡：`loginRequested`、`logoutRequested`、`manageRequested`。

页面不创建网络客户端、不执行 Steam 请求。集成层负责把后端目录/错误模型转换为 UI 只读快照，并连接上述意图。

## 状态与交互

- 加载时保留现有表格；状态条说明 Steam 当前请求。
- 缓存页明确标注查询时间和 stale，不标为实时。
- 限流显示退避秒数，UI 不预取或自动循环分页。
- 无缓存离线、响应结构变化分别呈现，均提供恢复动作。
- 浏览页单击选择只更新预览；双击/Enter 发出分析意图。
- `/` 聚焦浏览搜索；Esc 清除预览；1040 px 以下收缩表格辅助列并限制预览高度。

## 构建与集成注意事项

新增源文件应由 APP-INTEGRATION 模块加入 `src/src.pro`。本模块未连接 `AppController`/`MainWindow`，也未修改服务、模型和冻结文档。

受当前 Windows 沙箱所有权元数据冲突影响，既有 `src/src.pro` 无法由本模块更新；总装配代理已明确接管。相同问题导致本轮只读 `g++ -fsyntax-only` 未能返回源码诊断，需在 APP-INTEGRATION 完成 qmake 后以全量构建作最终编译验证。

### Latest validation

Independent build validation supersedes the earlier sandbox note: a temporary qmake static-library project containing only this module's new UI files completed qmake, C++17 compilation, MOC, and archive successfully with Qt 6.11.0 / MinGW 13.1, with no compiler warnings. The temporary project did not modify repository build files.

The validation fixes cover complete `QStyle`/selection-model includes, `QHeaderView::hideSection/showSection`, and personal-summary string concatenation. APP-INTEGRATION still owns adding sources to `src/src.pro` and the final full-application build. No ACL was modified.

主题采用 `WorkbenchTheme` 集中生成 QSS，避免页面散落颜色。它是局部可独立使用的主题层，后续可由主应用统一 `ThemePalette` 替换。

## 库存到历史趋势交互（2026-08-18）

- 库存表支持双击物品直接进入详情页，也支持右键选择“查看历史趋势”；跳转参数直接使用同步结果中的 `appid` 与 `marketHashName`。
- 详情页沿用现有来源页返回语义，从库存进入后，返回按钮和 Esc 均回到库存助手。
- `PriceChart` 在价格走势与 24h 历史点中启用鼠标跟踪：吸附到离指针最近的真实数据点，显示十字线、数据点和时间/价格/成交量浮层。
- 悬停值不进行价格插值；成交量缺失时明确显示 `—`，避免把未知数据渲染成零成交。
- 浮层自动避让图表边界，单历史点与多历史点使用同一交互逻辑。
- 库存右键菜单、库存来源下拉框及详情区间下拉框使用高对比深色弹层，保证选项可读。

验证：Qt 6.11.0 / MinGW 13.1 Release 全量构建通过，现有单元测试全部通过；视觉冒烟截图见 `docs/test`。

## 库存对表重设计（2026-08-18）

库存列表改为八列：`选择 / 物品 / 类型 / 持有数量 / 交易状态 / 市场出售 / 计划出售 / 历史趋势`。

- `交易状态`直接使用 Steam 库存描述中的 `tradable`，显示“可交易/不可交易”。
- `市场出售`直接使用 `marketable`，显示“可出售/不可出售”，不再使用含义模糊的“可交接”。
- 同步结果显示完整库存分组，不再只返回可出售项；不可出售项保留可见，但选择框与出售数量不可用。
- 批量选择和“出售重复项”只作用于 `marketable=true` 的可见行，服务层创建草稿前仍执行数量与市场名校验。
- `历史趋势`列明确显示“双击查看/无市场页”，右键菜单在缺少市场名称时禁用。
- 表格底部同时显示可见分组数与其中可出售分组数，完成同步后显示总分组数与可出售分组数。

## 界面重叠与高 DPI 修复（2026-08-18）

- 全局主题为普通标签、复选框、单选框和行内状态补齐深色界面文字色，避免系统默认黑色字与背景混在一起。
- `QGroupBox` 标题使用独立上边距、标题子控件位置和内容内边距，交易规则与费用计算器的标题不再压住表单。
- 概览、物品浏览、交易规则、库存和设置页接入纵向滚动容器，在 `1040x680` 和缩放场景下保留完整操作入口。
- 快速预览的长物品名改为根据容器宽度单行省略，完整名称和 `market_hash_name` 保留在悬浮提示中，不再与游戏名重叠。
- 规则浏览器刷新后回到文档顶部，小尺寸下隐藏非关键筛选提示，避免与搜索控件挤压。

验证：Qt 6.11.0 / MinGW 13.1 Release 构建与全部单元测试通过；11 个界面在 `1040x680` 和 `1280x800` 两种尺寸下完成 22 张截图回归。关键证据见 `docs/test/smoke-ui-overlap-fixed-*.png`。

## 新版盘口与单品市场操作（2026-08-18）

- 盘口请求由已失效的 `itemordershistogram` 迁移到 Steam 新版官方 `/market/orderbook` `queryAction` 协议，不再依赖 `item_nameid`；解析响应中的紧凑买卖档位和 `eCurrency`。
- 盘口增加买卖价差、价差率、前五档买卖深度、盘口失衡率，并显示数据来源、响应币种和抓取时间。
- 库存当前行增加“上架到 Steam 市场”和“选择交易用户”；右键菜单提供相同操作，双击仍进入历史趋势。
- 社区物品单品上架交接到 Steam 官方 multisell 页面；其他游戏物品打开本人官方库存并定位资产。应用不自动填写价格、不提交交易，也不代替 Steam Guard 确认。
- 交易对象支持 SteamID64、Steam 官方交易报价链接或留空后在官方页面选择。链接执行精确 HTTPS 主机/路径及 partner/token 校验，token 仅用于本次跳转且不持久化。
- 公开库存模式保持只读；所有上架和用户交易入口都要求 Steam 官方已认证会话。
- 批量上架价格、已选组数/件数、费用摘要和主按钮提升到库存表上方首屏操作区；按钮明确显示“批量上架已选（N 件）”，禁用提示区分未登录与未勾选，不再把入口隐藏在表格下方。

验证：Release 全量构建通过，全部单元测试通过；盘口协议、响应解析、官方 URL 生成与恶意主机拒绝均有回归测试。库存页和详情页在 `1040x680`、`1280x800` 完成截图检查，批量入口另有首屏可见性冒烟断言，未发现新增文字或组件重叠。

## 市场筛选工作台与直观指标（2026-08-18）

物品浏览页按数据能力拆分为两级筛选，避免把局部结果误称为全市场筛选：

- 远端筛选：名称或 `market_hash_name` 关键词、游戏范围、热门/价格/名称排序，直接映射到 Steam 市场搜索分页。
- 快速定位：提供 CS2 武器箱、印花、刀具，Dota 2 Immortal 和 Steam 交易卡牌预设，一次设置游戏与关键词。
- 当前页二次筛选：类型包含、最低/最高价格、最低挂单量和仅看有报价；输入变化立即生效，不额外请求 Steam。
- 筛选摘要持续显示远端条件、当前页条件和可见条数；分页区域同时区分远端范围与当前页显示数量。
- 四项直观指标：远端命中、当前页显示、可见价格中位数、可见挂单总量。
- 列表增加“流动性”分级：挂单量 `≥1000` 为高、`≥100` 为中、其余有挂单为低，无挂单为无；该分级只表示当前挂单供给，不代表成交速度。
- 选中物品后继续使用右侧快速预览，可直接进入价格分析、关注或设置提醒。

筛选计算由无界面的 `MarketPageFilterEngine` 承担，组合条件、偶数样本中位数和挂单量汇总均有单元测试。筛选布局采用分行结构，在 `1040x680`、`1280x800` 和 `1920x1080` 下不会因横向控件过多裁切搜索或指标卡。
