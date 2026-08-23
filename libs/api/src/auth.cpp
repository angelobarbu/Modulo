#include <modulo/api/auth.h>
#include <modulo/api/json.h>

#include <QJsonArray>

#include <utility>

namespace modulo::api {

namespace {

template <typename T>
std::unexpected<core::Error> fail(core::Result<T>&& result) {
    return std::unexpected{std::move(result).error()};
}

} // namespace

// --- RegisterRequest ---------------------------------------------------------

QJsonObject RegisterRequest::toJson() const {
    return QJsonObject{{QStringLiteral("email"), email},
                       {QStringLiteral("displayName"), displayName},
                       {QStringLiteral("password"), password}};
}

core::Result<RegisterRequest> RegisterRequest::fromJson(const QJsonObject& json) {
    auto email = json::requireString(json, QLatin1StringView{"email"});
    if (!email) {
        return fail(std::move(email));
    }
    auto displayName = json::requireString(json, QLatin1StringView{"displayName"});
    if (!displayName) {
        return fail(std::move(displayName));
    }
    auto password = json::requireString(json, QLatin1StringView{"password"});
    if (!password) {
        return fail(std::move(password));
    }
    return RegisterRequest{
        .email = std::move(*email), .displayName = std::move(*displayName), .password = std::move(*password)};
}

// --- LoginRequest ------------------------------------------------------------

QJsonObject LoginRequest::toJson() const {
    return QJsonObject{{QStringLiteral("email"), email}, {QStringLiteral("password"), password}};
}

core::Result<LoginRequest> LoginRequest::fromJson(const QJsonObject& json) {
    auto email = json::requireString(json, QLatin1StringView{"email"});
    if (!email) {
        return fail(std::move(email));
    }
    auto password = json::requireString(json, QLatin1StringView{"password"});
    if (!password) {
        return fail(std::move(password));
    }
    return LoginRequest{.email = std::move(*email), .password = std::move(*password)};
}

// --- UserDto -----------------------------------------------------------------

QJsonObject UserDto::toJson() const {
    return QJsonObject{{QStringLiteral("id"), id},
                       {QStringLiteral("email"), email},
                       {QStringLiteral("displayName"), displayName},
                       {QStringLiteral("roles"), QJsonArray::fromStringList(roles)}};
}

core::Result<UserDto> UserDto::fromJson(const QJsonObject& json) {
    auto id = json::requireString(json, QLatin1StringView{"id"});
    if (!id) {
        return fail(std::move(id));
    }
    auto email = json::requireString(json, QLatin1StringView{"email"});
    if (!email) {
        return fail(std::move(email));
    }
    auto displayName = json::requireString(json, QLatin1StringView{"displayName"});
    if (!displayName) {
        return fail(std::move(displayName));
    }
    auto roles = json::requireStringList(json, QLatin1StringView{"roles"});
    if (!roles) {
        return fail(std::move(roles));
    }
    return UserDto{.id = std::move(*id),
                   .email = std::move(*email),
                   .displayName = std::move(*displayName),
                   .roles = std::move(*roles)};
}

// --- LoginResponse -----------------------------------------------------------

QJsonObject LoginResponse::toJson() const {
    return QJsonObject{{QStringLiteral("token"), token}, {QStringLiteral("user"), user.toJson()}};
}

core::Result<LoginResponse> LoginResponse::fromJson(const QJsonObject& json) {
    auto token = json::requireString(json, QLatin1StringView{"token"});
    if (!token) {
        return fail(std::move(token));
    }
    auto userJson = json::requireObject(json, QLatin1StringView{"user"});
    if (!userJson) {
        return fail(std::move(userJson));
    }
    auto user = UserDto::fromJson(*userJson);
    if (!user) {
        return fail(std::move(user));
    }
    return LoginResponse{.token = std::move(*token), .user = std::move(*user)};
}

} // namespace modulo::api
