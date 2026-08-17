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

