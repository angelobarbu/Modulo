# Modulo — High-Level Design (end of Increment 1, Step 5)

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
QHttpServer · routes
JSON error envelope"]
            config["config module
env → Config
(MODULO_* vars)"]
            http --> config
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

    apiclient -- "HTTP GET /api/v1/health
127.0.0.1:8080 (loopback only)" --> http
    migrator -- "SQL over libpq" --> pg
    sql --> migrator
    http -. "libpqxx pool — Increment 2" .-> pg
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
    httpm["modulo_server_http"]
    dbm["modulo_server_db
(Qt-free · libpqxx)"]

    server(["modulo_server (exe)"])
    migrateexe(["modulo_migrate (exe, Qt-free)"])
    clientexe(["modulo_client (exe)"])

    api --> core
    cfg --> core
    httpm --> api
    httpm --> cfg
    server --> httpm
    migrateexe --> dbm
    clientexe --> api

    qt["Qt6: Core · Network · HttpServer · Quick"]
    pqxx["libpqxx 8"]
    httpm -.-> qt
    clientexe -.-> qt
    core -.-> qt
    dbm -.-> pqxx
```

Every server-side module is its own static library (`CMakeLists.txt` + `include/modulo/...` + `src/`),
created by the `modulo_*` CMake toolkit functions (warnings, sanitizers, clang-tidy, version
injection, qt.conf generation applied uniformly). Coming next: `modulo_server_auth` (Increment 2),
then transactions / transfers / holdings / rates / documents as sibling modules.

## 3. Runtime flow — health check (the pipe proven in Step 5)

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
    D-->>A: Result&lt;HealthResponse&gt;
    A-->>Q: serverReachable / serverStatus properties
    Note over Q: green pulsing dot · "server ok (v0.1.0)"
```

## 4. Runtime flow — migrations

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

**Conventions locked in so far:** full-Qt uniformity (QJson wire format, `QString`/`.arg()`,
`quint16`, `qInfo`/`qCritical`; `QLoggingCategory` from Increment 2) with `std::expected`/
`std::filesystem` where Qt has no equivalent; the only Qt-free zone is `modules/db` +
`modulo_migrate`; uniform error envelope `{"error":{"code","message"}}`; loopback-only
binding; append-only migrations; one step per prompt, README updated every step.
