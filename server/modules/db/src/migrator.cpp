#include <modulo/server/db/migrator.h>

#include <pqxx/pqxx>

#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>
#include <utility>

namespace modulo::server::db {

namespace {

constexpr std::string_view kCreateSchemaMigrations = R"sql(
    CREATE TABLE IF NOT EXISTS schema_migrations (
        version integer PRIMARY KEY,
        name text NOT NULL,
        checksum text NOT NULL,
        applied_at timestamptz NOT NULL DEFAULT now()
    )
)sql";

/// Parse "NNNN_name.sql" into (version, name); std::nullopt if the pattern
/// does not match.
std::optional<std::pair<int, std::string>> parseFilename(const std::string& filename) {
    constexpr std::string_view kSuffix = ".sql";
    constexpr std::size_t kVersionDigits = 4;
    // Shortest valid: "0000_x.sql"
    if (filename.size() < kVersionDigits + 1 + 1 + kSuffix.size() || !filename.ends_with(kSuffix)) {
        return std::nullopt;
    }
    if (filename[kVersionDigits] != '_') {
        return std::nullopt;
    }

    int version = 0;
    const auto [ptr, ec] = std::from_chars(filename.data(), filename.data() + kVersionDigits, version);
    if (ec != std::errc{} || ptr != filename.data() + kVersionDigits) {
        return std::nullopt;
    }

    std::string name = filename.substr(kVersionDigits + 1, filename.size() - kVersionDigits - 1 - kSuffix.size());
    return std::pair{version, std::move(name)};
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    if (!stream) {
        throw MigrationError(std::format("cannot read migration file '{}'", path.string()));
    }
    std::ostringstream contents;
    contents << stream.rdbuf();
    return std::move(contents).str();
}

/// Content checksum, computed by PostgreSQL itself (md5 is fine here: this
/// detects accidental edits of applied files, it is not a security boundary).
std::string checksumOf(pqxx::work& tx, const std::string& sql) {
    return tx.query_value<std::string>("SELECT md5($1)", pqxx::params{sql});
}

} // namespace

Migrator::Migrator(std::string databaseUrl, std::filesystem::path migrationsDir, Logger logger)
    : databaseUrl_{std::move(databaseUrl)}, migrationsDir_{std::move(migrationsDir)}, logger_{std::move(logger)} {
}

std::vector<Migration> Migrator::discover() const {
    if (!std::filesystem::is_directory(migrationsDir_)) {
        throw MigrationError(std::format("migrations directory '{}' does not exist", migrationsDir_.string()));
    }

    std::vector<Migration> migrations;
    for (const auto& entry : std::filesystem::directory_iterator{migrationsDir_}) {
        const std::string filename = entry.path().filename().string();
        if (filename.starts_with('.')) {
            continue; // tolerate .DS_Store and friends
        }

        auto parsed = parseFilename(filename);
        if (!parsed || !entry.is_regular_file()) {
            throw MigrationError(
                std::format("unexpected file '{}' in migrations directory (expected NNNN_name.sql)", filename));
        }
        migrations.push_back({.version = parsed->first, .name = std::move(parsed->second), .path = entry.path()});
    }

    std::ranges::sort(migrations, {}, &Migration::version);

    const auto duplicate = std::ranges::adjacent_find(migrations, {}, &Migration::version);
    if (duplicate != migrations.end()) {
        throw MigrationError(std::format("duplicate migration version {:04}", duplicate->version));
    }

    return migrations;
}

MigrationResult Migrator::run() {
    const auto migrations = discover();

    pqxx::connection connection{databaseUrl_};

    {
        pqxx::work tx{connection};
        tx.exec(kCreateSchemaMigrations);
        tx.commit();
    }

    MigrationResult result;
    for (const auto& migration : migrations) {
        pqxx::work tx{connection};

        const std::string sql = readFile(migration.path);
        const std::string checksum = checksumOf(tx, sql);

        const auto known =
            tx.exec("SELECT checksum FROM schema_migrations WHERE version = $1", pqxx::params{migration.version});
        if (!known.empty()) {
            if (known[0][0].as<std::string>() != checksum) {
                throw MigrationError(std::format("migration {:04}_{} was applied with a different content checksum; "
                                                 "applied migration files are append-only and must never be edited",
                                                 migration.version, migration.name));
            }
            ++result.skipped;
            continue; // transaction aborts harmlessly
        }

        try {
            tx.exec(sql);
            tx.exec("INSERT INTO schema_migrations (version, name, checksum) VALUES ($1, $2, $3)",
                    pqxx::params{migration.version, migration.name, checksum});
            tx.commit();
        } catch (const pqxx::sql_error& error) {
            throw MigrationError(std::format("migration {:04}_{} failed and was rolled back: {}", migration.version,
                                             migration.name, error.what()));
        }

        ++result.applied;
        log(std::format("applied {:04}_{}", migration.version, migration.name));
    }

    return result;
}

void Migrator::log(std::string_view message) const {
    if (logger_) {
        logger_(message);
    }
}

} // namespace modulo::server::db
