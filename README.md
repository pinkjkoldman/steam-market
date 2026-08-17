# Steam 行情终端（Steam Market Terminal）

类似证券行情软件的 Steam 市场数据统计小软件（Windows 桌面，Qt 6.11 / C++17）。

## 功能

- 行情：搜索 Steam 社区市场物品，查看当前价、销量、24h 涨跌幅；
- 图表：详情页走势（折线）/ 分时 / 日K（蜡烛 + 成交量 + MA5/10/20）；
- 盘口：买盘/卖盘五档挂单 + 最高买价/最低卖价（Steam itemordershistogram）；
- 排行榜：成交量榜、涨幅榜、跌幅榜；
- 交易规则：内置 Steam 市场规则库 + 费用计算器（费率可配置）；
- 模拟交易：记录买卖（含手续费），联动持仓估值；
- 自选：关注列表，启动与定时自动刷新；
- 提醒：低于/高于/24h 涨跌幅阈值，托盘通知；
- 持仓：手动录入库存，按市价估值汇总盈亏；
- 比价：可插拔数据源（内置 Steam 源 + CSV 导入兜底）；
- 缓存：SQLite 本地持久化，断网降级展示。

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
```

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
