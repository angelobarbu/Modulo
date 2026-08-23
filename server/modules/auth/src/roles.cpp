#include <modulo/server/auth/roles.h>

namespace modulo::server::auth {

QString roleName(Role role) {
    switch (role) {
    case Role::Admin:
        return QStringLiteral("admin");
    case Role::User:
        return QStringLiteral("user");
    }
    return {};
}

std::optional<Role> roleFromName(QStringView name) {
    if (name == QLatin1StringView{"admin"}) {
        return Role::Admin;
    }
    if (name == QLatin1StringView{"user"}) {
        return Role::User;
    }
    return std::nullopt;
}

std::optional<Role> roleFromId(qint16 id) {
    switch (id) {
    case 1:
        return Role::Admin;
    case 2:
        return Role::User;
    default:
        return std::nullopt;
    }
}

} // namespace modulo::server::auth
