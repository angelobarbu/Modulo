# Modulo — High-Level Design

## 1. Component & deployment view

```mermaid
flowchart LR
    subgraph desktop["macOS desktop"]
        subgraph client["modulo_client (Qt 6.8 / QML)"]
            qml["Main.qml
dark shell · status dot
polls every 3 s"]
            apiclient["ApiClient (QObject)
QNetworkAccessManager
QML_ELEMENT"]
            qml --> apiclient
        end

        subgraph serverproc["modulo_server (C++23 / QCoreApplication)"]
            http["http module
QHttpServer · routes · guards
error envelope · security headers"]
            authsvc["auth module
AuthService · repositories
Argon2id · tokens"]
            config["config module
env → Config
(MODULO_* vars)"]
            pool["db module
ConnectionPool"]
            http --> config
            http --> authsvc
            authsvc --> pool
        end

        subgraph migrate["modulo_migrate (CLI, Qt-free)"]
            migrator["db module
Migrator · libpqxx
checksums · transactions"]
        end
    end

    subgraph docker["Docker"]
        pg[("PostgreSQL 16
127.0.0.1:5433
modulo_dev · modulo_test
volume: modulo_pgdata")]
    end

    sql["db/migrations/
NNNN_name.sql
(append-only)"]

    apiclient -- "HTTP /api/v1/health, /api/v1/auth/*
127.0.0.1:8080 (loopback only)" --> http
    migrator -- "SQL over libpq" --> pg
    sql --> migrator
    pool -- "libpqxx" --> pg
```

## 2. Static library dependency graph

```mermaid
flowchart BT
    core["modulo_core
Result&lt;T&gt; (std::expected)
version() · QString-based"]
    api["modulo_api
Q_GADGET DTOs
Health / Error
api::json::require*"]
    cfg["modulo_server_config"]
    httpm["modulo_server_http
Server · guards · routes
responses · lcHttp"]
    dbm["modulo_server_db
(Qt-free · libpqxx)
Migrator · ConnectionPool"]
    authm["modulo_server_auth
AuthService · lcAuth
PasswordHasher (Argon2id)
token · Role
UserRepository · SessionRepository"]

    server(["modulo_server (exe)"])
    migrateexe(["modulo_migrate (exe, Qt-free)"])
    clientexe(["modulo_client (exe)"])

    api --> core
    cfg --> core
    authm --> core
    authm --> dbm
    httpm --> api
    httpm --> cfg
    httpm --> authm
    server --> httpm
    migrateexe --> dbm
    clientexe --> api

    qt["Qt6: Core · Network · HttpServer · Quick"]
    pqxx["libpqxx 8"]
    sodium["libsodium"]
    httpm -.-> qt
    clientexe -.-> qt
    core -.-> qt
    dbm -.-> pqxx
    authm -.-> sodium
```

Every server-side module is its own static library (`CMakeLists.txt` + `include/modulo/...` + `src/`),
created by the `modulo_*` CMake toolkit functions (warnings, sanitizers, clang-tidy, version
injection, qt.conf generation applied uniformly). `modulo_server_auth` holds the crypto and data layer
of Increment 2; the HTTP routes that use it (service + guards) are wired through `modulo_server_http`
next. Transactions / transfers / holdings / rates / documents follow as sibling modules.

## 3. Runtime flow — health check

```mermaid
sequenceDiagram
    participant Q as Main.qml (Timer 3 s)
    participant A as ApiClient
    participant S as QHttpServer route
    participant D as modulo_api DTOs

    Q->>A: checkHealth()
    A->>S: GET /api/v1/health
    S->>D: HealthResponse{ok, 0.1.0}.toJson()
    S-->>A: 200 {"status":"ok","version":"0.1.0"}
    A->>D: HealthResponse::fromJson (validating)
    D-->>A: Result of HealthResponse or Error
    A-->>Q: serverReachable / serverStatus properties
    Note over Q: green pulsing dot · "server ok (v0.1.0)"
```

## 4. Runtime flow — login and an authenticated request

```mermaid
sequenceDiagram
    participant C as Client
    participant R as auth routes (http)
    participant G as authed() guard
    participant A as AuthService
    participant P as PostgreSQL

    C->>R: POST /api/v1/auth/login {email, password}
    R->>A: login(email, password)
    A->>P: users by email (citext)
    A->>A: Argon2id verify (dummy hash if unknown, same timing)
    A->>A: token::generate (32 CSPRNG bytes, base64url)
    A->>P: INSERT sessions (sha256 digest, expires_at = now + 30 d)
    A-->>R: LoginResult {token, user}
    R-->>C: 200 {token, user} + security headers

    C->>G: GET /api/v1/auth/me, Authorization: Bearer token
    G->>A: authenticate(token)
    A->>P: sessions by digest (not revoked, not expired)
    A->>P: user by id (not disabled) + roles
    A->>P: touch session (sliding expiry, throttled)
    A-->>G: AuthContext {userId, sessionId, roles}
    G->>R: handler(context, request)
    R-->>C: 200 UserDto
    Note over G: no or bad token gives 401 auth.unauthenticated, missing role gives 403 auth.forbidden
```

