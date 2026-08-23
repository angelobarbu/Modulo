#include <modulo/api/json.h>

#include <QJsonArray>
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

core::Result<QStringList> requireStringList(const QJsonObject& object, QLatin1StringView key) {
    const QJsonValue value = object.value(key);
    if (!value.isArray()) {
        return core::makeError(QStringLiteral("api.invalid_field"),
                               QStringLiteral("missing or non-array field '%1'").arg(key));
    }
    QStringList list;
    for (const QJsonValue& element : value.toArray()) {
        if (!element.isString()) {
            return core::makeError(QStringLiteral("api.invalid_field"),
                                   QStringLiteral("field '%1' must contain only strings").arg(key));
        }
        list.append(element.toString());
    }
    return list;
}

} // namespace modulo::api::json
