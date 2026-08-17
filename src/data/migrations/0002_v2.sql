-- 0002_v2：v2 迭代新增表（盘口快照、模拟交易）
CREATE TABLE IF NOT EXISTS orderbook_snapshots (
    market_hash_name TEXT PRIMARY KEY REFERENCES items(market_hash_name),
    buy_orders_json  TEXT,
    sell_orders_json TEXT,
    highest_buy      REAL,
    lowest_sell      REAL,
    fetched_at       DATETIME NOT NULL
);

CREATE TABLE IF NOT EXISTS trades (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    market_hash_name TEXT NOT NULL REFERENCES items(market_hash_name),
    appid            INTEGER NOT NULL,
    side             TEXT NOT NULL CHECK (side IN ('buy','sell')),
    quantity         INTEGER NOT NULL CHECK (quantity >= 1),
    price            REAL NOT NULL,
    fee              REAL NOT NULL DEFAULT 0,
    total            REAL NOT NULL,
    traded_at        DATETIME NOT NULL,
    note             TEXT
);
CREATE INDEX IF NOT EXISTS idx_trades_time ON trades (traded_at);
CREATE INDEX IF NOT EXISTS idx_trades_hash ON trades (market_hash_name);
