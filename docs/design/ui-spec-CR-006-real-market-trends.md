# CR-006 UI 规格：详情页真实走势

## 1. 桌面线框

```text
┌──────────────────────────────────────────────────────────────────────┐
│ ← 返回     物品名称                           [★自选] [提醒]        │
│ 最新价 ¥--   24h销量 --   数据口径：Steam 历史点                   │
├──────────────────────────────────────────────────────────────────────┤
│ ● Steam 在线数据  CNY · 更新 14:32 · 3,248 点 · 2020-08—2026-08    │
│ [1周] [1月] [3月] [1年] [全部]                         [刷新]       │
├──────────────────────────────────────────────────────────────────────┤
│ [价格走势] [24h 历史点] [日 K（聚合）]                              │
│ ┌──────────────────────── 价格主图 ───────────────────────────────┐ │
│ │                         ╱╲__╱╲                                  │ │
│ └─────────────────────────────────────────────────────────────────┘ │
│ ┌──────────────────────── 成交量副图 ─────────────────────────────┐ │
│ │                ▁▂▆▃▂▅    与主图共享时间范围                    │ │
│ └─────────────────────────────────────────────────────────────────┘ │
│ 说明：按 Steam 历史点显示；存在 2 处断档，未插值。                  │
├──────────────────────────────────────────────────────────────────────┤
│ 盘口 / 平台比价（保持现有结构）                                     │
└──────────────────────────────────────────────────────────────────────┘
```

缓存或登录态将第二行替换为状态横幅；不得覆盖图表或使用阻塞模态框。

## 2. 图表实现契约

### 2.1 `RealHistoryChart`

前端采用两个垂直 `QChartView`：

- 上图：`QLineSeries`；单点时改用 `QScatterSeries`；
- 下图：`QAreaSeries` 表示成交量，不再把 `QBarSeries` 挂到 `QDateTimeAxis`；
- 两图各自持有 `QDateTimeAxis`，范围由同一个 `ChartViewport` 同步；下图显示 X 标签，上图隐藏重复 X 标签；
- 价格 Y 轴自动范围加 5% padding；成交量 Y 轴从 0 起；
- volume=null 的点不画量值，不能按 0 绘制；
- 原始点不插值；断档处拆分为多个 line segment，避免跨越长空白连线；
- 选中/悬停时两图共享垂直游标和 tooltip。

组件接口：

```text
setDataset(const HistoryDataset& dataset)
setRangeDays(int days)        // 7/30/90/365/0
setReducedMotion(bool enabled)
clear(ChartEmptyReason reason)

signals:
pointFocused(QDateTime recordedAtUtc, double price, optional<int> volume)
```

### 2.2 `KlineChart`

- 输入新增 `sampleCount` 与 `sparse`；
- 页签标题固定“日 K（聚合）”；面板说明固定“按 Steam 历史点本地聚合”；
- sparse 日蜡烛允许 O=H=L=C，tooltip 显示“样本 1”；
- MA 线只在窗口样本日数足够时出现；图例可切换但默认全部显示。

## 3. 组件契约

| 组件 | 属性/信号 | 状态 | 说明 |
| --- | --- | --- | --- |
| `HistoryStatusBanner` | `setState(HistoryViewState)`；`loginRequested`、`retryRequested` | loading/online/cache/auth/rate/empty/error/offline | 图标+标题+辅助信息+最多一个主动作 |
| `HistoryRangeSelector` | `rangeChanged(int days)` | 7/30/90/365/all | 本地筛选，不触发网络 |
| `HistoryRefreshButton` | `refreshRequested()` | idle/loading/cooldown | loading 禁重复；cooldown 显示剩余时间 |
| `RealHistoryChart` | `setDataset`、`setRangeDays` | normal/single/sparse/empty | 双子图共享时间范围 |
| `HistoryTooltip` | time/price/volume/source | visible/hidden | 时间显示本地时区，内部仍 UTC |
| `KlineChart` | `setBars(KlineBar[])` | normal/sparse/empty | 明确聚合语义 |
| `DataQualityNote` | gapCount/sampleCount/range | normal/warn | 只解释，不篡改数据 |

## 4. 状态视觉

| 状态 | 图标/文字 | 色彩 token | 行为 |
| --- | --- | --- | --- |
| Online | 实心圆 + “Steam 在线数据” | `history.online` | 允许刷新 |
| Cache | 时钟 + “缓存数据” | `history.cache` | 显示最后时间 |
| Auth | 锁 + “登录获取官方历史” | `history.auth` | 登录主按钮 |
| Rate limited | 计时器 + “请求受限” | `history.warning` | 倒计时 |
| Empty | 空折线 + “暂无历史点” | `history.neutral` | 官方页/刷新 |
| Error/Offline | 断线 + 明确错误标题 | `history.error`/`history.offline` | 保留缓存时不清图 |

## 5. 响应式

| 窗口 | 行为 |
| --- | --- |
| 1040×680 | 来源条换成两行；周期可横向紧凑；主图 ≥180px、副图 ≥64px；盘口下移可滚动 |
| 1280×800 | 默认线框；主图约 260px、副图 90px |
| 1920×1080 | 内容最大宽度 1600px；图表扩展，来源条不无限拉开 |
| 125%/150% DPI | 按钮高度 ≥36px；坐标轴最多 6 个主刻度；tooltip 不越出图表 |

## 6. 加载、空与错误

- 首次无缓存：静态图表骨架 + 来源条 loading；
- 有缓存刷新：图表保持，右上显示小型 spinner 和“正在刷新”；
- 真实空数据：不沿用旧物品曲线；显示当前物品的明确空状态；
- 切换物品：立即清除前一物品 selection/tooltip；可显示新物品缓存；
- schema 变化：显示“Steam 数据格式暂不可用”，保留旧缓存并提供官方页面；
- 数据库写失败：在线图可临时显示，但状态为“本次数据未保存”，不得声称缓存成功。

## 7. 验收截图矩阵

- 分辨率：1040×680、1280×800、1920×1080；
- DPI：100%、125%、150%；
- 状态：online、cache+warning、auth_required、rate_limited、empty、source_error、single point、sparse gaps；
- 主题：深色默认 + 反向涨跌色；
- 真实联网截图：仅裁剪详情内容，隐藏账户卡和任何身份信息。

