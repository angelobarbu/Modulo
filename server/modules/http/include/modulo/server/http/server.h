#pragma once

#include <modulo/core/result.h>
#include <modulo/server/config/config.h>

#include <QHttpServer>

namespace modulo::server::http {

/// The Modulo REST API server.
///
/// Owns the QHttpServer instance and registers every route. Feature modules
/// contribute their routes here as increments land (auth, transactions, ...).
/// Requires a running Qt event loop (QCoreApplication) to serve requests.
class Server {
public:
    explicit Server(config::Config config);

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// Bind to 127.0.0.1 on config.httpPort (0 = OS-assigned, used by tests)
    /// and start serving. Returns the actually bound port.
    [[nodiscard]] core::Result<quint16> listen();

private:
    void registerRoutes();

    config::Config config_;
    QHttpServer server_;
};

} // namespace modulo::server::http
