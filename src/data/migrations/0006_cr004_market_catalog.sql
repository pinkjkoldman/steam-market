-- CR-004/CR-005: user-driven Steam market catalog page cache.
-- The existing items(market_hash_name) primary key remains unchanged in this release.
CREATE TABLE IF NOT EXISTS market_catalog_pages (
    cache_key       TEXT PRIMARY KEY,
    query_json      TEXT NOT NULL,
    appid           INTEGER,
    offset          INTEGER NOT NULL CHECK (offset >= 0),
    page_size       INTEGER NOT NULL CHECK (page_size BETWEEN 1 AND 100),
    total_count     INTEGER NOT NULL CHECK (total_count BETWEEN 0 AND 10000000),
    result_json     TEXT NOT NULL CHECK (length(result_json) <= 2097152),
    currency        TEXT NOT NULL CHECK (currency IN ('CNY','USD')),
    language        TEXT NOT NULL CHECK (language IN ('schinese','english')),
    fetched_at      DATETIME NOT NULL,
    expires_at      DATETIME NOT NULL,
    last_accessed_at DATETIME NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_market_catalog_app_fetched
    ON market_catalog_pages (appid, fetched_at DESC);
CREATE INDEX IF NOT EXISTS idx_market_catalog_expires
    ON market_catalog_pages (expires_at);
CREATE INDEX IF NOT EXISTS idx_market_catalog_lru
    ON market_catalog_pages (last_accessed_at DESC);

CREATE TABLE IF NOT EXISTS market_scope_snapshots (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    scope_key   TEXT NOT NULL CHECK (
        scope_key = 'all' OR (
            scope_key GLOB 'appid:[1-9]*'
            AND substr(scope_key, 7) NOT GLOB '*[^0-9]*'
        )
    ),
    total_count INTEGER NOT NULL CHECK (total_count BETWEEN 0 AND 10000000),
    fetched_at  DATETIME NOT NULL,
    source      TEXT NOT NULL CHECK (source = 'steam_market_search'),
    UNIQUE (scope_key, fetched_at)
);
CREATE INDEX IF NOT EXISTS idx_market_scope_latest
    ON market_scope_snapshots (scope_key, fetched_at DESC);

ALTER TABLE items ADD COLUMN localized_name TEXT;
ALTER TABLE items ADD COLUMN sell_listings INTEGER CHECK (sell_listings IS NULL OR sell_listings >= 0);
ALTER TABLE items ADD COLUMN lowest_sell_minor INTEGER CHECK (lowest_sell_minor IS NULL OR lowest_sell_minor >= 0);
ALTER TABLE items ADD COLUMN catalog_seen_at DATETIME;
ALTER TABLE items ADD COLUMN catalog_currency TEXT CHECK (catalog_currency IS NULL OR catalog_currency IN ('CNY','USD'));
ALTER TABLE items ADD COLUMN catalog_language TEXT CHECK (catalog_language IS NULL OR catalog_language IN ('schinese','english'));
