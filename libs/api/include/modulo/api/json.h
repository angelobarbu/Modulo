#pragma once

#include <modulo/core/result.h>

#include <QJsonObject>
#include <QString>

namespace modulo::api::json {

/// Required-field accessors for wire data.
///
/// QJson's own reads default silently (a missing key yields an empty value);
/// wire parsing must never do that. These helpers make every absent or
/// wrongly-typed field an explicit Error with code "api.invalid_field".

core::Result<QString> requireString(const QJsonObject& object, QLatin1StringView key);

core::Result<QJsonObject> requireObject(const QJsonObject& object, QLatin1StringView key);

} // namespace modulo::api::json
