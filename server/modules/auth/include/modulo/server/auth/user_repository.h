#pragma once

#include <modulo/core/result.h>
#include <modulo/server/auth/roles.h>
#include <modulo/server/db/connection_pool.h>

#include <QDateTime>
#include <QList>
#include <QString>

#include <optional>

namespace modulo::server::auth {

/// A row of `users` plus its roles. `passwordHash` is the Argon2id string
/// (never exposed on the wire - the service layer maps to DTOs).
struct UserRecord {
    QString id; ///< uuid as text
    QString email;
    QString displayName;
    QString passwordHash;
    QDateTime createdAt;
    bool disabled = false;
    QList<Role> roles;
};

/// Data access for users and their roles. Every method runs in its own
/// transaction on a pooled connection and returns Result - it never throws.
/// Error codes: "auth.email_taken" (duplicate email, case-insensitive),
/// "db.*" for infrastructure failures.
class UserRepository {
public:
    explicit UserRepository(db::ConnectionPool& pool);

    /// Inserts the user and its role assignments atomically.
    core::Result<UserRecord> create(const QString& email, const QString& displayName, const QString& passwordHash,
                                    const QList<Role>& roles);

    /// Case-insensitive lookup (email is citext). nullopt = no such user.
    core::Result<std::optional<UserRecord>> findByEmail(const QString& email);

    core::Result<std::optional<UserRecord>> findById(const QString& id);

    /// Total number of users (the first registration becomes admin).
    core::Result<qint64> count();

private:
    db::ConnectionPool& pool_;
};

} // namespace modulo::server::auth
