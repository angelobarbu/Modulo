#include "pg.h"

#include <modulo/server/auth/user_repository.h>

#include <QUuid>

namespace modulo::server::auth {

namespace {

const std::string kUserColumns =
    "id::text, email::text, display_name, password_hash, " + pg::epochMs("created_at") + ", (disabled_at IS NOT NULL)";

QList<Role> loadRoles(pqxx::work& tx, const std::string& userId) {
    QList<Role> roles;
    for (const auto& row :
         tx.exec("SELECT role_id FROM user_roles WHERE user_id = $1::uuid ORDER BY role_id", pqxx::params{userId})) {
        if (const auto role = roleFromId(row[0].as<qint16>())) {
            roles.append(*role);
        }
    }
    return roles;
}

UserRecord toRecord(pqxx::work& tx, const pqxx::row& row) {
    UserRecord record;
    record.id = pg::fromStd(row[0].as<std::string>());
    record.email = pg::fromStd(row[1].as<std::string>());
    record.displayName = pg::fromStd(row[2].as<std::string>());
    record.passwordHash = pg::fromStd(row[3].as<std::string>());
    record.createdAt = pg::fromEpochMs(row[4].as<long long>());
    record.disabled = row[5].as<bool>();
    record.roles = loadRoles(tx, row[0].as<std::string>());
    return record;
}

std::optional<UserRecord> findOne(pqxx::work& tx, const std::string& whereClause, const pqxx::params& params) {
    const auto rows = tx.exec("SELECT " + kUserColumns + " FROM users WHERE " + whereClause, params);
    if (rows.empty()) {
        return std::nullopt;
    }
    return toRecord(tx, rows.one_row());
}

} // namespace

UserRepository::UserRepository(db::ConnectionPool& pool) : pool_{pool} {
}

core::Result<UserRecord> UserRepository::create(const QString& email, const QString& displayName,
                                                const QString& passwordHash, const QList<Role>& roles) {
    auto result = pg::withTransaction(pool_, [&](pqxx::work& tx) {
        const auto row = tx.exec("INSERT INTO users (email, display_name, password_hash) VALUES ($1, $2, $3) "
                                 "RETURNING " +
                                     kUserColumns,
                                 pqxx::params{pg::toStd(email), pg::toStd(displayName), pg::toStd(passwordHash)})
                             .one_row();
        const std::string userId = row[0].as<std::string>();
        // Idempotent: a repeated role is not an error, so the only unique
        // violation this transaction can raise is the email one.
        for (const Role role : roles) {
            tx.exec("INSERT INTO user_roles (user_id, role_id) VALUES ($1::uuid, $2) ON CONFLICT DO NOTHING",
                    pqxx::params{userId, static_cast<qint16>(role)});
        }
        return toRecord(tx, row);
    });

    if (!result && result.error().code == QStringLiteral("db.unique_violation")) {
        return core::makeError(QStringLiteral("auth.email_taken"),
                               QStringLiteral("an account with this email already exists"));
    }
    return result;
}

core::Result<std::optional<UserRecord>> UserRepository::findByEmail(const QString& email) {
    return pg::withTransaction(
        pool_, [&](pqxx::work& tx) { return findOne(tx, "email = $1::citext", pqxx::params{pg::toStd(email)}); });
}

core::Result<std::optional<UserRecord>> UserRepository::findById(const QString& id) {
    // A malformed id can never match a row; reject it here instead of letting
    // the $1::uuid cast fail inside PostgreSQL and surface as db.query_failed.
    if (QUuid::fromString(id).isNull()) {
        return std::optional<UserRecord>{};
    }
    return pg::withTransaction(
        pool_, [&](pqxx::work& tx) { return findOne(tx, "id = $1::uuid", pqxx::params{pg::toStd(id)}); });
}

core::Result<qint64> UserRepository::count() {
    return pg::withTransaction(pool_, [](pqxx::work& tx) {
        return static_cast<qint64>(tx.query_value<long long>("SELECT count(*) FROM users"));
    });
}

} // namespace modulo::server::auth
