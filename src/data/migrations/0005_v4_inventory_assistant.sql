CREATE TABLE steam_accounts (
    steam_id TEXT PRIMARY KEY,
    display_name TEXT,
    session_state TEXT NOT NULL CHECK (session_state IN ('anonymous','authenticated','expired','unavailable')),
    last_verified_at TEXT,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE inventory_syncs (
    sync_id TEXT PRIMARY KEY,
    steam_id TEXT NOT NULL REFERENCES steam_accounts(steam_id) ON DELETE CASCADE,
    appid INTEGER NOT NULL,
    context_id TEXT NOT NULL,
    state TEXT NOT NULL CHECK (state IN ('running','completed','failed','cancelled')),
    cursor TEXT,
    page_count INTEGER NOT NULL DEFAULT 0 CHECK (page_count >= 0),
    asset_count INTEGER NOT NULL DEFAULT 0 CHECK (asset_count >= 0),
    error_code TEXT,
    started_at TEXT NOT NULL,
    completed_at TEXT
);

CREATE INDEX idx_inventory_syncs_context_time
    ON inventory_syncs(steam_id, appid, context_id, started_at DESC);

CREATE TABLE inventory_descriptions (
    appid INTEGER NOT NULL,
    class_id TEXT NOT NULL,
    instance_id TEXT NOT NULL,
    market_hash_name TEXT,
    display_name TEXT NOT NULL,
    icon_url TEXT,
    item_type TEXT,
    category TEXT NOT NULL DEFAULT 'unknown',
    marketable INTEGER NOT NULL DEFAULT 0 CHECK (marketable IN (0,1)),
    tradable INTEGER NOT NULL DEFAULT 0 CHECK (tradable IN (0,1)),
    tags_json TEXT NOT NULL DEFAULT '[]' CHECK (json_valid(tags_json)),
    updated_at TEXT NOT NULL,
    PRIMARY KEY (appid, class_id, instance_id)
);

CREATE INDEX idx_inventory_desc_hash ON inventory_descriptions(appid, market_hash_name);
CREATE INDEX idx_inventory_desc_category ON inventory_descriptions(category, marketable);

CREATE TABLE inventory_assets (
    steam_id TEXT NOT NULL REFERENCES steam_accounts(steam_id) ON DELETE CASCADE,
    appid INTEGER NOT NULL,
    context_id TEXT NOT NULL,
    asset_id TEXT NOT NULL,
    class_id TEXT NOT NULL,
    instance_id TEXT NOT NULL,
    amount INTEGER NOT NULL DEFAULT 1 CHECK (amount > 0),
    last_seen_sync_id TEXT NOT NULL REFERENCES inventory_syncs(sync_id),
    marketable_after TEXT,
    last_seen_at TEXT NOT NULL,
    PRIMARY KEY (steam_id, appid, context_id, asset_id),
    FOREIGN KEY (appid, class_id, instance_id)
        REFERENCES inventory_descriptions(appid, class_id, instance_id)
);

CREATE INDEX idx_inventory_assets_group
    ON inventory_assets(steam_id, appid, context_id, class_id, instance_id);
CREATE INDEX idx_inventory_assets_sync ON inventory_assets(last_seen_sync_id);

CREATE TABLE listing_drafts (
    draft_id TEXT PRIMARY KEY,
    steam_id TEXT NOT NULL REFERENCES steam_accounts(steam_id) ON DELETE CASCADE,
    status TEXT NOT NULL CHECK (status IN ('draft','quoting','ready','handed_off','stale','abandoned')),
    currency TEXT NOT NULL,
    strategy_type TEXT NOT NULL CHECK (strategy_type IN ('fixed','lowest_sell_minus_tick','highest_buy')),
    strategy_value_minor INTEGER,
    keep_per_group INTEGER NOT NULL DEFAULT 0 CHECK (keep_per_group >= 0),
    inventory_synced_at TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE listing_draft_items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    draft_id TEXT NOT NULL REFERENCES listing_drafts(draft_id) ON DELETE CASCADE,
    appid INTEGER NOT NULL,
    context_id TEXT NOT NULL,
    market_hash_name TEXT NOT NULL,
    inventory_quantity INTEGER NOT NULL CHECK (inventory_quantity > 0),
    selected_quantity INTEGER NOT NULL CHECK (selected_quantity > 0 AND selected_quantity <= inventory_quantity),
    buyer_pays_minor INTEGER CHECK (buyer_pays_minor > 0),
    fee_minor INTEGER CHECK (fee_minor >= 0),
    seller_receives_minor INTEGER CHECK (seller_receives_minor >= 0),
    state TEXT NOT NULL CHECK (state IN ('included','excluded','quote_failed','stale')),
    exclusion_reason TEXT,
    quote_at TEXT,
    UNIQUE (draft_id, appid, context_id, market_hash_name)
);

CREATE INDEX idx_listing_draft_items_state ON listing_draft_items(draft_id, state);

CREATE TABLE handoff_batches (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    draft_id TEXT NOT NULL REFERENCES listing_drafts(draft_id) ON DELETE CASCADE,
    batch_no INTEGER NOT NULL CHECK (batch_no > 0),
    appid INTEGER NOT NULL,
    context_id TEXT NOT NULL,
    official_url TEXT NOT NULL,
    status TEXT NOT NULL CHECK (status IN ('ready','opened','abandoned')),
    opened_at TEXT,
    UNIQUE (draft_id, batch_no)
);
