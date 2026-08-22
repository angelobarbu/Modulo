-- 0001_init: baseline migration.
--
-- Establishes the meta table and exercises the migration pipeline end to end.
-- Real schema (auth, transactions, ...) arrives in later increments, one
-- append-only migration file each. Applied migration files must NEVER be
-- edited — the runner verifies checksums and refuses to continue if one
-- changes.

CREATE TABLE meta (
    key text PRIMARY KEY,
    value text NOT NULL,
    updated_at timestamptz NOT NULL DEFAULT now()
);

INSERT INTO meta (key, value)
VALUES ('schema_baseline', 'increment-1');
