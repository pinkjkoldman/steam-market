-- 0004_v3_autotrade：自动化交易（登录会话、规则、订单）
CREATE TABLE IF NOT EXISTS sessions (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    account_name     TEXT NOT NULL,
    cookie_encrypted TEXT NOT NULL,
    steam_id         TEXT,
    created_at       DATETIME NOT NULL,
    expires_at       DATETIME
);

CREATE TABLE IF NOT EXISTS trading_rules (
    id                INTEGER PRIMARY KEY AUTOINCREMENT,
    appid             INTEGER NOT NULL,
    market_hash_name  TEXT NOT NULL,
    side              TEXT NOT NULL CHECK (side IN ('buy','sell')),
    price_condition   TEXT NOT NULL DEFAULT 'any' CHECK (price_condition IN ('below','above','any')),
    threshold_price   REAL,
    quantity          INTEGER NOT NULL DEFAULT 1,
    budget_limit      REAL,
    daily_limit       REAL,
    cooldown_minutes  INTEGER NOT NULL DEFAULT 30,
    enabled           INTEGER NOT NULL DEFAULT 1,
    created_at        DATETIME NOT NULL,
    updated_at        DATETIME NOT NULL
);

CREATE TABLE IF NOT EXISTS orders (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    rule_id          INTEGER,
    market_hash_name TEXT NOT NULL,
    appid            INTEGER NOT NULL,
    side             TEXT NOT NULL CHECK (side IN ('buy','sell')),
    quantity         INTEGER NOT NULL,
    price            REAL NOT NULL,
    fee              REAL NOT NULL DEFAULT 0,
    status           TEXT NOT NULL DEFAULT 'pending',
    external_id      TEXT,
    error            TEXT,
    created_at       DATETIME NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_orders_time ON orders (created_at);
