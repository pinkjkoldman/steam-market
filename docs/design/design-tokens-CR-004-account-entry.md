# CR-004 账户入口设计令牌（G2）

令牌扩展既有 `design-tokens-usability.md`，G3 由统一主题层消费，禁止组件内散落颜色与尺寸常量。

## 1. 色彩

| Token | 值 | 用途 |
|---|---|---|
| color.canvas | `#07111F` | 应用最底层背景 |
| color.surface.1 | `#0B1726` | 主内容与欢迎层 |
| color.surface.2 | `#101F31` | 卡片 |
| color.surface.3 | `#15283C` | hover/浮层 |
| color.border.subtle | `#24364A` | 普通边框 |
| color.border.accentMuted | `#37678E` | hover/推荐卡边框 |
| color.text.primary | `#F2F7FB` | 标题与关键数据 |
| color.text.secondary | `#A9B8C7` | 正文 |
| color.text.muted | `#718397` | 辅助说明 |
| color.accent.primary | `#2F9FEA` | 主按钮/链接 |
| color.accent.hover | `#53B5F4` | 主按钮 hover |
| color.accent.pressed | `#2085C7` | 主按钮 pressed |
| color.status.success | `#35D0A1` | 已认证/成功 |
| color.status.warning | `#F3B95F` | 会话失效/注意 |
| color.status.error | `#F17878` | 阻断错误 |
| color.focus | `#8CD2FF` | 键盘焦点环 |

正文配色必须通过 WCAG 4.5:1；大字与非文本控件至少 3:1。状态色不单独承担语义。

## 2. 字体

字体族按 Windows 回退：`Segoe UI Variable`, `Microsoft YaHei UI`, `Segoe UI`, sans-serif。

| Token | size/line/weight | 用途 |
|---|---|---|
| type.display | 32/40/650 | 欢迎页标题 |
| type.title.lg | 22/30/600 | 模式卡标题 |
| type.title.md | 18/26/600 | 受限态/账户面板 |
| type.body | 15/23/400 | 正文 |
| type.body.sm | 13/20/400 | 辅助信息 |
| type.label | 13/18/600 | 按钮/徽标 |
| type.data | 14/20/550 | SteamID/数值，tabular nums |

Qt 实现使用设备无关字号并验证中文回退；不靠全大写制造层级。

## 3. 间距与布局

基础步长 4 dp：`space.1=4`、`2=8`、`3=12`、`4=16`、`5=20`、`6=24`、`7=28`、`8=32`、`10=40`、`12=48`、`16=64`、`20=80`、`24=96`。

- 页面水平内边距：32 dp，窄宽 24 dp；
- 卡片内边距：28 dp，紧凑态 24 dp；
- 模式卡列间距：24 dp；
- 表单标签与输入：8 dp；
- 主次按钮：12 dp。

## 4. 圆角、边框、阴影

- radius.sm 6 dp：按钮/输入；
- radius.md 10 dp：菜单/提示条；
- radius.lg 14 dp：模式卡/受限态；
- border.default 1 dp；focus 2 dp + 2 dp offset；
- shadow.card：`0 12 32 rgba(0,0,0,.22)`，仅欢迎模式卡；
- 禁止多层彩色辉光和高亮玻璃边。

## 5. 控件尺寸

- button.md：44 dp；button.sm：36 dp；
- input.md：42 dp；
- icon.sm/md/lg：16/24/48 dp；
- avatar.account：36 dp；
- touch/click target 最小 36×36 dp，主要操作 44×44 dp。

## 6. 动效

- motion.fast 120 ms；normal 160 ms；slow 220 ms；
- easing.standard `OutCubic`；进入 `OutQuad`；离开 `InQuad`；
- reduce-motion：位移 0，持续时间不超过 80 ms。

## 7. 图标

统一 24×24 viewBox、2 px stroke、round cap/join；16 dp 时使用单独简化路径而非直接压缩复杂图标。状态图标与文字间距 8 dp。品牌图标资产必须注明来源与许可；自制中性图标纳入仓库 SVG 源文件。

## 8. Qt 组件映射

| Token 组 | Qt 层 |
|---|---|
| color.* | `ThemePalette` / QPalette + QSS variables generator |
| type.* | `Typography` helper |
| space/radius | `UiMetrics` constants |
| motion.* | `MotionSpec`，受系统设置覆盖 |
| icon.* | `IconRegistry`，按语义名取图标 |

组件禁止直接出现十六进制色值；视觉回归若需例外，必须在 G3 实现文档登记。
