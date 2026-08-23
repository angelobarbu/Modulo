#include "pg.h"

#include <modulo/server/auth/session_repository.h>

namespace modulo::server::auth {

namespace {

const std::string kSessionColumns = "id::text, user_id::text, " + pg::epochMs("created_at") + ", " +
                                    pg::epochMs("expires_at") + ", " + pg::epochMs("last_seen_at") +
                                    ", (revoked_at IS NOT NULL)";

SessionRecord toRecord(const pqxx::row& row) {
    SessionRecord record;
    record.id = pg::fromStd(row[0].as<std::string>());
    record.userId = pg::fromStd(row[1].as<std::string>());
    record.createdAt = pg::fromEpochMs(row[2].as<long long>());
    record.expiresAt = pg::fromEpochMs(row[3].as<long long>());
    record.lastSeenAt = pg::fromEpochMs(row[4].as<long long>());
    record.revoked = row[5].as<bool>();
    return record;
}

core::VoidResult requireOneRow(const pqxx::result& result) {
    if (result.affected_rows() == 0) {
        return core::makeError(QStringLiteral("auth.session_not_found"),
                               QStringLiteral("session does not exist or is already revoked"));
    }
    return {};
}

} // namespace

SessionRepository::SessionRepository(db::ConnectionPool& pool) : pool_{pool} {
}

core::Result<SessionRecord> SessionRepository::create(const QString& userId, const QByteArray& tokenDigest,
                                                      const QDateTime& expiresAt) {
    return pg::withTransaction(pool_, [&](pqxx::work& tx) {
        const auto row = tx.exec("INSERT INTO sessions (user_id, token_sha256, expires_at) "
                                 "VALUES ($1::uuid, $2, $3::timestamptz) RETURNING " +
                                     kSessionColumns,
                                 pqxx::params{pg::toStd(userId), pg::toBytes(tokenDigest), pg::toIso(expiresAt)})
                             .one_row();
        return toRecord(row);
    });
}

core::Result<std::optional<SessionRecord>> SessionRepository::findActiveByDigest(const QByteArray& tokenDigest) {
    return pg::withTransaction(pool_, [&](pqxx::work& tx) -> std::optional<SessionRecord> {
        const auto rows = tx.exec("SELECT " + kSessionColumns +
                                      " FROM sessions WHERE token_sha256 = $1 AND revoked_at IS NULL "
                                      "AND expires_at > now()",
                                  pqxx::params{pg::toBytes(tokenDigest)});
        if (rows.empty()) {
            return std::nullopt;
        }
        return toRecord(rows.one_row());
    });
}

core::VoidResult SessionRepository::touch(const QString& sessionId, const QDateTime& newExpiresAt) {
    auto result = pg::withTransaction(pool_, [&](pqxx::work& tx) {
        return requireOneRow(tx.exec("UPDATE sessions SET last_seen_at = now(), expires_at = $2::timestamptz "
                                     "WHERE id = $1::uuid AND revoked_at IS NULL",
                                     pqxx::params{pg::toStd(sessionId), pg::toIso(newExpiresAt)}));
    });
    if (!result) {
        return std::unexpected{result.error()};
    }
    return *result;
}

core::VoidResult SessionRepository::revoke(const QString& sessionId) {
    auto result = pg::withTransaction(pool_, [&](pqxx::work& tx) {
        return requireOneRow(
            tx.exec("UPDATE sessions SET revoked_at = now() WHERE id = $1::uuid AND revoked_at IS NULL",
                    pqxx::params{pg::toStd(sessionId)}));
    });
    if (!result) {
        return std::unexpected{result.error()};
    }
    return *result;
}

core::Result<qint64> SessionRepository::revokeAllForUser(const QString& userId) {
    return pg::withTransaction(pool_, [&](pqxx::work& tx) {
        return static_cast<qint64>(
            tx.exec("UPDATE sessions SET revoked_at = now() WHERE user_id = $1::uuid AND revoked_at IS NULL",
                    pqxx::params{pg::toStd(userId)})
                .affected_rows());
    });
}

} // namespace modulo::server::auth
