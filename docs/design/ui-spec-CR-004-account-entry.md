# CR-004 高保真 UI 与组件规范（G2）

## 1. 视觉定位

“专业市场终端 × Steam 社区氛围 × 克制的游戏感”。界面以深海军蓝为基底、Steam 蓝为主强调、青绿只用于在线/成功语义。深色层次依靠边框和明度，不使用大面积霓虹、重玻璃、复杂粒子或广告轮播。

概念参考：[欢迎页概念图](assets/cr004-welcome-concept-v1.png)、[账户/权限状态板](assets/cr004-account-states-concept-v1.png) 与 [Steam 全市场概览 v2](assets/cr004-market-overview-full-steam-concept-v2.png)。旧 `cr004-market-overview-concept-v1.png` 的本地标的池口径已废弃。市场概览、物品浏览、价格分析及个人工作区的详细信息架构见 `workbench-ia-CR-004.md`；概念图仅定义构图和氛围，文档契约优先。

## 2. 欢迎页构图

画布采用 16:10 优先布局：

- 顶部 64 dp 应用栏：48×48 品牌符号、产品名、版本/安全状态可选区域；
- 内容顶部留白 72–96 dp；
- 标题居中，最大宽 760 dp；
- 模式卡网格宽 960 dp，列间距 24 dp；
- 页脚显示隐私提示与“可随时切换”说明，不放营销信息。

欢迎层覆盖主内容但保留窗口原生标题栏；背景可显示低对比的市场网格纹理，透明度 ≤6%，禁止模拟真实行情造成信息误导。

## 3. ModeCard 组件

### 属性

`variant = guest | steam`、`recommended`、`busy`、`disabled`、`errorText`、`capabilities[]`、`primaryAction`。

### 尺寸

- 桌面：最小高 286 dp，圆角 14 dp，内边距 28 dp；
- 图标容器 64×64 dp；标题 22 dp；正文 15 dp；
- 主按钮高 44 dp，最小宽 152 dp；能力列表行高 28 dp。

### 状态

- default：1 px `border.subtle`；
- hover：边框过渡到 `border.accentMuted`，上移 1 dp；
- focus-within：2 px 外焦点环，不上移；
- selected/recommended：顶部徽标，不改变权限语义；
- busy：按钮内进度环，不降低整卡透明度；
- error：边框保留中性，仅错误行使用警告图标，避免整卡“危险红”。

整卡不作为按钮；只有明确主按钮可激活。

## 4. BrandMark 与图标计划

G2 使用可代码化 SVG/Qt Path 图标，统一 2 px 圆角描边、24 dp 基准网格：

- 品牌符号：圆环/节点表达 Steam 社区，简化柱线表达市场终端；
- 游客：`eye`，强调浏览而非匿名面具；
- 登录：`steam-link` 或受控品牌符号，不伪造 Steam 官方按钮；
- 身份：`person-outline`、`person-check`、`person-clock`；
- 权限：`lock`；公开库存：`inventory-search`；
- 状态：success/warning/error 使用 circle-check/triangle-alert/circle-x。

禁止混用 emoji、彩色系统图标、不同笔画粗细和未经许可的 Steam 商标变体。G3 若使用 Steam 官方标志，应复核品牌使用条件；否则使用中性“外部官方登录”图标并以文字说明 Steam。

## 5. AccountCard 组件

- 宽度随侧栏，最小高 80 dp；
- 左侧 36 dp 头像/状态图标，中间两行文本，右侧 28 dp 菜单按钮；
- 主标题单行省略，辅助信息最多两行；
- Guest 使用中性蓝灰，Authenticated 使用青绿状态点，Expired 使用琥珀警告；
- 点击卡片正文打开账户面板，菜单按钮有独立可访问名称；
- 窄侧栏折叠时保留状态图标与 tooltip，但登录/失效状态仍须有文本入口。

## 6. AccessPrompt 与受限态

受限态最大宽 520 dp，垂直居中于内容区，不使用模态框阻断公开功能：

- 图标 48 dp；标题 20 dp；正文 14–15 dp；
- 主/次按钮横排，窄宽度纵排；
- `RequireLogin`、`RequirePublicId`、`Reauthenticate` 使用不同文案，但结构一致；
- 不展示模糊的“权限不足”，必须说明需要哪种身份以及登录后会发生什么。

## 7. 登录表面

WebView 登录页顶部使用本地可信栏：锁图标、`Steam 官方登录`、规范化主机名、关闭按钮。可信栏不可由网页覆盖；网页内容与本地栏之间有 1 px 分隔。若导航被拦截，使用本地错误页，绝不把远端 HTML 注入应用错误组件。

## 8. 文案规则

- 使用“登录 Steam”，不用“绑定账号”；
- 使用“游客模式”，不用“未登录用户”；
- 使用“本人库存”和“公开库存”区分能力；
- 使用“会话已失效”，不用“登录错误”；
- 主操作使用动词短语，不使用“确定”；
- 安全说明具体到“官方页面”和“不保存密码”，不承诺无法验证的绝对安全。

## 9. 动效

- 欢迎层退出：透明度 140 ms + 上移 4 dp；
- 卡片 hover：120 ms；
- 状态点/提示条切换：160 ms cross-fade；
- 进度环连续旋转但不与大面积背景联动；
- 系统减少动态时，所有位移动画关闭，透明过渡 ≤80 ms。

## 10. 分辨率检查

G3 截图验收：1040×680、1280×800、1920×1080，Windows 125%、150%、200%。每组检查：标题换行、卡片等高、主按钮可见、安全提示、账户卡省略、受限态和错误态。

## 11. 组件交付契约

| 组件 | 必须实现状态 | 截图/测试要求 |
|---|---|---|
| AccountEntryOverlay | default、login busy、WebView unavailable、offline | 3 尺寸 + 键盘遍历 |
| ModeCard | default、hover、focus、busy、disabled、error | 视觉回归 |
| AccountCard | Guest、Public、Authenticating、Authenticated、Expired | 状态矩阵 |
| AccessPrompt | RequireLogin、RequirePublicId、Reauthenticate | 行为与焦点回归 |
| LoginTrustBar | normal、blocked navigation、clear failure | 域名与关闭行为 |
| InventoryRestrictedState | guest、expired、private inventory | 页面 smoke |

## 12. 美术迭代计划

- G2：完成构图、色彩、组件和品牌图标方向；
- G3：用 SVG/Qt 样式实现品牌符号、6 类功能图标、欢迎页背景纹理与全部状态；
- G4：以视觉回归截图修正高 DPI、文本溢出、对比度和动效降级；
- G5：仅做发布级资产压缩、版权/品牌检查和最终截图归档，不在上线前改变交互结构。
