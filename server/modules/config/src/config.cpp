#include <modulo/server/config/config.h>

namespace modulo::server::config {

namespace {

/// qEnvironmentVariable's own default only covers UNSET variables; an
/// exported-but-empty variable should fall back to the default too.
QString envOr(const char* name, const QString& fallback) {
    const QString value = qEnvironmentVariable(name);
    return value.isEmpty() ? fallback : value;
}

} // namespace

core::Result<Config> Config::fromEnvironment() {
    Config config;
    config.databaseUrl = envOr("MODULO_DB_URL", QString{});
    config.dataDir = envOr("MODULO_DATA_DIR", config.dataDir);

    const QString portText = envOr("MODULO_HTTP_PORT", QStringLiteral("8080"));
    bool valid = false;
    const quint16 port = portText.toUShort(&valid); // rejects non-numeric and > 65535
    if (!valid) {
        return core::makeError(
            QStringLiteral("config.invalid_port"),
            QStringLiteral("MODULO_HTTP_PORT must be an integer in [0, 65535], got '%1'").arg(portText));
    }
    config.httpPort = port;

    const QString registration = envOr("MODULO_ALLOW_REGISTRATION", QStringLiteral("true")).toLower();
    if (registration == QLatin1StringView{"true"} || registration == QLatin1StringView{"1"} ||
        registration == QLatin1StringView{"yes"}) {
        config.allowRegistration = true;
    } else if (registration == QLatin1StringView{"false"} || registration == QLatin1StringView{"0"} ||
               registration == QLatin1StringView{"no"}) {
        config.allowRegistration = false;
    } else {
        return core::makeError(QStringLiteral("config.invalid_bool"),
                               QStringLiteral("MODULO_ALLOW_REGISTRATION must be true/false (or 1/0, yes/no), got '%1'")
                                   .arg(registration));
    }

    return config;
}

} // namespace modulo::server::config
