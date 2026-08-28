# CR-006 设计令牌增量

> 继承 `design-tokens.md` 与 CR-004 令牌；本文件只新增真实历史状态和图表令牌。

| Token | 值 | 用途 |
| --- | --- | --- |
| `history.online` | `#49C596` | Steam 在线数据；必须同时显示文字 |
| `history.cache` | `#78A6D8` | 缓存数据 |
| `history.auth` | `#5A93FF` | 登录恢复动作 |
| `history.warning` | `#E8A84C` | 限流、断档、来源未知 |
| `history.error` | `#E5484D` | schema/持久化错误 |
| `history.offline` | `#9AA3B2` | 网络不可用 |
| `history.neutral` | `#7D8798` | 空数据/单点 |
| `chart.history.price` | `#5A93FF` | 价格折线 |
| `chart.history.pricePoint` | `#B7D0FF` | 选中/单点 |
| `chart.history.volumeFill` | `rgba(90,147,255,0.32)` | 成交量面积 |
| `chart.history.crosshair` | `rgba(232,234,237,0.55)` | 共享游标 |
| `chart.history.gap` | `#E8A84C` | 断档说明，不画连接线 |

尺寸：

- `history.banner.minHeight = 44px`；双行时自适应，不固定截断；
- `chart.history.priceMinHeight = 180px`；
- `chart.history.volumeMinHeight = 64px`；
- `chart.history.splitRatio = 72:28`；
- `chart.axis.maxMajorTicks = 6`；
- `chart.tooltip.maxWidth = 260px`。

状态不能只通过颜色表达；所有令牌均需搭配图标、标题和辅助文案。

