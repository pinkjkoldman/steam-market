# CR-004 + CR-005 G4 测试报告

## 1. 结论

工程验证通过：完整应用构建成功，自动化 104/104 通过，三档原生 UI 冒烟通过，单次匿名 Steam 市场联网验证通过，无开放产品缺陷。

正式 G4 门禁尚有一项用户参与条件：真实 Steam 官方登录、Steam Guard、本人库存与注销 UAT。该测试不能由 Codex 代填凭据，因此报告状态为“技术准出，待 UAT 确认”，未进入 G5。

## 2. 自动化结果

| 测试类 | 通过 | 失败 | 跳过 |
| --- | ---: | ---: | ---: |
| TestAccountEntry | 50 | 0 | 0 |
| TestAlerts | 5 | 0 | 0 |
| TestCsv | 4 | 0 | 0 |
| TestDatabase | 9 | 0 | 0 |
| TestFee | 4 | 0 | 0 |
| TestFullMarket | 9 | 0 | 0 |
| TestInventory | 8 | 0 | 0 |
| TestKline | 4 | 0 | 0 |
| TestOrderbook | 3 | 0 | 0 |
| TestPortfolio | 4 | 0 | 0 |
| TestTrades | 4 | 0 | 0 |
| 合计 | 104 | 0 | 0 |

- 退出码：0；
- Qt Test 6.11.0 / Windows 11；
- 合并日志：`docs/test/g4-automated-tests.txt`；分项日志位于 `g4-automated-classes/`。

## 3. 构建与运行时

- `src/src.pro` 强制全量重建成功；唯一警告来自 Microsoft WebView2 头文件的 MSVC `#pragma warning` 被 MinGW 忽略；
- `tests/tests.pro` 强制重建成功；最终增量重建包含新增边界测试；
- `bin/SteamMarketTerminal.exe`：1,061,888 bytes；
- `bin/tests.exe`：最终测试构建成功；
- 三档 smoke 均退出码 0：
  - 1040×680 逻辑视口 → 1560×1020 PNG；
  - 1280×800 逻辑视口 → 1920×1200 PNG；
  - 1920×1080 逻辑视口在当前屏幕工作区内输出 2244×1370 PNG。

物理像素尺寸受 Windows 150% DPI 和屏幕工作区约束；原始 PNG 全高不透明、无渲染缺页。

## 4. Steam 联网验证

2026-08-14T07:43:58Z 对 Steam Community Market 官方公开页面端点执行一次匿名请求：HTTP 200、JSON、8716 bytes、10 条结果、`total_count=523504`。未携带登录 Cookie，未自动分页、未重试、未触发交易接口。

## 5. 可用性与可访问性

- 概览和浏览页主流程、来源/范围/币种/时间/缓存状态均可见；
- 主按钮默认/hover 对比度从 2.68/2.10 提升到约 5.37/4.66；
- 概览和浏览表格改用 `activated`，覆盖鼠标双击与键盘 Enter；
- 三档原生 UI 冒烟通过，未发现关键内容不可达。

## 6. 门禁评估

| 条件 | 结果 |
| --- | --- |
| 自动化 Must 100% 执行 | 通过 |
| 自动化通过率 ≥95% | 通过：100% |
| 无开放高危缺陷 | 通过 |
| 无高危安全漏洞 | 通过（见安全报告） |
| 真实账户 UAT | 待用户确认 |
| G4 用户审批 | 待审批 |

结论：工程侧已达到准出标准；真实账户 UAT 未完成前，不将 G4 标记为最终批准。
