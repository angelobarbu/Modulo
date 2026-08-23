// modulo_server - Modulo REST API server.
//
// Configuration from environment variables (see .env.example).
// Runs until interrupted; serves on 127.0.0.1 only.

#include <modulo/core/version.h>
#include <modulo/server/auth/auth_service.h>
#include <modulo/server/config/config.h>
#include <modulo/server/db/connection_pool.h>
#include <modulo/server/http/server.h>

#include <QCoreApplication>

#include <cstdlib>

namespace {

constexpr std::size_t kPoolCapacity = 4;

int fail(const modulo::core::Error& error) {
    qCritical().noquote() << QStringLiteral("error [%1]: %2").arg(error.code, error.message);
    return EXIT_FAILURE;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app{argc, argv};
    QCoreApplication::setApplicationName(QStringLiteral("modulo_server"));

    const auto config = modulo::server::config::Config::fromEnvironment();
    if (!config) {
        return fail(config.error());
    }
    if (config->databaseUrl.isEmpty()) {
        return fail({QStringLiteral("config.missing_database_url"),
                     QStringLiteral("MODULO_DB_URL must be set (see .env.example)")});
    }

    // The pool opens connections lazily, so start-up does not require the
    // database to be reachable; the first authenticated request does.
    modulo::server::db::ConnectionPool pool{config->databaseUrl.toStdString(), kPoolCapacity};
    modulo::server::auth::AuthService authService{pool, {.allowRegistration = config->allowRegistration}};

    modulo::server::http::Server server{*config, &authService};
    const auto port = server.listen();
    if (!port) {
        return fail(port.error());
    }

    qInfo().noquote() << QStringLiteral("modulo_server v%1 listening on http://127.0.0.1:%2 (registration %3)")
                             .arg(modulo::core::version())
                             .arg(*port)
                             .arg(config->allowRegistration ? QStringLiteral("open") : QStringLiteral("closed"));
    return app.exec();
}
