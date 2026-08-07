#include <modulo/api/health.h>
#include <modulo/api/json.h>

#include <utility>

namespace modulo::api {

QJsonObject HealthResponse::toJson() const {
    return QJsonObject{{QStringLiteral("status"), status}, {QStringLiteral("version"), version}};
}

core::Result<HealthResponse> HealthResponse::fromJson(const QJsonObject& json) {
    auto status = json::requireString(json, QLatin1StringView{"status"});
    if (!status) {
        return std::unexpected{std::move(status).error()};
    }

    auto version = json::requireString(json, QLatin1StringView{"version"});
    if (!version) {
        return std::unexpected{std::move(version).error()};
    }

    return HealthResponse{.status = std::move(*status), .version = std::move(*version)};
}

} // namespace modulo::api
