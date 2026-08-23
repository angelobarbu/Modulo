#pragma once

#include <QString>
#include <QStringView>

#include <optional>

namespace modulo::server::auth {

/// Fixed role catalogue; ids match the rows seeded by db/migrations/0002_auth.sql.
enum class Role : qint16 { Admin = 1, User = 2 };

/// Wire/log name: "admin" / "user".
QString roleName(Role role);

/// Inverse of roleName(); std::nullopt for unknown names.
std::optional<Role> roleFromName(QStringView name);

/// Database id → Role; std::nullopt for ids not in the catalogue.
std::optional<Role> roleFromId(qint16 id);

} // namespace modulo::server::auth
