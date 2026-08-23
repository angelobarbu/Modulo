#pragma once

#include <modulo/core/result.h>
#include <modulo/server/auth/auth_service.h>
#include <modulo/server/auth/roles.h>
#include <modulo/server/http/responses.h>

#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QString>

#include <utility>

namespace modulo::server::http {

/// The token from an "Authorization: Bearer <token>" header; empty when the
/// header is absent or not a bearer scheme.
QString bearerToken(const QHttpServerRequest& request);

/// Resolves the request's bearer token through AuthService. Errors carry
/// auth.unauthenticated (missing/invalid/expired token) or a db.* code.
core::Result<auth::AuthContext> authenticateRequest(auth::AuthService& service, const QHttpServerRequest& request);

/// Wraps a handler `(const AuthContext&, const QHttpServerRequest&) -> QHttpServerResponse`
/// so it only runs for authenticated requests; everything else gets the 401 envelope.
///
///   server.route("/api/v1/auth/me", Method::Get, authed(service, [](const auto& ctx, const auto&) { ... }));
template <typename Handler>
auto authed(auth::AuthService& service, Handler handler) {
    return [&service, handler = std::move(handler)](const QHttpServerRequest& request) -> QHttpServerResponse {
        auto context = authenticateRequest(service, request);
        if (!context) {
            return errorResponse(context.error());
        }
        return handler(*context, request);
    };
}

/// authed() plus a role check: authenticated callers without the role get 403.
template <typename Handler>
auto requireRole(auth::AuthService& service, auth::Role role, Handler handler) {
    return authed(service, [role, handler = std::move(handler)](const auth::AuthContext& context,
                                                                const QHttpServerRequest& request) {
        if (!context.hasRole(role)) {
            return errorResponse(QHttpServerResponse::StatusCode::Forbidden, QStringLiteral("auth.forbidden"),
                                 QStringLiteral("this action requires the '%1' role").arg(auth::roleName(role)));
        }
        return handler(context, request);
    });
}

} // namespace modulo::server::http