## 5. Runtime flow — migrations

```mermaid
sequenceDiagram
    participant U as scripts/migrate.sh
    participant M as modulo_migrate
    participant G as Migrator (db module)
    participant P as PostgreSQL 16

    U->>M: MODULO_DB_URL + --dir db/migrations
    M->>G: run()
    G->>G: discover() — NNNN_name.sql, sorted, dup check
    G->>P: ensure schema_migrations
    loop each migration (one transaction)
        G->>P: md5(content) — checksum via Postgres
        alt already applied, checksum matches
            G->>G: skip
        else checksum differs
            G-->>M: MigrationError (append-only violated)
        else pending
            G->>P: apply SQL + record row, commit
        end
    end
    M-->>U: "N applied, M skipped" (exit code)
```

## 6. Data model

```mermaid
erDiagram
    users {
        uuid id PK
        citext email UK "case-insensitive"
        text display_name
        text password_hash "Argon2id (libsodium)"
        timestamptz created_at
        timestamptz updated_at "trigger set_updated_at"
        timestamptz disabled_at "null = active"
    }
    roles {
        smallint id PK
        text name UK "admin, user"
    }
    user_roles {
        uuid user_id PK, FK
        smallint role_id PK, FK
    }
    sessions {
        uuid id PK
        uuid user_id FK
        bytea token_sha256 UK "digest only, 32 bytes"
        timestamptz created_at
        timestamptz expires_at "sliding 30 days"
        timestamptz last_seen_at
        timestamptz revoked_at "null = active"
    }
    meta {
        text key PK
        text value
        timestamptz updated_at
    }
    schema_migrations {
        integer version PK
        text name
        text checksum "md5 of file"
        timestamptz applied_at
    }

    users ||--o{ user_roles : "has"
    roles ||--o{ user_roles : "granted as"
    users ||--o{ sessions : "owns (cascade delete)"
```

Migrations so far: `0001_init` (meta), `0002_auth` (users, roles, user_roles, sessions, `set_updated_at()` trigger,
`citext` extension). `schema_migrations` is not created by a migration file — the migration engine
(`modules/db`, `migrator.cpp`) creates it on first run and owns it. Tokens are never stored: the server keeps only the SHA-256 digest, so a database leak cannot
be replayed as a login.

## 7. Test architecture

```mermaid
flowchart LR
    subgraph unit["label: unit — Qt Test, no Docker"]
        t1["modulo_core_tests"]
        t2["modulo_api_health_dto_tests
modulo_api_error_dto_tests
modulo_api_json_tests"]
        t3["modulo_server_config_tests"]
        t6["modulo_core_password_policy_tests
modulo_server_auth_*_tests
(hasher · token · roles)"]
    end
    subgraph integ["label: integration — opt-in"]
        t4["modulo_integration_tests
in-process QHttpServer on port 0
+ QNetworkAccessManager client"]
        t7["modulo_auth_repositories_tests
ConnectionPool + repositories
against modulo_test"]
        t8["modulo_auth_flow_tests
auth endpoints over real HTTP"]
    end
    subgraph ui["label: ui — Qt Quick Test, offscreen"]
        t5["modulo_client_qml_tests
tst_*.qml via QUICK_TEST_MAIN"]
    end

    env["MODULO_TEST_DB_URL"] -. "unset → QSKIP → CTest Skipped" .-> t4
    env -.-> t7
    env -.-> t8
    support["tests/support/include/modulo/testing/
integration.h: MODULO_REQUIRE_TEST_DATABASE(), httpGet()"] --> t4

    presets["ctest --preset unit | integration | ui | all"] --> unit
    presets --> integ
    presets --> ui
```

Conventions: one `QObject` test class per binary (`QTEST_GUILESS_MAIN`), data-driven rows via
`_data()` slots; each target's `tests/` directory is auto-discovered by the CMake toolkit, which
links `Qt6::Test`, adds the shared support include dir, and maps Qt Test's `SKIP   :` output to
CTest's *Skipped* status. Cross-module integration tests live only in `server/tests/integration/`.
