# Steam 行情终端（Steam Market Terminal）v1.0.0

类似证券行情软件的 Steam 市场数据统计小软件（Windows 桌面，Qt 6.11 / C++17）。

## 功能

- 行情：搜索 Steam 社区市场物品，查看当前价、销量、24h 涨跌幅；
- 市场筛选：按关键词、游戏和远端排序检索，支持常用类型快捷定位，并可按当前页类型、价格、挂单量组合筛选；
- 市场概览：显示远端命中数、当前页可见数、中位价、挂单总量和流动性分级；
- 图表：详情页走势（折线）/ 分时 / 日K（蜡烛 + 成交量 + MA5/10/20）；
- 盘口：Steam 新版官方 `queryAction` 订单簿、买卖五档、价差、深度与失衡率；
- 排行榜：成交量榜、涨幅榜、跌幅榜；
- 交易规则：内置 Steam 市场规则库 + 费用计算器（费率可配置）；
- 模拟交易：记录买卖（含手续费），联动持仓估值；
- 自选：关注列表，启动与定时自动刷新；
- 提醒：低于/高于/24h 涨跌幅阈值，托盘通知；
- 持仓：手动录入库存，按市价估值汇总盈亏；
- 库存：同步真实库存，支持单品上架、批量出售交接和选择用户发起官方交易报价；
- 比价：可插拔数据源（内置 Steam 源 + CSV 导入兜底）；
- 缓存：SQLite 本地持久化，断网降级展示；
- 网络代理：跟随系统 / 直连 / HTTP / SOCKS5，保存后立即生效；
- 单实例保护：重复启动会唤起已运行窗口；
- 应用图标 + 版本管理（v1.0.0，见“设置 → 关于”）。

## 隐私与免责声明

本软件为非官方工具，行情数据来自 Steam 社区市场公开接口，仅供个人参考，不构成任何交易建议。软件不收集、不上传任何用户数据；Steam 登录仅用于获取自己的库存与官方历史价格，密码不经过本软件。

## 构建（Windows，Qt 6.11 + MinGW）

```powershell
scripts/build.ps1
```

或手工执行：

```powershell
& 'D:\Qt\6.11.0\mingw_64\bin\qmake.exe' SteamMarketTerminal.pro
& 'D:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe' -j4
```

运行：`.\bin\SteamMarketTerminal.exe`。数据与日志写入 `%APPDATA%/SteamMarketTerminal/`。

## 测试与冒烟

```powershell
scripts/build.ps1 -Test
.\bin\tests.exe
.\bin\SteamMarketTerminal.exe --smoke-test .\smoke.png
.\bin\SteamMarketTerminal.exe --smoke-test .\detail.png --smoke-scene detail --smoke-size 1280x800
```

## 打包发布

```powershell
scripts/package.ps1
```

生成 `release/SteamMarketTerminal-portable.zip`（便携版）与 `release/SteamMarketTerminal-Setup.exe`（自解压安装包），并自动执行无 Qt 环境的运行时验证。

## 目录结构

```text
src/
  app/       应用层（AppController、MainWindow）
  core/      业务逻辑（models、services、errors）
  data/      数据访问（DatabaseManager、repositories、migrations）
  network/   Steam 市场接口客户端（限速、重试）
  price_sources/  比价数据源（IPriceSource、CSV 导入）
  ui/        页面与组件（pages、widgets、TrayManager）
  utils/     日志、币种等工具
tests/       Qt Test 单元测试
docs/        项目文档
```
