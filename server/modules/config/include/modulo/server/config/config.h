#pragma once

#include <modulo/core/result.h>

#include <QString>
#include <QtGlobal>

namespace modulo::server::config {

/// Server process configuration sourced from environment variables
/// (documented in .env.example at the repo root).
struct Config {
    /// MODULO_DB_URL. May be empty for features that do not touch the
    /// database; features that need it validate at their own startup.
    QString databaseUrl;

    /// MODULO_HTTP_PORT. Port 0 asks the OS for a free port (used by tests).
    quint16 httpPort = 8080;

    /// MODULO_DATA_DIR — root for server-managed files (document uploads).
    QString dataDir = QStringLiteral("./var/data");

    /// Build a Config from the process environment. Unset or empty variables
    /// keep their defaults; malformed values yield an Error whose code is
    /// prefixed "config.".
    static core::Result<Config> fromEnvironment();
};

} // namespace modulo::server::config
