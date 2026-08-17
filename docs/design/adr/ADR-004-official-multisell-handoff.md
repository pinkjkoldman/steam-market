# ADR-004：交易改为 Steam 官方批量上架页面交接

- 状态：建议批准
- 日期：2026-08-03
- 关联：CR-003、US-26、US-27

## 背景

v3 直接调用未公开 `sellitem/createbuyorder`，无法覆盖卡牌资产模型，且无人值守自动化违反当前 Steam 用户协议。Steam 市场上架最终确认不可关闭。

## 决策

- 原生应用负责库存同步、批量选择、定价草稿、费用预览和异常诊断；
- 交易提交由同一登录态 WebView2 打开 Steam 官方 `market/multisell` 页面；
- 不注入价格、不自动点击、不自动批准手机/邮件确认；
- 本地状态使用 `draft → ready → handed_off`，不得因打开页面而写成 `submitted/success`；
- 不支持多卖的项目降级为官方单品市场页。

## 影响

- `TradingService` 自动执行路径弃用；旧表保留只读以支持回滚与历史查看；
- 新增草稿和批次模型；
- 账号风险下降，但官方页面变更仍需适配与降级。

## 官方依据

- Steam 用户协议：<https://store.steampowered.com/subscriber_agreement/english>
- 市场确认：<https://help.steampowered.com/en/faqs/view/2E6E-A02C-5581-8904>
