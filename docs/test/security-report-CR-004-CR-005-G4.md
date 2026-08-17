# CR-004 + CR-005 G4 安全测试报告

## 1. 结论

未发现开放高危安全漏洞。G4 发现并修复一项中风险整数溢出边界；账户、权限、匿名目录、严格解析、缓存事务和禁止全量遍历控制均通过验证。Steam 网页端点条款风险仍是 G5 发布合规门禁，不属于已关闭的代码漏洞。

## 2. 检查结果

| 检查项 | 结果 | 证据 |
| --- | --- | --- |
| 密钥与凭据 | 通过 | 源码/测试/示例中无 API key、publisher key、密码、token 或 Cookie 值 |
| 日志脱敏 | 通过 | 未发现记录 Cookie、token、密码、完整 SteamID 或完整回调 URL 的日志语句 |
| 身份一致性 | 通过 | Cookie SteamID 与信号不一致时不认证；50 项账户测试通过 |
| 新鲜登录会话 | 通过（代码与假宿主） | WebView2 Cookie 清理成功后才导航；清理失败阻断；迟到回调丢弃 |
| 安全注销 | 通过（服务边界） | WebView 清理被调用，QNetworkCookieJar 清空，身份回到游客 |
| 权限门 | 通过 | 六态 × 七能力 42 行数据驱动矩阵 |
| 目录隔离 | 通过 | 独立匿名 QNetworkAccessManager，CookieJar 拒绝收发 Cookie |
| 传输/重定向 | 通过 | 精确 HTTPS 主机/路径，ManualRedirectPolicy，重定向整页拒绝 |
| 输入/响应 | 通过 | 查询/appid/offset/limit/币种/语言白名单；2 MiB、字段长度、整数和 schema 上限 |
| 图标 URL | 通过 | 仅 HTTPS Steam CDN 主机；userinfo/超长/非白名单 URL 丢弃 |
| 限流 | 通过 | 单并发、至少 1.5 秒；429 使用有界 Retry-After；不自动分页 |
| SQL 注入/一致性 | 通过 | 动态 IN 仅生成 `?` 占位符；用户值全部绑定；事务失败回滚 |
| 市场写操作 | 通过 | 未发现 create/cancel buy order、sell item、market POST 等路径 |
| 全量抓取 | 通过 | 未发现 offset<total_count 循环、目录定时遍历或代理/验证码绕过 |

## 3. G4 修复

`Retry-After` 原实现先做 `qint64 seconds * 1000`，极大服务端值可能溢出。现改为先校验负数/解析失败，再与 24 小时上限的秒值比较，最后才乘法。覆盖正常、缺失、负数和 qint64 最大值四类测试。

## 4. 依赖核查

- WebView2 Runtime：151.0.4129.78；本地 Loader 签名有效，签名者 Microsoft Corporation；SDK 目标版本 150.0.4078.44，与 Runtime 向前兼容；
- Microsoft 建议使用 Evergreen Runtime 并允许自动更新，以持续获得安全修复：<https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/developer-guide>；
- Qt 6.11.1 已发布并包含安全与质量修复，建议 G5 构建环境升级后重跑全量测试：<https://www.qt.io/blog/qt-6.11.1-released>；
- Qt 6.11 目录中的已知高危补丁集中在 QtSvg，另有 Qt5Compat 补丁；本应用链接 Core/Gui/Widgets/Network/Sql/Charts/OpenGL，不链接 QtSvg、Qt5Compat、QtXml，相关公告不适用于当前攻击面：<https://download.qt.io/official_releases/qt/6.11/>；
- QtXml 的 CVE-2026-15037 为低危且本应用不链接/使用 QDom：<https://www.qt.io/blog/security-advisory-cve-2026-15037-xml-injection>。

依赖结论：当前未识别到适用于本应用链接模块的高危已知漏洞；升级 Qt 6.11.1 作为 G5 依赖卫生任务，不阻塞 G4 工程验证。

## 5. 剩余风险

1. 真实 Steam 登录和 Steam Guard UAT 需要用户亲自执行；不得截取或提交凭据/Cookie；
2. `search/render` 是 Steam 官方网页内部端点，不是面向普通第三方的正式 Web API；G5 必须复核当日条款和端点行为；
3. 持续全量镜像、后台值守、代理轮换、验证码绕过和登录 Cookie 采集仍明确禁止；
4. WebView2 Evergreen 自动更新不应被企业策略禁用。

## 6. 安全准出

- 开放高危漏洞：0；
- 开放中危产品漏洞：0；
- 发布合规高风险：R-019 继续开放，G5 复核；
- 安全结论：允许提交 G4 用户 UAT，不授权进入 G5 或发布。
