#include <modulo/api/json.h>

#include <QJsonValue>

namespace modulo::api::json {

core::Result<QString> requireString(const QJsonObject& object, QLatin1StringView key) {
    const QJsonValue value = object.value(key);
    if (!value.isString()) {
        return core::makeError(QStringLiteral("api.invalid_field"),
                               QStringLiteral("missing or non-string field '%1'").arg(key));
    }
    return value.toString();
}

core::Result<QJsonObject> requireObject(const QJsonObject& object, QLatin1StringView key) {
    const QJsonValue value = object.value(key);
    if (!value.isObject()) {
        return core::makeError(QStringLiteral("api.invalid_field"),
                               QStringLiteral("missing or non-object field '%1'").arg(key));
    }
    return value.toObject();
}

} // namespace modulo::api::json
