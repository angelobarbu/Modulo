#pragma once

#include <modulo/server/auth/auth_service.h>

#include <QHttpServer>

namespace modulo::server::http {

/// Registers the authentication endpoints:
///   POST /api/v1/auth/register  {email, displayName, password} -> 201 UserDto
///   POST /api/v1/auth/login     {email, password}              -> 200 LoginResponse
///   POST /api/v1/auth/logout    (bearer)                        -> 204
///   GET  /api/v1/auth/me        (bearer)                        -> 200 UserDto
/// `service` must outlive `server`.
void registerAuthRoutes(QHttpServer& server, auth::AuthService& service);

} // namespace modulo::server::http
