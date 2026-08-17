# CR-003 G3 模块实施计划

日期：2026-08-03  
输入基线：已批准的 G2 架构、API 契约、数据模型、安全设计、UX/UI 规范  
实施方式：单代理顺序开发；所有模块共享同一 Qt `.pro` 与应用装配文件，不并行修改。

## 1. 冻结契约

- 库存资产键：`steamId + appid + contextId + assetId`。
- 描述键：`appid + classId + instanceId`。
- Steam 卡牌默认上下文：`appid=753`、`contextId=6`。
- 金额：人民币分（integer minor units），不使用浮点保存报价。
- 自动化边界：只读库存、批量选择、定价草稿、打开 Steam 官方页面；禁止市场交易 POST、自动点击提交和自动确认。
- 登录边界：应用不接收 Steam 密码；只允许 Steam 官方网页登录会话或公开库存只读模式。

## 2. 模块目录与依赖

| 顺序 | 模块 | 主要文件 | 依赖 | 验收标准 |
|---:|---|---|---|---|
| 1 | 契约/技术探针 | `docs/implementation/contract-validation-CR-003.md`、`third_party/webview2/` | G2 契约、Qt MinGW | Schema 引用无悬空；WebView2 最小编译结果与降级选择有记录 |
| 2 | 数据库迁移 | `src/data/migrations/0005_cr003_inventory_workspace.sql`、`migrations.qrc` | G2 物理设计 | 新库可迁移到 v5；重复启动幂等；事务失败可回滚 |
| 3 | 库存领域模型/解析器 | `src/core/models/Inventory*.h`、`src/network/InventoryParser.*` | QtCore | 正确关联 assets/descriptions；识别可交易/可市场物品；分页游标可读 |
| 4 | Steam 只读库存客户端 | `src/network/SteamInventoryClient.*` | QtNetwork、解析器 | 支持 app/context、`count=5000`、`more_items/last_assetid`、限速与错误分类；无市场写请求 |
| 5 | 库存仓储与同步服务 | `src/data/repositories/InventoryRepository.*`、`src/core/services/InventoryService.*` | SQLite、客户端 | 分页成功后原子切换快照；失败保留上次完整数据；可按物品分组 |
| 6 | 定价草稿/交接 | `src/core/services/PricingDraftService.*`、`MultiSellHandoffService.*` | 库存模型、Qt URL | 批量数量不超过库存；金额为整数；同 app/context 分批；只生成/打开官方 URL |
| 7 | 官方登录宿主 | `src/network/IWebSessionHost.h`、`src/network/WebView2SessionHost.*` | WebView2 探针 | 官方域名白名单；不暴露 HostObject；不采集密码；若探针失败使用明确降级实现 |
| 8 | 库存工作台 UI | 复用 `src/ui/pages/TradingPage.*` 并重构语义 | 会话、库存、草稿、交接服务 | 登录状态、上下文、同步、过滤、分组全选、价格、草稿审核、空/错/加载状态完整 |
| 9 | 应用装配与主题 | `src/app/AppController.*`、`MainWindow.*`、`src/src.pro` | 全部模块 | 导航显示“库存助手”；依赖单向；应用可构建并启动 |
| 10 | G3 自测与文档 | `tests/test_inventory.*`、实现文档与 G3 报告 | Qt Test | 解析、分组、定价、URL、安全边界、迁移测试通过；构建与冒烟通过 |

## 3. 集成顺序

```text
迁移 → 领域模型/解析器 → 客户端 → 仓储/同步
     → 定价草稿/交接 → 登录宿主 → UI → 应用装配 → 自测
```

UI 只调用服务，不直接发网络请求或执行 SQL。外部 Steam 能力通过接口注入；测试使用固定 JSON 夹具，不访问真实账号或市场。

## 4. 安全删除清单

G3 中必须移除或失效化以下旧能力：

- 用户名/密码/Steam Guard 输入框和旧 RSA 登录请求。
- 手工 Cookie 粘贴入口。
- `createbuyorder`、`cancelbuyorder`、`sellitem` 市场写请求。
- “自动执行（跳过人工确认）”开关和周期交易扫描。
- 手工填写 `market_hash_name` 才能选择物品的主流程。

## 5. G3 门禁证据

- 构建日志与测试日志。
- 迁移 v5 和回滚说明。
- WebView2 探针结果或降级决策。
- 禁止端点字符串扫描结果。
- 库存工作台冒烟截图。
- `docs/implementation/backend-CR-003.md`、`frontend-CR-003.md`、`database-CR-003.md`、`integration-CR-003.md`。
