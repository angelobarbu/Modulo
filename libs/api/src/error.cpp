#include <modulo/api/error.h>
#include <modulo/api/json.h>

#include <utility>

namespace modulo::api {

QJsonObject ErrorResponse::toJson() const {
    return QJsonObject{
        {QStringLiteral("error"), QJsonObject{{QStringLiteral("code"), code}, {QStringLiteral("message"), message}}}};
}

core::Result<ErrorResponse> ErrorResponse::fromJson(const QJsonObject& json) {
    auto envelope = json::requireObject(json, QLatin1StringView{"error"});
    if (!envelope) {
        return std::unexpected{std::move(envelope).error()};
    }

    auto code = json::requireString(*envelope, QLatin1StringView{"code"});
    if (!code) {
        return std::unexpected{std::move(code).error()};
    }

    auto message = json::requireString(*envelope, QLatin1StringView{"message"});
    if (!message) {
        return std::unexpected{std::move(message).error()};
    }

    return ErrorResponse{.code = std::move(*code), .message = std::move(*message)};
}

} // namespace modulo::api
