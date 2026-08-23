#pragma once

#include <QLoggingCategory>

namespace modulo::server::http {

/// "modulo.http" - server lifecycle at info level, one line per request at
/// debug level (method, path, status). Enable with
/// QT_LOGGING_RULES="modulo.http.debug=true".
Q_DECLARE_LOGGING_CATEGORY(lcHttp)

} // namespace modulo::server::http
