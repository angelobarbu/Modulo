#pragma once

#include <modulo/core/result.h>
#include <modulo/server/db/connection_pool.h>

#include <QByteArray>
#include <QDateTime>
#include <QString>

#include <optional>

namespace modulo::server::auth {

/// A row of `sessions`. The token itself is never stored; rows are found by
/// the SHA-256 digest of the token the client presents.
struct SessionRecord {
    QString id; ///< uuid as text
    QString userId;
    QDateTime createdAt;
    QDateTime expiresAt;
    QDateTime lastSeenAt;
    bool revoked = false;
};

/// Data access for sessions. Every method runs in its own transaction on a
/// pooled connection and returns Result - it never throws.
/// Error codes: "auth.session_not_found" (touch/revoke on a missing or already
/// revoked session), "db.*" for infrastructure failures.
class SessionRepository {
public:
    explicit SessionRepository(db::ConnectionPool& pool);

    /// `tokenDigest` must be exactly 32 bytes (token::digest output).
    core::Result<SessionRecord> create(const QString& userId, const QByteArray& tokenDigest,
                                       const QDateTime& expiresAt);

    /// Only sessions that are neither revoked nor expired. nullopt otherwise.
    core::Result<std::optional<SessionRecord>> findActiveByDigest(const QByteArray& tokenDigest);

    /// Sliding expiry: records activity and pushes expires_at forward.
    core::VoidResult touch(const QString& sessionId, const QDateTime& newExpiresAt);

    core::VoidResult revoke(const QString& sessionId);

    /// "Log out everywhere": returns the number of sessions revoked.
    core::Result<qint64> revokeAllForUser(const QString& userId);

private:
    db::ConnectionPool& pool_;
};

} // namespace modulo::server::auth
