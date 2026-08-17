# 设计令牌

> 前端实现必须使用本令牌，禁止硬编码颜色/字号/间距。

## 1. 色彩

| Token | 值 | 用途 |
| --- | --- | --- |
| color.bg.base | `#1B1E24` | 窗口背景（深色） |
| color.bg.surface | `#232731` | 面板/卡片背景 |
| color.bg.surfaceAlt | `#2B303C` | 表头/悬停背景 |
| color.border | `#3A4050` | 分隔线/边框 |
| color.text.primary | `#E8EAED` | 主文本 |
| color.text.secondary | `#9AA3B2` | 次级文本 |
| color.text.muted | `#6B7280` | 占位/禁用 |
| color.accent | `#3D7EFF` | 品牌色/选中/链接 |
| color.up | `#E5484D` | 涨（默认红涨绿跌） |
| color.down | `#46A758` | 跌 |
| color.warn | `#F5A623` | 警告/过期角标 |
| color.error | `#E5484D` | 错误提示 |
| color.ok | `#46A758` | 在线状态 |

> 配色方案切换（设置 → 涨跌配色）时仅交换 `color.up` 与 `color.down` 的映射，其余不变。

## 2. 字体与字号

| Token | 值 |
| --- | --- |
| font.family | "Microsoft YaHei UI", "Segoe UI", sans-serif |
| font.size.xs | 11px（辅助/角标） |
| font.size.sm | 12px（次级文本） |
| font.size.md | 13px（正文/表格） |
| font.size.lg | 15px（卡片标题） |
| font.size.xl | 20px（详情页物品名/价格） |
| font.weight.normal / medium / bold | 400 / 600 / 700 |
| font.tabular | 等宽数字（价格/销量列） |

## 3. 间距与圆角

| Token | 值 |
| --- | --- |
| spacing.xs | 4px |
| spacing.sm | 8px |
| spacing.md | 12px |
| spacing.lg | 16px |
| spacing.xl | 24px |
| radius.sm | 4px（输入框/按钮） |
| radius.md | 6px（卡片/面板） |
| radius.lg | 8px（弹窗） |

## 4. 阴影与高度

- `shadow.card`：`0 1px 3px rgba(0,0,0,0.35)`（卡片）；
- `shadow.popup`：`0 8px 24px rgba(0,0,0,0.5)`（弹窗/菜单）；
- 表格行高默认 36px；详情页卡片最小高度 84px。

## 5. 可访问性

- 文本/背景对比度 ≥ 4.5:1（正常文本），次级文本 ≥ 3:1；
- 涨跌不止用颜色区分，另附 ▲/▼ 符号；
- 全部控件键盘可达（Tab 顺序、回车确认、Esc 取消）；
- 支持 125%/150% 高 DPI（Qt `AA_EnableHighDpiScaling` 已内置于 Qt6，随窗口缩放）。

---

# v2 令牌补充（CR-001）

- `chart.candle.up`：`#E5484D`（阳线，红涨）；`chart.candle.down`：`#46A758`（阴线，绿跌）——与 color.up/down 联动，配色切换时同步；
- `chart.ma5/ma10/ma20`：`#F5A623` / `#3D7EFF` / `#9AA3B2`（均线区分色）；
- `orderbook.buy`：`#E5484D`（买盘）；`orderbook.sell`：`#46A758`（卖盘）；
- `chart.volume`：`rgba(61,126,255,0.45)`（成交量柱，可复用 v1 销量色）。
