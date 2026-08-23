#pragma once

#include <modulo/core/result.h>

#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonObject>
#include <QString>

namespace modulo::server::http {

/// JSON body with the given status.
QHttpServerResponse jsonResponse(const QJsonObject& body,
                                 QHttpServerResponse::StatusCode status = QHttpServerResponse::StatusCode::Ok);

/// The uniform error envelope {"error":{"code","message"}}, status derived
/// from the code via statusFor().
QHttpServerResponse errorResponse(const core::Error& error);

QHttpServerResponse errorResponse(QHttpServerResponse::StatusCode status, const QString& code, const QString& message);

/// Maps stable error codes to HTTP statuses: auth.invalid_credentials /
/// auth.unauthenticated -> 401, auth.forbidden / auth.registration_disabled
/// -> 403, auth.email_taken -> 409, *not_found -> 404, api.* / password.* /
/// auth.invalid_* -> 400, db.* -> 503, anything else -> 500.
QHttpServerResponse::StatusCode statusFor(const QString& code);

/// The request body parsed as a JSON object; api.invalid_json otherwise.
core::Result<QJsonObject> jsonBody(const QHttpServerRequest& request);

} // namespace modulo::server::http
