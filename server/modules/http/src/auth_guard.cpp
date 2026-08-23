#include <modulo/server/http/auth_guard.h>

#include <QHttpHeaders>

namespace modulo::server::http {

QString bearerToken(const QHttpServerRequest& request) {
    const QByteArray header = request.headers().value(QHttpHeaders::WellKnownHeader::Authorization).toByteArray();
    constexpr QByteArrayView kScheme{"Bearer "};
    if (!header.startsWith(kScheme)) {
        return {};
    }
    return QString::fromLatin1(header.mid(kScheme.size()).trimmed());
}

core::Result<auth::AuthContext> authenticateRequest(auth::AuthService& service, const QHttpServerRequest& request) {
    const QString token = bearerToken(request);
    if (token.isEmpty()) {
        return core::makeError(QStringLiteral("auth.unauthenticated"),
                               QStringLiteral("missing bearer token (Authorization: Bearer <token>)"));
    }

    auto context = service.authenticate(token);
    if (!context) {
        return std::unexpected{context.error()};
    }
    if (!context->has_value()) {
        return core::makeError(QStringLiteral("auth.unauthenticated"),
                               QStringLiteral("token is invalid, expired or revoked"));
    }
    return std::move(**context);
}

} // namespace modulo::server::http
