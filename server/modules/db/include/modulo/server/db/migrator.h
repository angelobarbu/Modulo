#pragma once

#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace modulo::server::db {

/// Thrown when migration discovery or application fails. The migration that
/// caused the failure is named in the message; the database is left as of the
/// last successfully committed migration.
class MigrationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// A migration file discovered on disk. Files live in db/migrations/ and are
/// named NNNN_name.sql (four-digit version, underscore, snake_case name).
struct Migration {
    int version = 0;
    std::string name;
    std::filesystem::path path;
};

/// Outcome of a Migrator::run() invocation.
struct MigrationResult {
    int applied = 0;
    int skipped = 0;
};

/// Applies SQL migration files to a PostgreSQL database.
///
/// State is tracked in the schema_migrations table (created on demand):
/// one row per applied migration with its version, name, content checksum,
/// and timestamp. Rules:
///   - migrations run in ascending version order, each inside one transaction;
///   - an already-applied migration whose file is unchanged is skipped;
///   - an already-applied migration whose file content CHANGED aborts the run
///     (migrations are append-only — never edit an applied file);
///   - a failing migration rolls back and aborts; nothing after it runs.
class Migrator {
public:
    /// Receives one human-readable progress line per migration.
    using Logger = std::function<void(std::string_view)>;

    Migrator(std::string databaseUrl, std::filesystem::path migrationsDir, Logger logger = {});

    /// Scan the migrations directory. Non-dot files that do not match the
    /// NNNN_name.sql pattern, and duplicate versions, raise MigrationError.
    /// Returns migrations sorted by ascending version.
    [[nodiscard]] std::vector<Migration> discover() const;

    /// Apply every pending migration. Throws MigrationError (see class docs)
    /// or pqxx errors on connection failure.
    MigrationResult run();

private:
    void log(std::string_view message) const;

    std::string databaseUrl_;
    std::filesystem::path migrationsDir_;
    Logger logger_;
};

} // namespace modulo::server::db
