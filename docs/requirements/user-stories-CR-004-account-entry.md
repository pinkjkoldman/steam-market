# CR-004 用户故事与验收标准

## US-30 首次启动选择身份（Must）

作为首次使用者，我希望启动时选择登录或游客模式。

- Given 没有启动选择，When 正常启动，Then 显示欢迎页并提供“游客模式”和“登录 Steam”；
- Given 选择游客，When 进入，Then 不发起登录请求并打开行情页；
- Given 选择登录，When 继续，Then 打开 Steam 官方登录页，应用不出现密码输入框；
- Given WebView2 不可用，When 点击登录，Then 显示诊断且游客仍可用；
- Given 使用 `--smoke-test`，When 启动，Then 欢迎页不得阻塞自动测试。

## US-31 游客使用公开功能（Must）

- Given 当前为游客，When 浏览行情、排行、规则、自选或本地持仓，Then 不反复要求登录；
- Given 游客打开库存助手，When 输入 SteamID64，Then 可查询公开库存并明确不是账户登录；
- Given 游客访问登录态历史或盘口，When 数据不可用，Then 显示原因、降级数据和一次点击登录入口。

## US-32 Steam 官方登录（Must）

- Given 当前为游客，When 发起登录，Then 凭据只在 Steam 官方 WebView 中输入；
- Given 官方登录产生有效 SteamID64 与 Cookie，When 应用接收，Then 全局变为已登录并进入库存助手；
- Given 登录未完成或取消，When 返回，Then 保持游客且不丢失本地工作；
- Given 登录失败，When 错误到达，Then 显示恢复建议且日志不含 Cookie 或令牌。

## US-33 全局身份与切换（Must）

- Given 任意主页面，When 查看侧栏账户区，Then 可见游客、公开库存或已登录之一；
- Given 游客或公开库存，When 点击登录，Then 发起官方登录；
- Given 已登录，When 确认退出，Then 清理 Cookie 并切换为游客；
- Given 退出完成，When 查看自选、持仓和设置，Then 本地数据保持不变；
- Given 停留详情页，When 身份变化，Then 页面不被无条件重置。

## US-34 受限能力就地登录（Must）

- Given 游客点击同步私人库存，When 权限检查，Then 显示原因及“登录 Steam”主按钮；
- Given 游客选择公开库存，When 查询，Then 不要求登录；
- Given 登录完成，When 返回原操作，Then 自动重试一次或明确提示继续，不产生重复任务。

## US-35 记住启动模式（Should）

- Given 用户选择下次自动游客，When 下次启动，Then 直接进入游客行情页；
- Given 用户选择每次询问，When 下次启动，Then 再次显示欢迎页；
- Given 曾经登录，When 重启且没有安全恢复设计，Then 不得仅凭历史选择显示已登录；
- Given 设置被重置，When 再次启动，Then 回到身份选择页。

## US-36 安全退出与过期（Must）

- Given 已登录，When 确认退出，Then WebView 与 QNetworkCookieJar Cookie 均被清除；
- Given 会话过期，When 私有请求返回认证错误，Then 全局降级为游客并提供重新登录；
- Given 任意错误日志，When 检查，Then 不含 Cookie、令牌、密码或 Steam Guard 内容。

## US-37 清晰可信的欢迎页（Must）

作为首次使用者，我希望欢迎页清楚、有质感且不诱导登录，以便快速理解两种模式。

- Given 欢迎页默认状态，When 用户在 1040×680 窗口查看，Then 游客与登录两张模式卡片、能力差异、主按钮和安全说明无需滚动即可看见；
- Given 两种模式并列，When 比较视觉层级，Then 游客可带“推荐”标签，但登录仍有完整按钮、说明和相近视觉权重；
- Given 用户查看登录卡片，When 阅读安全说明，Then 明确看到“Steam 官方页面”和“不接收或保存密码”；
- Given 用户使用键盘，When 按 Tab，Then 焦点依次到达游客入口、登录入口和记住选项，Enter 可激活当前入口；
- Given 用户关闭欢迎页，When 没有选择模式，Then 应用按游客进入而不是退出或停留空白窗口。

## US-38 一致的账户状态视觉（Must）

作为用户，我希望一眼识别当前身份，并理解公开库存与登录账户的区别。

- Given 当前为游客、公开库存、已登录或过期，When 查看侧栏账户卡片，Then 每种状态均使用图标、状态名称和辅助文案，不只使用颜色；
- Given 当前为公开库存身份，When 查看账户卡片，Then 显示“仅公开数据”与 SteamID64 尾号，不出现“已登录”或安全认证标记；
- Given 当前已登录，When 查看账户卡片，Then 显示名称、SteamID64 尾号和可信状态，退出入口位于账户菜单；
- Given 会话状态改变，When 信号到达，Then 侧栏、库存助手和受限提示在同一事件循环内更新为一致状态；
- Given 用户确认退出，When 清理完成，Then 账户卡片转为游客且本地数据仍然可见。

## US-39 专业一致的视觉系统（Must）

作为高频行情用户，我希望界面兼具 Steam 社区气质和专业终端的可读性。

- Given 欢迎页与主工作台，When 并排比较，Then 使用同一品牌标记、字体层级、冷蓝强调色、圆角与图标笔画风格；
- Given 导航与操作按钮，When 检查图形资产，Then 不混用 Emoji、文本符号和多套图标；
- Given 行情价格和汇总数字变化，When 刷新，Then 数字保持稳定对齐且不会因字宽跳动；
- Given 状态、涨跌或错误信息，When 关闭颜色显示或进行灰度检查，Then 仍可通过符号、标签或文字区分；
- Given 页面存在装饰背景，When 检查文字区域，Then 装饰保持低对比度且不降低正文或按钮可读性。

## US-40 响应式、高 DPI 与动效（Must）

作为使用不同显示器或键盘的用户，我希望界面在缩放与动态状态下依然稳定。

- Given 1040×680、1280×800 与 1680×1080，When 打开欢迎页和主工作台，Then 关键主操作可达、文字不重叠、内容不会无限拉宽；
- Given Windows 缩放为 125% 或 150%，When 查看模式卡片、侧栏账户区和表格，Then 文案与图标不截断，交互目标高度至少约 36px；
- Given 正文、辅助文字和控件边界，When 进行对比度检查，Then 正文达到 4.5:1，大字与非文本边界达到 3:1；
- Given 减少动态效果偏好开启，When 身份或页面状态切换，Then 非必要动画停用且功能不受影响；
- Given 默认动态效果，When 状态切换，Then 动画仅限约 120～220ms 的淡入或轻量位移，不阻塞输入。

## US-41 登录与异常视觉状态（Must）

作为登录中的用户，我希望每一步状态都可理解、可取消、可恢复。

- Given 登录已经发起，When 官方页面加载，Then 欢迎页或账户区显示“等待 Steam 官方登录”并提供取消；
- Given 用户取消或登录未完成，When 返回应用，Then 恢复游客状态，不显示失败红色警报；
