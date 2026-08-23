#pragma once

#include <modulo/core/result.h>
#include <modulo/server/auth/roles.h>
#include <modulo/server/auth/session_repository.h>
#include <modulo/server/auth/user_repository.h>
#include <modulo/server/db/connection_pool.h>

#include <QList>
#include <QString>

#include <optional>

namespace modulo::server::auth {

/// Who is making an authenticated request. Built by AuthService::authenticate
/// from a bearer token and handed to route handlers by the HTTP guards.
struct AuthContext {
    QString userId;
    QString sessionId;
    QList<Role> roles;

    bool hasRole(Role role) const { return roles.contains(role); }
};

struct LoginResult {
    QString token; ///< Opaque bearer token; shown to the client exactly once.
    UserRecord user;
};

struct AuthOptions {
    /// Whether accounts beyond the first may register. The first account is
    /// always allowed (it becomes admin), otherwise nobody could log in.
    bool allowRegistration = true;
    /// Session lifetime; authenticated requests slide it forward.
    int sessionDays = 30;
};

/// Authentication use cases on top of the repositories: registration, login,
/// logout and bearer-token authentication with a sliding session expiry.
///
/// Error codes (HTTP status is assigned by the http module):
///   auth.invalid_email, auth.invalid_display_name, password.too_short/too_long,
///   auth.registration_disabled, auth.email_taken, auth.invalid_credentials,
///   auth.session_not_found, db.*.
class AuthService {
public:
    AuthService(db::ConnectionPool& pool, AuthOptions options = {});

    /// The first account receives admin + user, later accounts user only.
    core::Result<UserRecord> registerUser(const QString& email, const QString& displayName, const QString& password);

    /// One generic auth.invalid_credentials for unknown email, wrong password
    /// and disabled account, with uniform timing (a dummy hash is verified
    /// when the account does not exist).
    core::Result<LoginResult> login(const QString& email, const QString& password);

    /// Revokes the session behind the token. auth.session_not_found if it is
    /// unknown, expired or already revoked.
    core::VoidResult logout(const QString& token);

    /// Resolves a bearer token to its AuthContext, or nullopt when the token is
    /// unknown, expired, revoked, or belongs to a disabled account. Slides the
    /// expiry forward (throttled to once per few minutes per session).
    core::Result<std::optional<AuthContext>> authenticate(const QString& token);

    /// Full record of the authenticated user (nullopt if deleted meanwhile).
    core::Result<std::optional<UserRecord>> userOf(const AuthContext& context);

private:
    UserRepository users_;
    SessionRepository sessions_;
    AuthOptions options_;
    QString dummyHash_; ///< verified when login targets a non-existent account
};

} // namespace modulo::server::auth
