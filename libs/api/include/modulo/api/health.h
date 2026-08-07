#pragma once

#include <modulo/core/result.h>

#include <QJsonObject>
#include <QString>

namespace modulo::api {

/// Response body of GET /api/v1/health.
///
/// DTO conventions (all Modulo DTOs follow this shape): a Q_GADGET struct —
/// QML-readable by value, no QObject overhead — with an explicit, validating
/// QJson mapping. fromJson() rejects missing/mistyped fields via
/// api::json::require* instead of QJson's silent defaults.
struct HealthResponse {
    Q_GADGET
    Q_PROPERTY(QString status MEMBER status)
    Q_PROPERTY(QString version MEMBER version)

public:
    QString status;  ///< "ok" when the server is serving requests.
    QString version; ///< Server semantic version.

    [[nodiscard]] QJsonObject toJson() const;
    [[nodiscard]] static core::Result<HealthResponse> fromJson(const QJsonObject& json);
};

} // namespace modulo::api
