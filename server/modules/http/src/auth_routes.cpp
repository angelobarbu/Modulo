#include <modulo/api/auth.h>
#include <modulo/server/auth/roles.h>
#include <modulo/server/http/auth_guard.h>
#include <modulo/server/http/auth_routes.h>
#include <modulo/server/http/responses.h>

namespace modulo::server::http {

namespace {

using Method = QHttpServerRequest::Method;
using Status = QHttpServerResponse::StatusCode;

api::UserDto toDto(const auth::UserRecord& user) {
    QStringList roles;
    for (const auth::Role role : user.roles) {
        roles.append(auth::roleName(role));
    }
    return api::UserDto{.id = user.id, .email = user.email, .displayName = user.displayName, .roles = roles};
}

} // namespace

void registerAuthRoutes(QHttpServer& server, auth::AuthService& service) {
    server.route(QStringLiteral("/api/v1/auth/register"), Method::Post,
                 [&service](const QHttpServerRequest& request) -> QHttpServerResponse {
                     const auto body = jsonBody(request);
                     if (!body) {
                         return errorResponse(body.error());
                     }
                     const auto dto = api::RegisterRequest::fromJson(*body);
                     if (!dto) {
                         return errorResponse(dto.error());
                     }
                     const auto user = service.registerUser(dto->email, dto->displayName, dto->password);
                     if (!user) {
                         return errorResponse(user.error());
                     }
                     return jsonResponse(toDto(*user).toJson(), Status::Created);
                 });

    server.route(QStringLiteral("/api/v1/auth/login"), Method::Post,
                 [&service](const QHttpServerRequest& request) -> QHttpServerResponse {
                     const auto body = jsonBody(request);
                     if (!body) {
                         return errorResponse(body.error());
                     }
                     const auto dto = api::LoginRequest::fromJson(*body);
                     if (!dto) {
                         return errorResponse(dto.error());
                     }
                     const auto login = service.login(dto->email, dto->password);
                     if (!login) {
                         return errorResponse(login.error());
                     }
                     const api::LoginResponse response{.token = login->token, .user = toDto(login->user)};
                     return jsonResponse(response.toJson());
                 });

    server.route(QStringLiteral("/api/v1/auth/logout"), Method::Post,
                 authed(service, [&service](const auth::AuthContext&, const QHttpServerRequest& request) {
                     if (const auto revoked = service.logout(bearerToken(request)); !revoked) {
                         return errorResponse(revoked.error());
                     }
                     return QHttpServerResponse{Status::NoContent};
                 }));

    server.route(QStringLiteral("/api/v1/auth/me"), Method::Get,
                 authed(service, [&service](const auth::AuthContext& context, const QHttpServerRequest&) {
                     const auto user = service.userOf(context);
                     if (!user) {
                         return errorResponse(user.error());
                     }
                     if (!user->has_value()) {
                         return errorResponse(Status::NotFound, QStringLiteral("auth.user_not_found"),
                                              QStringLiteral("the authenticated user no longer exists"));
                     }
                     return jsonResponse(toDto(**user).toJson());
                 }));
}

} // namespace modulo::server::http
