-- 0003_v3：多游戏支持——唯一约束加入 appid，避免跨游戏数据混淆
CREATE UNIQUE INDEX IF NOT EXISTS ux_price_history_appid
    ON price_history (market_hash_name, appid, currency, recorded_at);
CREATE UNIQUE INDEX IF NOT EXISTS ux_price_snap_appid
    ON price_snapshots (market_hash_name, appid, currency, created_at);
