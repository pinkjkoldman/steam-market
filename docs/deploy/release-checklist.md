# 发布检查清单

> 阶段 5 部署交付（2026-08-01 执行）。

## 构建与测试

- [x] Release 构建通过（`scripts/build.ps1`，退出码 0）
- [x] 单元测试全部通过（14/14，退出码 0）
- [x] 冒烟自测通过（`--smoke-test`，退出码 0，截图生成）

## 打包

- [x] windeployqt 部署全部 Qt 依赖（Qt6Core/Widgets/Charts/Sql/Network/OpenGL/OpenGLWidgets/Svg）
- [x] 编译器运行库（libgcc/libstdc++/libwinpthread）已包含
- [x] qt.conf 已生成（插件路径指向应用目录）
- [x] 仅保留 SQLite 驱动（qsqlite.dll）
- [x] 使用说明与 CSV 样例已附带

## 自包含验证（干净 PATH，无 Qt 环境）

- [x] 发布目录 exe 直接运行冒烟：退出码 0
- [x] 安装包解压后 exe 直接运行冒烟：退出码 0

## 数据与回滚

- [x] 数据库随首次启动自动创建并迁移（版本 0001）
- [x] 备份机制（设置页"立即备份" → steam_market.db.bak）
- [x] 回滚方案已文档化（见 rollback-plan.md）
- [x] 日志与错误追踪可用（%APPDATA%/.../logs/app.log）

## v2（CR-001）追加检查

- [x] 迁移 0002 应用成功（orderbook_snapshots、trades）
- [x] 规则库/费率随程序内嵌（rules.json + settings）
- [x] v2 冒烟（K线/排行榜/费用/模拟交易）通过
- [x] v2 打包版干净环境冒烟通过
