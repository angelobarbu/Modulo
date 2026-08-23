#pragma once

#include <modulo/core/result.h>

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace modulo::api {

/// POST /api/v1/auth/register body.
struct RegisterRequest {
    Q_GADGET
    Q_PROPERTY(QString email MEMBER email)
    Q_PROPERTY(QString displayName MEMBER displayName)
    Q_PROPERTY(QString password MEMBER password)

public:
    QString email;
    QString displayName;
    QString password;

    QJsonObject toJson() const;
    static core::Result<RegisterRequest> fromJson(const QJsonObject& json);
};

/// POST /api/v1/auth/login body.
struct LoginRequest {
    Q_GADGET
    Q_PROPERTY(QString email MEMBER email)
    Q_PROPERTY(QString password MEMBER password)

public:
    QString email;
    QString password;

    QJsonObject toJson() const;
    static core::Result<LoginRequest> fromJson(const QJsonObject& json);
};

/// A user as exposed by the API (never the password hash).
struct UserDto {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString email MEMBER email)
    Q_PROPERTY(QString displayName MEMBER displayName)
    Q_PROPERTY(QStringList roles MEMBER roles)

public:
    QString id;
    QString email;
    QString displayName;
    QStringList roles; ///< role names: "admin", "user"

    QJsonObject toJson() const;
    static core::Result<UserDto> fromJson(const QJsonObject& json);
};

/// POST /api/v1/auth/login response: the opaque bearer token plus the user.
struct LoginResponse {
    Q_GADGET
    Q_PROPERTY(QString token MEMBER token)
    Q_PROPERTY(modulo::api::UserDto user MEMBER user)

public:
    QString token;
    UserDto user;

    QJsonObject toJson() const;
    static core::Result<LoginResponse> fromJson(const QJsonObject& json);
};

} // namespace modulo::api
