# v4 设计令牌（CR-003）

## 1. 色彩

| Token | 值 | 用途 |
| --- | --- | --- |
| `color.bg.canvas` | `#12161D` | 窗口背景 |
| `color.bg.surface` | `#1B2230` | 主面板 |
| `color.bg.raised` | `#242D3D` | 抽屉、悬浮卡片 |
| `color.bg.hover` | `#2C3749` | 悬停/选中弱态 |
| `color.border.default` | `#3A465A` | 边框与分隔 |
| `color.text.primary` | `#F3F6FA` | 主文本 |
| `color.text.secondary` | `#B8C1CE` | 次级文本 |
| `color.text.muted` | `#8792A3` | 占位/禁用 |
| `color.accent` | `#66C0F4` | Steam 会话/链接/焦点 |
| `color.action` | `#3D7EFF` | 主操作 |
| `color.action.hover` | `#5A93FF` | 主操作悬停 |
| `color.success` | `#49C58F` | 成功/可售 |
| `color.warning` | `#F2B84B` | 过期/保护/限流 |
| `color.error` | `#FF6B6B` | 错误/阻断 |
| `color.info` | `#66C0F4` | 信息态 |

涨跌颜色继续由用户设置映射，不与库存可售/不可售颜色共用。

## 2. 字体

| Token | 值 |
| --- | --- |
| `font.family.ui` | `"Microsoft YaHei UI", "Segoe UI", sans-serif` |
| `font.family.numeric` | `"Cascadia Mono", "Segoe UI Mono", monospace` |
| `font.size.caption` | `12px` |
| `font.size.body` | `14px` |
| `font.size.bodyStrong` | `14px / 600` |
| `font.size.section` | `16px / 600` |
| `font.size.pageTitle` | `22px / 700` |
| `font.size.amount` | `24px / 700` |

## 3. 间距与尺寸

| Token | 值 |
| --- | --- |
| `space.1` | `4px` |
| `space.2` | `8px` |
| `space.3` | `12px` |
| `space.4` | `16px` |
| `space.5` | `20px` |
| `space.6` | `24px` |
| `space.8` | `32px` |
| `radius.control` | `6px` |
| `radius.card` | `10px` |
| `size.control.md` | `36px` |
| `size.control.lg` | `40px` |
| `size.table.row` | `48px` |
| `size.icon.item` | `40px` |
| `size.focusRing` | `2px` |

## 4. 阴影与动效

- `shadow.raised`: `0 8px 24px rgba(0,0,0,0.35)`；
- `shadow.dialog`: `0 16px 48px rgba(0,0,0,0.55)`；
- `motion.fast`: 100ms；`motion.normal`: 160ms；
- 同步进度使用不旋转的进度条或低速 spinner；不使用闪烁。

## 5. QSS 约定

- 所有颜色和尺寸由集中主题生成器输出，页面不得硬编码十六进制值；
- `objectName` 只用于语义样式（primaryButton、dangerButton、statusBadge），禁止按页面复制大段 QSS；
- 禁用态降低对比度但仍须可读；焦点态使用 `color.accent` 2px 描边；
- 表格 zebra 行不使用高对比条纹，仅以 `color.bg.surface/raised` 轻微区分。
