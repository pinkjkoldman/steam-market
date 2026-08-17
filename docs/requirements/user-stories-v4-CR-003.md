# v4 用户故事与验收标准（CR-003）

## US-24 登录后直接查看库存（Must）

作为需要整理大量物品的玩家，我希望登录后直接看到自己的库存，而不是先搜索物品。

- Given 尚未建立会话，When 打开库存页，Then 显示“通过 Steam 官方页面登录”，应用不显示密码输入框或 Cookie 粘贴框；
- Given 官方登录成功，When 回到库存页，Then 自动加载并展示库存，无需输入物品名称；
- Given 返回 429、会话失效或库存不可访问，When 加载失败，Then 显示明确原因和恢复入口。

## US-25 卡牌与社区物品（Must）

作为卡牌收藏玩家，我希望软件识别 Steam 社区库存中的卡牌并按同名物品聚合。

- Given `753/6` 包含普通卡、闪卡、表情和背景，When 同步完成，Then 按类型分类且普通卡与闪卡不混合；
- Given 一个名称对应多个资产，When 展示列表，Then 显示聚合数量并保留每个独立 `assetid`；
- Given 名称位于 `descriptions`，When 解析库存，Then 通过 `classid + instanceid` 正确关联；
- Given 响应含 `more_items/last_assetid`，When 加载，Then 自动继续分页且不重复、不漏项。

## US-26 批量选择与定价（Must）

作为需要出售大量卡牌的玩家，我希望一次选择多种卡牌并生成价格草稿。

- Given 库存已加载，When 选择全部可售卡牌或按游戏/套卡筛选，Then 仅选中 `marketable=true` 的资产；
- Given 选择固定价、最低在售价减一档或最高求购价，When 生成草稿，Then 展示买家支付、手续费、卖家到手和更新时间；
- Given 某物品无行情、受保护或低于最低到手价，When 生成草稿，Then 默认移入异常区且不提交；
- Given 设置每种保留 N 张，When 选择全部，Then 每组实际选中数不超过库存数减 N。

## US-27 Steam 官方批量上架交接（Must）

作为玩家，我希望减少逐个进入物品页和逐个填价，同时保留 Steam 官方确认。

- Given 草稿通过校验，When 点击“在 Steam 中复核并上架”，Then 打开登录态的官方 `market/multisell` 页面并带入所选同质物品；
- Given 某物品不支持批量上架，When 交接，Then 标记并提供打开官方市场页的降级入口；
- Given 用户未在 Steam 页面提交，When 返回应用，Then 草稿不得标记为已上架；
- Given Steam 要求市场确认，When 提交完成，Then 提示到手机应用或邮件确认，不尝试自动确认。

## US-28 可诊断的同步状态（Must）

作为用户，我希望知道 Steam 接口为什么失败以及下一步怎么做。

- Given 返回 401/403/429、空响应或结构变化，When 同步失败，Then 分别显示会话、权限、限流或解析错误；
- Given 使用缓存库存，When 页面展示，Then 显示缓存时间与“可能已变化”；
- Given 满足重试条件，When 用户重试，Then 从失败页码继续且不重复资产。

## US-29 库存优先界面（Should）

作为高频使用者，我希望主要操作集中在一个界面中。

- Given 库存已加载，When 改变筛选和选择，Then 批量定价区与汇总实时更新；
- Given 窗口变窄，When 缩小窗口，Then 筛选可收起、关键列可见、批量操作栏固定可达；
- Given 有失败或待处理项，When 查看任务区，Then 可定位物品并查看恢复建议。
