-- 0002_auth: users, roles and sessions.
--
-- Password hashing (Argon2id) and session-token generation happen in the
-- server (libsodium); the database only ever stores the password hash and
-- the SHA-256 digest of a session token, never the token itself.
-- Append-only: never edit this file once applied.

CREATE EXTENSION IF NOT EXISTS citext; -- case-insensitive emails

-- Generic "bump updated_at on UPDATE" trigger, reused by later tables.
CREATE FUNCTION set_updated_at() RETURNS trigger
    LANGUAGE plpgsql
AS $$
BEGIN
    NEW.updated_at := now();
    RETURN NEW;
END;
$$;

CREATE TABLE users (
    id            uuid        PRIMARY KEY DEFAULT gen_random_uuid(),
    email         citext      NOT NULL UNIQUE,
    display_name  text        NOT NULL,
    password_hash text        NOT NULL,              -- Argon2id, libsodium crypto_pwhash_str format
    created_at    timestamptz NOT NULL DEFAULT now(),
    updated_at    timestamptz NOT NULL DEFAULT now(),
    disabled_at   timestamptz,                        -- set = account locked out

    CONSTRAINT users_email_not_blank CHECK (length(trim(email)) > 0),
    CONSTRAINT users_display_name_not_blank CHECK (length(trim(display_name)) > 0)
);

CREATE TRIGGER users_set_updated_at
    BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

-- Fixed role catalogue; ids are stable and referenced by the server.
CREATE TABLE roles (
    id   smallint PRIMARY KEY,
    name text     NOT NULL UNIQUE
);

INSERT INTO roles (id, name) VALUES
    (1, 'admin'),
    (2, 'user');

CREATE TABLE user_roles (
    user_id uuid     NOT NULL REFERENCES users (id) ON DELETE CASCADE,
    role_id smallint NOT NULL REFERENCES roles (id),
    PRIMARY KEY (user_id, role_id)
);

CREATE TABLE sessions (
    id           uuid        PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id      uuid        NOT NULL REFERENCES users (id) ON DELETE CASCADE,
    token_sha256 bytea       NOT NULL UNIQUE,         -- digest of the opaque bearer token
    created_at   timestamptz NOT NULL DEFAULT now(),
    expires_at   timestamptz NOT NULL,                -- sliding: extended on use
    last_seen_at timestamptz NOT NULL DEFAULT now(),
    revoked_at   timestamptz,                         -- set = logged out / revoked

    CONSTRAINT sessions_token_sha256_length CHECK (octet_length(token_sha256) = 32),
    CONSTRAINT sessions_expires_after_creation CHECK (expires_at > created_at)
);

-- Lookups: "which user owns this token?" (covered by the UNIQUE index on
-- token_sha256) and "active sessions of a user" (logout-everywhere, listing).
CREATE INDEX sessions_user_id_active_idx
    ON sessions (user_id)
    WHERE revoked_at IS NULL;
