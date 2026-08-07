# Modulo

A personal investment tracker for crypto and stock assets — transactions, bank↔exchange
transfers, aggregated holdings with dashboards, uploaded documents, and daily exchange-rate
updates.

**Architecture:** client-server. A C++23 REST backend (Qt `QHttpServer`) owns PostgreSQL,
authentication sessions, and business logic; a Qt 6 / QML desktop client for macOS consumes
the API. The backend is designed to be containerized later, and future web/mobile clients
can target the same API.

**Stack:** C++23 · Qt 6.8 · QML · PostgreSQL 16 · CMake ≥ 3.28 · libpqxx · libsodium ·
Catch2 v3 · nlohmann-json

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
- The local Homebrew PostgreSQL (if any) can keep running — the dockerized database uses
  port **5433** precisely to avoid clashing with a local server on 5432.

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

Build directories land in `build/<preset>/`. Third-party sources fetched by CPM are cached
in `.cache/cpm/` and survive build-directory wipes.

All build logic lives as `modulo_*` functions in [`cmake/`](cmake/) —
`modulo_add_library`, `modulo_add_executable`, `modulo_add_test`, `modulo_add_qml_test` —
so every `CMakeLists.txt` stays a short declarative call. Each server-side module is its
own static library with public headers in `include/modulo/...` and implementation in
`src/` (header files use the `.h` extension).

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

## Testing

Tests are registered with CTest under the labels `unit`, `integration`, and `ui`:

```sh
ctest --preset unit           # fast, no Docker needed
ctest --preset integration    # requires the database (scripts/db-up.sh)
ctest --preset ui             # QML/Qt Quick tests
ctest --preset all
```

(No tests exist yet — the first scaffolding tests arrive with the test-setup step of
Increment 1.)

## Code style

- [`.clang-format`](.clang-format) — LLVM base, 4-space indent, 120 columns, `int* p`
  pointer style, include groups ordered local → `<modulo/...>` → Qt → third-party → std.
- [`.clang-tidy`](.clang-tidy) — `bugprone-*`, `performance-*`, `modernize-*`,
  `readability-*` plus naming rules (`CamelCase` types, `camelBack` functions/variables,
  trailing-underscore private members).

```sh
scripts/format.sh             # format all first-party sources in place
scripts/format.sh --check     # verify only (CI mode)
```

## Repository layout

```
cmake/            CMake toolkit: all build logic as modulo_* functions + vendored CPM.cmake
docker/           docker-compose.yml (Postgres 16 on :5433) + one-time initdb scripts
scripts/          db-up.sh, db-down.sh, format.sh
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
