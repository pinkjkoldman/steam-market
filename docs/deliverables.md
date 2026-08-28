# 交付物索引

| 阶段 | 交付物 | 路径 | 状态 |
| --- | --- | --- | --- |
| 需求分析 | PRD | docs/requirements/PRD.md | 已交付 |
| 需求分析 | 用户故事与验收标准 | docs/requirements/user-stories.md | 已交付 |
| 系统设计 | 架构文档 | docs/design/architecture.md | 已交付 |
| 系统设计 | 架构决策记录（ADR） | docs/design/adr/ADR-*.md | 已交付 |
| 系统设计 | API 契约 | docs/design/api-contract.yaml | 已交付 |
| 系统设计 | 数据模型 | docs/design/data-model.md | 已交付 |
| 系统设计 | 数据库设计 | docs/design/database-design.md | 已交付 |
| 系统设计 | 安全设计 | docs/design/security.md | 已交付 |
| 系统设计 | UI/UX 规范 | docs/design/ui-spec.md | 已交付 |
| 系统设计 | 设计令牌 | docs/design/design-tokens.md | 已交付 |
| 系统设计 | 用户流程 | docs/design/ux.md | 已交付 |
| 开发实现 | Qt 工程源码（v1+v2） | src/ | 已交付 |
| 测试验证 | 测试计划/用例/报告/安全报告 | docs/test/* | 已交付 |
| 部署交付 | 打包脚本/CI 说明 | scripts/package.ps1、docs/deploy/ci-cd.md | 已交付 |
| 部署交付 | 便携版 ZIP（v2.0.0） | release/SteamMarketTerminal-portable.zip | 已交付 |
| 部署交付 | 自解压安装包（v2.0.0） | release/SteamMarketTerminal-Setup.exe | 已交付 |
| 部署交付 | 发布/部署/回滚文档 | docs/deploy/* | 已交付 |
| 门禁记录 | G1~G5 + CR-001 + v2 门禁 | docs/approvals/* | 已交付 |
| v3 变更 | CR-002 门禁报告 | docs/approvals/CR-002-v3-gate.md | 待审批 |
| v3 需求/设计 | PRD v3 / US-18~23 / 架构/API/数据/安全/UI 更新 | docs/requirements/*、docs/design/* | 已产出，待审批 |
| v3 已实施 | 多游戏（迁移 0003）+ 字体 + 解析修复 | src/ | 已产出，待审批 |
| v3 自动化交易 | AuthService/TradingService/自动交易页/迁移 0004 | src/ | 已产出，待审批 |
| v3 门禁报告 | G3+G4（v3） | docs/approvals/phase-3-report-v3.md | 待审批 |
| v3 制品 | portable.zip / Setup.exe（v3） | release/ | 已产出，待验收 |
| CR-004 需求 | 启动登录与游客模式 PRD | docs/requirements/PRD-CR-004-account-entry.md | 已批准 |
| CR-004 需求 | US-30～41（功能、UI、美术、响应式） | docs/requirements/user-stories-CR-004-account-entry.md | 已批准 |
| CR-004 架构 | 状态机、权限矩阵与模块边界 | docs/design/architecture-CR-004-account-entry.md | 待审批 |
| CR-004 ADR | 账户入口与会话边界 | docs/design/adr/ADR-004-account-entry-session-boundary.md | 待审批 |
| CR-004 契约 | 账户、权限与 UI 组件内部契约 | docs/design/account-contract-CR-004.md | 待审批 |
| CR-004 数据 | 运行时模型与启动设置 | docs/design/data-model-CR-004-account-entry.md | 待审批 |
| CR-004 数据库 | 无 schema 迁移与回滚结论 | docs/design/database-design-CR-004-account-entry.md | 待审批 |
| CR-004 安全 | 威胁模型与新鲜会话清理 | docs/design/security-CR-004-account-entry.md | 待审批 |
| CR-004 UX | 欢迎、切换、受限与恢复流程 | docs/design/ux-CR-004-account-entry.md | 待审批 |
| CR-004 UI | 高保真组件与美术计划 | docs/design/ui-spec-CR-004-account-entry.md | 待审批 |
| CR-004 令牌 | 色彩、字体、布局、动效与图标令牌 | docs/design/design-tokens-CR-004-account-entry.md | 待审批 |
| CR-004 视觉 | 欢迎页与账户状态概念稿 | docs/design/assets/cr004-*.png | 待审批 |
| CR-004 工作台 | 市场概览、物品浏览、价格分析与个人区 IA | docs/design/workbench-ia-CR-004.md | 待审批 |
| CR-004 工作台视觉（旧版） | 本地标的池市场概览概念稿 | docs/design/assets/cr004-market-overview-concept-v1.png | 已废弃，由 CR-005 v2 替代 |
| CR-005 需求 | Steam 全市场概览需求与 US-42～44 | docs/requirements/CR-005-steam-full-market-overview.md | 待审批 |
| CR-005 研究 | Steam 全市场 API 一手资料核查 | docs/research/steam-market-full-market-api-2026-08-13.md | 已交付 |
| CR-005 架构 | 全市场分页、缓存、限流与边界 | docs/design/steam-full-market-architecture-CR-004.md | 待审批 |
| CR-005 数据 | 目录、范围快照与来源模型 | docs/design/data-model-CR-005-steam-full-market.md | 待审批 |
| CR-005 数据库 | 页面缓存与 0006 迁移设计 | docs/design/database-design-CR-004-full-market.md | 待审批 |
| CR-005 API | 全市场目录内部契约 | docs/design/api-contract-CR-005-steam-full-market.yaml | 待审批 |
| CR-005 安全 | 网页端点、限流和合规设计 | docs/design/security-CR-005-steam-full-market.md | 待审批 |
| CR-005 视觉 | Steam 全市场概览高保真 v2 | docs/design/assets/cr004-market-overview-full-steam-concept-v2.png | 待审批 |
| CR-005 变更 | G1/G2 影响评估 | docs/approvals/change-request-CR-005-steam-full-market.md | 待审批 |
| CR-004/005 审批 | G2 修订审批记录 | docs/approvals/phase-2-approval-CR-004-CR-005.md | 已交付 |
| CR-004/005 G3 | 模块实施计划与冻结契约 | docs/implementation/module-plan.md | 进行中 |
| CR-004 门禁 | G2 系统设计报告 | docs/approvals/phase-2-report-CR-004.md | 待审批 |
| CR-004 门禁 | G1 功能 + UI/美术需求报告 | docs/approvals/phase-1-report-CR-004.md | 已批准 |
| CR-004 G3 实现 | 账户六态、权限门、新鲜 WebView2 会话 | src/core/models/Identity*.h、src/core/services/AccessGate.*、SteamSessionService.* | 已交付，待 G3 审批 |
| CR-005 G3 实现 | 全市场目录客户端、服务、仓库与迁移 0006 | src/core/models/MarketCatalog.h、src/network/SteamMarketCatalogClient.*、src/core/services/FullMarketService.*、src/data/repositories/MarketCatalogRepository.* | 已交付，待 G3 审批 |
| CR-004/005 G3 UI | 欢迎、市场概览、物品浏览、账户与查看器组件 | src/ui/pages/WelcomePage.*、MarketOverviewPage.*、MarketBrowserPage.*、src/ui/widgets/* | 已交付，待 G3 审批 |
| CR-004/005 G3 文档 | 后端、前端、数据库与集成实现说明 | docs/implementation/backend-account.md、backend-full-market.md、database.md、frontend-workbench.md、integration-CR-004-CR-005.md | 已交付，待 G3 审批 |
| CR-004/005 G3 视觉 | 1040×680、1280×800、1920×1080 市场概览冒烟图 | docs/test/smoke-g3-overview-*.png | 已交付，待 G3 审批 |
| CR-004/005 G3 门禁 | 阶段 3 报告 | docs/approvals/phase-3-report-CR-004-CR-005.md | 待审批 |
| CR-004 审批 | G1 审批记录 | docs/approvals/phase-1-approval-CR-004.md | 已交付 |
| CR-006 变更 | 真实市场走势图变更登记与影响评估 | docs/approvals/change-request-CR-006-real-market-trends.md | 已批准 |
| CR-006 需求 | 真实市场走势图 PRD | docs/requirements/PRD-CR-006-real-market-trends.md | 已批准 |
| CR-006 需求 | US-45～US-51 与 Given/When/Then 验收标准 | docs/requirements/user-stories-CR-006-real-market-trends.md | 已批准 |
| CR-006 门禁 | G1 需求阶段报告 | docs/approvals/phase-1-report-CR-006.md | 已批准 |
| CR-006 审批 | G1 审批记录 | docs/approvals/phase-1-approval-CR-006.md | 已交付 |
| CR-006 架构 | 类型化单品历史架构 | docs/design/architecture-CR-006-real-market-trends.md | 已交付，待 G2 审批 |
| CR-006 ADR | 单品按需历史深模块决策 | docs/design/adr/ADR-005-typed-on-demand-market-history.md | 已交付，待 G2 审批 |
| CR-006 API | 真实历史内部契约 | docs/design/api-contract-CR-006-real-market-trends.yaml | 已交付，待 G2 审批 |
| CR-006 数据 | HistoryDataset/Provenance/Quality 模型 | docs/design/data-model-CR-006-real-market-trends.md | 已交付，待 G2 审批 |
| CR-006 数据库 | v7 历史表重建、同步状态与恢复方案 | docs/design/database-design-CR-006-real-market-trends.md | 已交付，待 G2 审批 |
| CR-006 安全 | 真实历史威胁模型与安全基线 | docs/design/security-CR-006-real-market-trends.md | 已交付，待 G2 审批 |
| CR-006 UX | 真实走势流程与恢复状态 | docs/design/ux-CR-006-real-market-trends.md | 已交付，待 G2 审批 |
| CR-006 UI | 双子图与状态组件契约 | docs/design/ui-spec-CR-006-real-market-trends.md | 已交付，待 G2 审批 |
| CR-006 UI | 历史状态与图表设计令牌 | docs/design/design-tokens-CR-006-real-market-trends.md | 已交付，待 G2 审批 |
| CR-006 门禁 | G2 系统设计阶段报告 | docs/approvals/phase-2-report-CR-006.md | 待审批 |
