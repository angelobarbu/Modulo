#include <modulo/api/error.h>
#include <modulo/server/http/responses.h>

#include <QJsonDocument>
#include <QJsonParseError>

namespace modulo::server::http {

namespace {

QByteArray toBody(const QJsonObject& json) {
    return QJsonDocument{json}.toJson(QJsonDocument::Compact);
}

} // namespace

QHttpServerResponse jsonResponse(const QJsonObject& body, QHttpServerResponse::StatusCode status) {
    return QHttpServerResponse{"application/json", toBody(body), status};
}

QHttpServerResponse errorResponse(const core::Error& error) {
    return errorResponse(statusFor(error.code), error.code, error.message);
}

QHttpServerResponse errorResponse(QHttpServerResponse::StatusCode status, const QString& code, const QString& message) {
    const api::ErrorResponse envelope{.code = code, .message = message};
    return jsonResponse(envelope.toJson(), status);
}

QHttpServerResponse::StatusCode statusFor(const QString& code) {
    using Status = QHttpServerResponse::StatusCode;

    if (code == QLatin1StringView{"auth.invalid_credentials"} || code == QLatin1StringView{"auth.unauthenticated"}) {
        return Status::Unauthorized;
    }
    if (code == QLatin1StringView{"auth.forbidden"} || code == QLatin1StringView{"auth.registration_disabled"}) {
        return Status::Forbidden;
    }
    if (code == QLatin1StringView{"auth.email_taken"}) {
        return Status::Conflict;
    }
    if (code.endsWith(QLatin1StringView{"not_found"})) {
        return Status::NotFound;
    }
    if (code.startsWith(QLatin1StringView{"api."}) || code.startsWith(QLatin1StringView{"password."}) ||
        code.startsWith(QLatin1StringView{"auth.invalid_"})) {
        return Status::BadRequest;
    }
    if (code.startsWith(QLatin1StringView{"db."})) {
        return Status::ServiceUnavailable;
    }
    return Status::InternalServerError;
}

core::Result<QJsonObject> jsonBody(const QHttpServerRequest& request) {
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(request.body(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return core::makeError(QStringLiteral("api.invalid_json"),
                               QStringLiteral("request body must be a JSON object"));
    }
    return document.object();
}

} // namespace modulo::server::http
