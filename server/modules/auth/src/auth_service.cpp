#include <modulo/core/password_policy.h>
#include <modulo/server/auth/auth_service.h>
#include <modulo/server/auth/logging.h>
#include <modulo/server/auth/password_hasher.h>
#include <modulo/server/auth/token.h>

#include <QDateTime>

#include <utility>

namespace modulo::server::auth {

namespace {

constexpr int kMaxEmailLength = 254;
constexpr int kMaxDisplayNameLength = 100;
constexpr qint64 kTouchIntervalSeconds = 5 * 60;

QString normalizeEmail(const QString& email) {
    return email.trimmed();
}

core::VoidResult validateEmail(const QString& email) {
    const qsizetype at = email.indexOf(u'@');
    const bool shapeOk =
        !email.isEmpty() && email.size() <= kMaxEmailLength && at > 0 && at < email.size() - 1 && !email.contains(u' ');
    if (!shapeOk) {
        return core::makeError(QStringLiteral("auth.invalid_email"), QStringLiteral("email address is not valid"));
    }
    return {};
}

core::VoidResult validateDisplayName(const QString& displayName) {
    if (displayName.isEmpty() || displayName.size() > kMaxDisplayNameLength) {
        return core::makeError(QStringLiteral("auth.invalid_display_name"),
                               QStringLiteral("display name must be 1 to %1 characters").arg(kMaxDisplayNameLength));
    }
    return {};
}

std::unexpected<core::Error> invalidCredentials() {
    return core::makeError(QStringLiteral("auth.invalid_credentials"),
                           QStringLiteral("email or password is incorrect"));
}

} // namespace

AuthService::AuthService(db::ConnectionPool& pool, AuthOptions options)
    : users_{pool}, sessions_{pool}, options_{options} {
    // Hashing an arbitrary password once at start-up gives login() something
    // real to verify against when the account does not exist, so the response
    // time does not reveal whether an email is registered.
    if (auto hash = PasswordHasher::hash(QStringLiteral("modulo-dummy-password"))) {
        dummyHash_ = std::move(*hash);
    }
}

core::Result<UserRecord> AuthService::registerUser(const QString& rawEmail, const QString& rawDisplayName,
                                                   const QString& password) {
    const QString email = normalizeEmail(rawEmail);
    if (auto valid = validateEmail(email); !valid) {
        return std::unexpected{valid.error()};
    }
    const QString displayName = rawDisplayName.trimmed();
    if (auto valid = validateDisplayName(displayName); !valid) {
        return std::unexpected{valid.error()};
    }
    if (auto valid = core::validatePassword(password); !valid) {
        return std::unexpected{valid.error()};
    }

    const auto existing = users_.count();
    if (!existing) {
        return std::unexpected{existing.error()};
    }
    const bool firstAccount = *existing == 0;
    if (!firstAccount && !options_.allowRegistration) {
        return core::makeError(QStringLiteral("auth.registration_disabled"),
                               QStringLiteral("registration is disabled on this server"));
    }

    auto hash = PasswordHasher::hash(password);
    if (!hash) {
        return std::unexpected{hash.error()};
    }

    const QList<Role> roles = firstAccount ? QList<Role>{Role::Admin, Role::User} : QList<Role>{Role::User};
    auto user = users_.create(email, displayName, *hash, roles);
    if (user) {
        qCInfo(lcAuth).noquote() << QStringLiteral("user registered id=%1 roles=%2%3")
                                        .arg(user->id)
                                        .arg(roles.size())
                                        .arg(firstAccount ? QStringLiteral(" (first account, admin)") : QString{});
    }
    return user;
}

core::Result<LoginResult> AuthService::login(const QString& rawEmail, const QString& password) {
    const QString email = normalizeEmail(rawEmail);

    const auto found = users_.findByEmail(email);
    if (!found) {
        return std::unexpected{found.error()};
    }

    // Always verify against a real hash so a missing account costs the same
    // time as a wrong password.
    const QString& hash = found->has_value() ? (*found)->passwordHash : dummyHash_;
    const bool passwordOk = PasswordHasher::verify(hash, password);
    if (!found->has_value() || !passwordOk || (*found)->disabled) {
        qCInfo(lcAuth) << "login failed";
        return invalidCredentials();
    }

    const UserRecord& user = **found;
    const QString token = token::generate();
    const QDateTime expires = QDateTime::currentDateTimeUtc().addDays(options_.sessionDays);
    const auto session = sessions_.create(user.id, token::digest(token), expires);
    if (!session) {
        return std::unexpected{session.error()};
    }

    qCInfo(lcAuth).noquote() << QStringLiteral("login ok user=%1 session=%2").arg(user.id, session->id);
    return LoginResult{.token = token, .user = user};
}

core::VoidResult AuthService::logout(const QString& token) {
    const auto session = sessions_.findActiveByDigest(token::digest(token));
    if (!session) {
        return std::unexpected{session.error()};
    }
    if (!session->has_value()) {
        return core::makeError(QStringLiteral("auth.session_not_found"), QStringLiteral("session is not active"));
    }
    const auto revoked = sessions_.revoke((*session)->id);
    if (revoked) {
        qCInfo(lcAuth).noquote() << QStringLiteral("logout session=%1").arg((*session)->id);
    }
    return revoked;
}

core::Result<std::optional<AuthContext>> AuthService::authenticate(const QString& token) {
    if (token.isEmpty()) {
        return std::optional<AuthContext>{};
    }

    const auto session = sessions_.findActiveByDigest(token::digest(token));
    if (!session) {
        return std::unexpected{session.error()};
    }
    if (!session->has_value()) {
        return std::optional<AuthContext>{};
    }

    const auto user = users_.findById((*session)->userId);
    if (!user) {
        return std::unexpected{user.error()};
    }
    if (!user->has_value() || (*user)->disabled) {
        return std::optional<AuthContext>{};
    }

    // Sliding expiry, throttled: one UPDATE per session per interval, not per request.
    const QDateTime now = QDateTime::currentDateTimeUtc();
    if ((*session)->lastSeenAt.secsTo(now) >= kTouchIntervalSeconds) {
        if (const auto touched = sessions_.touch((*session)->id, now.addDays(options_.sessionDays)); !touched) {
            qCWarning(lcAuth).noquote()
                << QStringLiteral("could not slide session %1: %2").arg((*session)->id, touched.error().message);
        }
    }

    return AuthContext{.userId = (*user)->id, .sessionId = (*session)->id, .roles = (*user)->roles};
}

core::Result<std::optional<UserRecord>> AuthService::userOf(const AuthContext& context) {
    return users_.findById(context.userId);
}

} // namespace modulo::server::auth
