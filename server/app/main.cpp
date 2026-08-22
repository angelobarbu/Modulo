// modulo_server — Modulo REST API server.
//
// Configuration from environment variables (see .env.example).
// Runs until interrupted; serves on 127.0.0.1 only.

#include <modulo/core/version.h>
#include <modulo/server/config/config.h>
#include <modulo/server/http/server.h>

#include <QCoreApplication>

#include <cstdlib>

int main(int argc, char* argv[]) {
    QCoreApplication app{argc, argv};

    const auto config = modulo::server::config::Config::fromEnvironment();
    if (!config) {
        qCritical().noquote() << QStringLiteral("error [%1]: %2").arg(config.error().code, config.error().message);
        return EXIT_FAILURE;
    }

    modulo::server::http::Server server{*config};
    const auto port = server.listen();
    if (!port) {
        qCritical().noquote() << QStringLiteral("error [%1]: %2").arg(port.error().code, port.error().message);
        return EXIT_FAILURE;
    }

    qInfo().noquote()
        << QStringLiteral("modulo_server v%1 listening on http://127.0.0.1:%2").arg(modulo::core::version()).arg(*port);
    return app.exec();
}
