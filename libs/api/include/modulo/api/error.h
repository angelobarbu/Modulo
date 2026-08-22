#pragma once

#include <modulo/core/result.h>

#include <QJsonObject>
#include <QString>

namespace modulo::api {

/// Uniform error envelope used by every API endpoint:
///   {"error": {"code": "<stable.identifier>", "message": "<detail>"}}
/// Clients branch on `code`; `message` is for humans and logs only.
struct ErrorResponse {
    Q_GADGET
    Q_PROPERTY(QString code MEMBER code)
    Q_PROPERTY(QString message MEMBER message)

public:
    QString code;
    QString message;

    QJsonObject toJson() const;
    static core::Result<ErrorResponse> fromJson(const QJsonObject& json);
};

} // namespace modulo::api
