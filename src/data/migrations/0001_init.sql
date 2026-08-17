-- 0001_init：初始表结构（对应 data-model.md）
CREATE TABLE IF NOT EXISTS schema_migrations (
    version    INTEGER PRIMARY KEY,
    applied_at DATETIME NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS items (
    market_hash_name TEXT PRIMARY KEY,
    appid            INTEGER NOT NULL,
    name             TEXT NOT NULL,
    icon_url         TEXT,
    item_type        TEXT,
    rarity           TEXT,
    first_seen_at    DATETIME NOT NULL,
    updated_at       DATETIME NOT NULL
);

CREATE TABLE IF NOT EXISTS price_snapshots (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    market_hash_name TEXT NOT NULL REFERENCES items(market_hash_name),
    appid            INTEGER NOT NULL,
    currency         TEXT NOT NULL,
    price_low        REAL,
    price_high       REAL,
    volume           INTEGER,
    created_at       DATETIME NOT NULL,
    UNIQUE (market_hash_name, currency, created_at)
);
CREATE INDEX IF NOT EXISTS idx_snap_hash_time ON price_snapshots (market_hash_name, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_snap_time ON price_snapshots (created_at);

CREATE TABLE IF NOT EXISTS price_history (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    market_hash_name TEXT NOT NULL REFERENCES items(market_hash_name),
    appid            INTEGER NOT NULL,
    currency         TEXT NOT NULL,
    price            REAL NOT NULL,
    volume           INTEGER,
    recorded_at      DATETIME NOT NULL,
    UNIQUE (market_hash_name, currency, recorded_at)
);
CREATE INDEX IF NOT EXISTS idx_history_hash_time ON price_history (market_hash_name, recorded_at ASC);
CREATE INDEX IF NOT EXISTS idx_history_time ON price_history (recorded_at);

CREATE TABLE IF NOT EXISTS watchlist (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    market_hash_name TEXT NOT NULL UNIQUE REFERENCES items(market_hash_name),
    appid            INTEGER NOT NULL,
    added_at         DATETIME NOT NULL,
    note             TEXT,
    sort_order       INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX IF NOT EXISTS idx_watch_sort ON watchlist (sort_order);

CREATE TABLE IF NOT EXISTS alerts (
    id                INTEGER PRIMARY KEY AUTOINCREMENT,
    market_hash_name  TEXT NOT NULL REFERENCES items(market_hash_name),
    appid             INTEGER NOT NULL,
    condition_type    TEXT NOT NULL CHECK (condition_type IN ('below','above','percent_24h')),
    threshold_value   REAL,
    percent_value     REAL,
    enabled           INTEGER NOT NULL DEFAULT 1,
    last_triggered_at DATETIME,
    created_at        DATETIME NOT NULL,
    updated_at        DATETIME NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_alerts_enabled_hash ON alerts (enabled, market_hash_name);

CREATE TABLE IF NOT EXISTS portfolio_items (
    id                INTEGER PRIMARY KEY AUTOINCREMENT,
    market_hash_name  TEXT NOT NULL REFERENCES items(market_hash_name),
    appid             INTEGER NOT NULL,
    quantity          INTEGER NOT NULL CHECK (quantity >= 1),
    purchase_price    REAL,
    purchase_currency TEXT NOT NULL DEFAULT 'CNY',
    purchase_date     TEXT,
    note              TEXT,
    created_at        DATETIME NOT NULL,
    updated_at        DATETIME NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_portfolio_hash ON portfolio_items (market_hash_name);

CREATE TABLE IF NOT EXISTS platform_prices (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    market_hash_name TEXT NOT NULL REFERENCES items(market_hash_name),
    platform         TEXT NOT NULL CHECK (platform IN ('steam','buff','c5','youpin','csv')),
    price            REAL,
    currency         TEXT NOT NULL,
    url              TEXT,
    updated_at       DATETIME NOT NULL,
    UNIQUE (market_hash_name, platform)
);

CREATE TABLE IF NOT EXISTS settings (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
);
