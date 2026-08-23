#pragma once

#include <QLoggingCategory>

namespace modulo::server::auth {

/// "modulo.auth" - registration, login, logout and session events. Never logs
/// tokens, password material or email addresses; user and session ids only.
/// Filter at runtime with QT_LOGGING_RULES, e.g. "modulo.auth.debug=true".
Q_DECLARE_LOGGING_CATEGORY(lcAuth)

} // namespace modulo::server::auth
