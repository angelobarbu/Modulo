# Modulo

A personal investment tracker for crypto and stock assets — transactions, bank↔exchange
transfers, aggregated holdings with dashboards, uploaded documents, and daily exchange-rate
updates.

**Architecture:** client-server. A C++23 REST backend (Qt `QHttpServer`) owns PostgreSQL,
authentication sessions and business logic; a Qt 6 / QML desktop client for macOS consumes
the API. The backend is designed to be containerized later and future web/mobile clients
can target the same API.

**Stack:** C++23 · Qt 6.8 · QML · PostgreSQL 16 · CMake ≥ 3.28 · libpqxx · libsodium · Testing: Qt Test / Qt Quick Test (Client UI)

The project maximizes Qt framework usage — Qt is used everywhere unless it is clearly
costly and an alternative is much more efficient: QJson wire format, `Q_GADGET` DTOs readable
from QML, `QString` + `.arg()` as the project-wide string idiom, Qt integer typedefs
(`quint16`, etc.) in Qt-facing code, `qInfo()`/`qCritical()` logging in applications
(`QLoggingCategory` planned with the auth increment), Qt Test for every test suite, and Qt
networking/HTTP/UI throughout. The C++23 standard library is used only where Qt has no equivalent
(`std::expected`-based `Result<T>`, `std::filesystem`). The Qt-free zone is
`server/modules/db` + `modulo_migrate` (pure libpqxx; stdout is the CLI's interface),
keeping the future container's migration entrypoint minimal.

> Developed incrementally, one reviewed step at a time. This README grows with each step —
> see [Repository layout](#repository-layout) for what exists today.

## Prerequisites

One-time setup on macOS (Apple Silicon):

```sh
brew install cmake ninja llvm libpqxx libsodium qt
```

- **Qt 6.8+** is expected at `/opt/homebrew/opt/qt` (the CMake presets bake this path in).
- **llvm** provides `clang-format`/`clang-tidy`; it is keg-only, so scripts and CMake
  reference `/opt/homebrew/opt/llvm/bin` by absolute path.
- **Docker Desktop** must be running for the development database.
- The local Homebrew PostgreSQL (if any) can run in parallel - the dockerized database uses
  port **5433** precisely to avoid clashing with a local server on 5432.

Two quirks of this machine are compensated for in the build (no action needed):

- The newest macOS SDK no longer ships the legacy `AGL` framework, but Qt's OpenGL CMake
  wrapper unconditionally links it - the `dev` presets pin `WrapOpenGL_AGL` to the stub in
  the older SDK (Homebrew's Qt itself links AGL at runtime, so this adds nothing new).
- A second Qt (`qtbase`) shadows the shared Homebrew plugin path with version-incompatible
  plugins; the build generates a `qt.conf` beside every executable pinning plugin/QML
  resolution to the Qt actually linked.

Copy the environment template and adjust if needed:

```sh
cp .env.example .env
```

## Building

The build is driven entirely by CMake presets:

```sh
cmake --preset dev            # configure (Debug, warnings-as-errors)
cmake --build --preset dev    # build
```

| Configure preset | Purpose |
|---|---|
| `dev` | Debug build, `-Werror`, compile-commands export |
| `dev-asan` | `dev` + address & undefined-behavior sanitizers |
| `dev-tidy` | `dev` + clang-tidy on every compile |
| `release` | RelWithDebInfo |

Build directories are generated in `build/<preset>/`. All dependencies are Homebrew binary
libraries — nothing is downloaded at configure time.

All build logic can be found in `modulo_*` functions under [`cmake/`](cmake/) module -
`modulo_add_library`, `modulo_add_executable`, `modulo_add_test`, `modulo_add_qml_test`.
Thus, `CMakeLists.txt` becomes a short declarative call. Each server-side module is its
own static library with public headers in `include/modulo/...` and implementation in
`src/`.

## Running the stack

```sh
scripts/db-up.sh                      # 1. database (not needed by health yet)
./build/dev/server/app/modulo_server  # 2. REST API on http://127.0.0.1:8080
./build/dev/client/modulo_client      # 3. desktop client (separate terminal)
```

The server exposes `GET /api/v1/health` → `{"status":"ok","version":"0.1.0"}`; any
unknown route returns the uniform error envelope
`{"error":{"code":"not_found","message":"..."}}` with the matching HTTP status. The
client window (placeholder) polls health every 3 s and shows a live
green/red status indicator. `MODULO_HTTP_PORT` and `MODULO_API_URL` override the
server port and the client's target.

## Development database

Postgres 16 runs in Docker with a persistent named volume:

```sh
scripts/db-up.sh              # start + wait until healthy
scripts/db-down.sh            # stop (data preserved)
scripts/db-down.sh --wipe     # stop AND delete all data (asks for confirmation)
```

| What | Value |
|---|---|
| Host/port | `localhost:5433` (bound to 127.0.0.1 only) |
| Databases | `modulo_dev` (development), `modulo_test` (integration tests) |
| Credentials | user `modulo`, password `modulo` (dev-only) |
| Volume | `modulo_pgdata` |

`modulo_test` is created by [`docker/initdb/01_create_test_db.sql`](docker/initdb/01_create_test_db.sql)
on the first initialization of an empty volume.

### Migrations

Schema changes are plain SQL files in [`db/migrations/`](db/migrations/), named
`NNNN_name.sql` and applied in version order by the `modulo_migrate` binary
(module `modulo_server_db`, wrapped by a script):

```sh
scripts/migrate.sh            # applies pending migrations to $MODULO_DB_URL
```

The runner tracks state in a `schema_migrations` table (version, name, content
checksum, timestamp) and enforces these rules:

- each migration runs inside **one transaction** — a failure rolls back cleanly;
- already-applied, unchanged files are **skipped** (re-running is a no-op);
- migration files are **append-only**: editing an applied file changes its
  checksum and the runner refuses to continue;
- a stray non-migration file in the directory is an error (dotfiles are tolerated).

The database URL resolves in order: existing `MODULO_DB_URL` in the environment →
`.env` at the repo root → the dev-database default. `modulo_migrate --help` shows
the underlying CLI (`--url`, `--dir`).

## Testing

Tests are registered with CTest under the labels `unit`, `integration`, and `ui`:

```sh
ctest --preset unit           # Qt Test, fast, no Docker needed
ctest --preset integration    # opt-in: set MODULO_TEST_DB_URL (see .env.example)
ctest --preset ui             # Qt Quick Test, runs offscreen automatically
ctest --preset all
```

All suites use **Qt Test** (C++) and **Qt Quick Test** (QML) — one QObject test class per
binary, data-driven rows via `_data()` slots:

| Test binary | Label | What it covers |
|---|---|---|
| `modulo_core_tests` | unit | `version()` matches the CMake project version, semver shape |
| `modulo_api_health_dto_tests` | unit | `HealthResponse` JSON round-trip; `fromJson` rejecting missing/mistyped fields |
| `modulo_api_error_dto_tests` | unit | `ErrorResponse` envelope shape and round-trip; rejection of flat/incomplete envelopes |
| `modulo_api_json_tests` | unit | `api::json::require*` never falling back to QJson's silent defaults (missing, number, object, array, null) |
| `modulo_server_config_tests` | unit | defaults, every variable, empty-means-unset, port 0, malformed ports → `config.invalid_port` |
| `modulo_integration_tests` | integration | real `QHttpServer` on an OS-assigned port + real HTTP client: `/api/v1/health` body and version, 404 error envelope |
| `modulo_client_qml_tests` | ui | `QUICK_TEST_MAIN` runner over `client/tests/qml/tst_*.qml` (Qt Quick + Material smoke) |

Conventions: every module's tests live in its own `tests/` directory (auto-discovered by
the CMake toolkit); cross-module integration tests live in `server/tests/integration/`;
shared fixtures are in `tests/support/include/modulo/testing/`. Integration tests are
**opt-in**: they start with `MODULO_REQUIRE_TEST_DATABASE()`, which `QSKIP`s without
`MODULO_TEST_DB_URL`, and CTest reports the binary as *Skipped* — so `ctest --preset all`
never needs Docker to pass.

## Code style

- [`.clang-format`](.clang-format) — LLVM base, 4-space indent, 120 columns, `int* p`
  pointer style, include groups ordered local → `<modulo/...>` → Qt → third-party → std.
- [`.clang-tidy`](.clang-tidy) — `bugprone-*`, `performance-*`, `modernize-*`,
  `readability-*` plus naming rules (`CamelCase` types, `camelBack` functions/variables,
  trailing-underscore private members).

```sh
scripts/format.sh             # format all sources in place
scripts/format.sh --check     # verify only (CI mode)
```

## Repository layout

```
cmake/            CMake toolkit: all build logic as modulo_* functions
db/migrations/    append-only SQL schema migrations (NNNN_name.sql)
docs/             high_level_design.md (Architecture diagrams)
docker/           docker-compose.yml (Postgres 16 on :5433) + one-time initdb scripts
libs/core/        modulo_core — Qt-free foundations (version, Result<T> on std::expected)
libs/api/         modulo_api — Q_GADGET DTOs + validating QJson mappings shared by server and client
scripts/          db-up.sh, db-down.sh, migrate.sh, format.sh
tests/support/    shared test fixtures (<modulo/testing/...>) for integration tests
server/           backend: per-module static libraries + executables (each module has its own tests/)
  modules/config/ modulo_server_config — env-based process configuration
  modules/db/     modulo_server_db — migration engine (connection pool arrives in Increment 2)
  modules/http/   modulo_server_http — QHttpServer wrapper, routes, error envelope
  app/            modulo_server — REST API server executable
  migrate/        modulo_migrate — CLI migration runner
client/           modulo_client — QML desktop app (ApiClient + dark-theme shell)
CMakeLists.txt    thin root: options, toolkit includes, dependency resolution
CMakePresets.json configure/build/test presets (dev, dev-asan, dev-tidy, release)
.env.example      environment template (DB URLs, HTTP port, data dir)
```

## Implementation log

| Increment / step | Delivered |
|---|---|
| 1.0 — Prerequisites | Toolchain verified: ninja, llvm 22, libpqxx 8, libsodium, Qt 6.8.2 (no QPSQL driver → libpqxx), Docker |
| 1.1 — Style & hygiene | `.clang-format`, `.clang-tidy`, `.env.example`, `.gitignore` extension, `scripts/format.sh` |
| 1.2 — CMake superstructure | Function-based `cmake/` toolkit, vendored CPM v0.42.0, thin root `CMakeLists.txt`, presets |
| 1.3 — Dev database | Dockerized Postgres 16 (`:5433`, named volume, healthcheck), initdb for `modulo_test`, `db-up`/`db-down` scripts |
| 1.4 — Migrations | `modulo_server_db` module (first server static lib) with transactional, checksum-verified migration engine; `modulo_migrate` CLI; `0001_init.sql`; `scripts/migrate.sh` |
| 1.5 — Stubs across the stack | `modulo_core` (version, `Result<T>`), `modulo_api` (Health/Error DTOs), `config` + `http` server modules, `modulo_server` serving `/api/v1/health`, QML client with live status; toolkit grew `modulo_add_qml_app`, version injection, qt.conf generation, AGL workaround |
| 1.5b — Qt-wide uniformity | Decision: maximize Qt uniformity. DTOs became `Q_GADGET`s with validating QJson mappings (`api::json::require*` — no silent defaults); nlohmann-json dependency removed. `QString` project-wide (incl. `core::Error`/`version()`), `.arg()` over `std::format` in Qt code, `quint16` in Qt-facing types, `qInfo`/`qCritical` in apps; Qt-free zone narrowed to `modules/db` + `modulo_migrate` |
| 1.5c — Cleanup | Further code & comments cleanup; revisioned documentation |
| 1.6 — Test scaffolding | One passing suite per layer: core, api (DTO + `require*` rejection paths), config, in-process HTTP integration (opt-in via `MODULO_TEST_DB_URL`, Skipped otherwise), QML smoke (offscreen); toolkit auto-discovers `tests/` dirs |
| 1.6b — Qt Test everywhere | Decision: Qt Test replaces Catch2 (one framework for C++ and QML); Catch2 + CPM removed — the project now has zero source-level dependencies |
