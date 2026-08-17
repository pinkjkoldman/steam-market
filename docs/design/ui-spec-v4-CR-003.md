# v4 UI 规范与组件契约（CR-003）

## 1. 桌面布局

设计基准：1440×900；最小窗口：960×640；建议默认：1280×800。

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│ Steam 市场终端   库存助手        [● 已登录：玩家名] [同步 12:30] [重新同步] │
├──────────┬───────────────────────────────────────────┬───────────────────────┤
│ 行情     │ 我的库存  128组 / 346件                 │ 批量定价              │
│ 自选     │ [游戏▼][类型:交易卡牌▼][可售✓][筛选__] │ 已选 42组 / 91件      │
│ 持仓     │ [全选可售] [每种保留 1 张] [清除]       │ ○ 固定价              │
│ 排行榜   ├───────────────────────────────────────────┤ ● 最低售价减 0.01     │
│ 交易规则 │ ✓ 图标 物品             数量 可售 建议价 │ ○ 最高求购价          │
│ 库存助手 │ □ [图] Game A - Card 1   3    2   ¥0.32 │                       │
│ 设置     │ □ [图] Game B - Card 2   6    5   ¥0.28 │ [生成价格草稿]        │
│          │ ...                                       │ ───────────────────   │
│          │                                           │ 买家支付  ¥28.40      │
│          │                                           │ 手续费     ¥4.26      │
│          │                                           │ 预计到手  ¥24.14      │
│          │                                           │ 异常       3项        │
│          │                                           │ [复核草稿]            │
├──────────┴───────────────────────────────────────────┴───────────────────────┤
│ 同步任务：753/6 第2/3页 · 429等待 00:24 · [任务与错误 2]                    │
└──────────────────────────────────────────────────────────────────────────────┘
```

尺寸：导航 176px；筛选/列表主区最小 560px；定价面板 320px；顶部 56px；任务栏 36px。窗口 <1180px 时定价面板折叠为右侧抽屉；<1040px 时导航折叠为 56px 图标栏。

## 2. 登录空态

```text
                         [Steam 图形占位]
                    登录后直接查看你的库存
          卡牌、游戏物品将自动加载，无需逐个搜索
                  [ 通过 Steam 官方页面登录 ]
        应用不会读取或保存密码 · 支持 Steam Guard
                  [仅查看公开库存：输入SteamID]
```

“公开库存模式”为次级入口；私密/仅好友库存失败后引导登录，不引导用户修改隐私设置。

## 3. 官方登录窗口

- 原生标题栏：`Steam 官方登录`；
- 原生安全栏：锁图标 + 完整主机名，例如 `login.steampowered.com`；
- 内容区：WebView2；
- 底部提示：“密码和 Steam Guard 直接提交给 Steam，本应用不会保存”；
- 关闭按钮返回未登录状态；会话验证成功后自动关闭；
- 禁止隐藏地址、全屏或无边框。

## 4. 库存表

列契约：

| 列 | 宽度 | 内容 |
| --- | --- | --- |
| 选择 | 44 | 复选框；不可售时禁用并有原因 tooltip |
| 物品 | stretch | 40px 图标、显示名、游戏/类型次级行 |
| 库存 | 64 | 聚合数量 |
| 可售 | 64 | 可进入草稿的数量 |
| 最低售价 | 88 | 金额 + 更新时间 tooltip |
| 最高求购 | 88 | 金额 + 更新时间 tooltip |
| 建议价 | 88 | 草稿生成后显示；异常为“—” |
| 状态 | 104 | 可售/保护至日期/无行情/缓存 |

默认按类型、游戏、名称排序。普通卡/闪卡使用文本角标区分，不能只靠图标颜色。批量选择作用于当前筛选结果，执行前在按钮旁显示预计资产数。

## 5. 定价面板

组件：

- 选择汇总：物品组数、资产数；
- 保留规则：每种保留 0–999 张；
- 策略单选：固定买家支付价、最低在售价减 N 个最小价格档、最高求购价；
- 最低到手价：可选；低于时排除；
- 费用摘要：买家支付、Steam/游戏费、预计到手；
- 异常摘要：点击展开 `DraftIssuesDrawer`；
- 主按钮：`生成价格草稿` → `复核草稿` → `在 Steam 中复核并上架`；每步独立，不合并成一键。

## 6. 草稿复核层

```text
← 返回库存          批量上架草稿 #A1B2       库存同步于 2分钟前
| 物品 | 数量 | 买家支付/件 | 手续费 | 到手/件 | 小计 | 状态 |
----------------------------------------------------------------
异常项目（3） [展开]
批次：753/6 共2批 · 730/2 共1批
                                      [保存草稿] [打开 Steam 第1批]
```

库存超过 5 分钟时主按钮禁用，显示“重新同步后继续”。打开一批后显示“已交接到 Steam，等待你在官方页面操作”，并提供“打开下一批”，不显示完成勾选。

## 7. 任务与错误中心

错误分类及文案：

| 错误 | 主文案 | 操作 |
| --- | --- | --- |
| SESSION_EXPIRED | Steam 登录已过期 | 重新登录 |
| INVENTORY_PRIVATE | 当前库存不可访问 | 使用官方登录 |
| RATE_LIMITED | Steam 请求过于频繁，将在 N 秒后可重试 | 等待/取消 |
| PARSE_ERROR | Steam 返回格式发生变化 | 查看诊断编号/使用缓存 |
| CURSOR_STALLED | 库存分页没有继续前进 | 从当前上下文重试 |
| WEBVIEW_RUNTIME_MISSING | 缺少 WebView2 Runtime | 打开微软官方下载页 |

错误 UI 不显示 URL 查询、Cookie、SteamID 全文或响应正文。

## 8. 组件契约

| 组件 | 输入属性 | 信号 | 状态 |
| --- | --- | --- | --- |
| `AccountSessionBar` | `SessionStatus` | `loginRequested/logoutRequested/syncRequested` | anonymous/authenticated/expired/unavailable |
| `InventoryFilterBar` | contexts/categories/counts | `filterChanged/selectAllRequested` | enabled/loading/disabled |
| `InventoryTableModel` | `InventoryGroup[]` | `selectionChanged/groupActivated` | loading/normal/empty/stale/error |
| `PricingPanel` | selection summary, strategy, fees | `draftRequested/reviewRequested` | empty/dirty/quoting/ready/issues |
| `DraftReviewView` | `ListingDraft` | `saveRequested/handoffRequested` | ready/stale/handedOff |
| `SteamWebDialog` | allowed purpose + URL | `sessionVerified/closed/navigationBlocked` | initializing/loading/ready/error |
| `SyncTaskBar` | `InventorySync[]`, diagnostics | `cancel/retry/openDiagnostics` | idle/running/rateLimited/failed |
| `StatusBadge` | code/label/severity | none | info/success/warning/error |

前端不得使用表格 cell 作为业务真相；选择状态由 `InventorySelectionModel` 按稳定 group key 管理。

## 9. 可访问性

- 正文最小 14px，辅助文本最小 12px；表格行高 48px；
- 正常文本对比度 ≥4.5:1，焦点环 ≥3:1；
- 所有批量动作可键盘到达，复选框有可读名称；
- 价格列使用等宽数字并右对齐；
- 状态同时使用图标、文字和颜色；
- 125%/150%/200% DPI 不截断按钮和金额；
- 动画时长 ≤160ms，并尊重系统减少动画设置。
