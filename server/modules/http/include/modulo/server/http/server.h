#pragma once

#include <modulo/core/result.h>
#include <modulo/server/auth/auth_service.h>
#include <modulo/server/config/config.h>

#include <QHttpServer>

namespace modulo::server::http {

/// The Modulo REST API server.
///
/// Owns the QHttpServer instance and registers every route. Feature modules
/// contribute their routes here as increments land (auth, transactions, ...).
/// Requires a running Qt event loop (QCoreApplication) to serve requests.
///
/// Every response carries security headers (Cache-Control: no-store,
/// X-Content-Type-Options: nosniff, X-Frame-Options: DENY,
/// Referrer-Policy: no-referrer) and unrouted paths answer with the JSON
/// error envelope.
class Server {
public:
    /// `authService` may be null for a health-only server (used by tests);
    /// when given it must outlive the Server.
    explicit Server(config::Config config, auth::AuthService* authService = nullptr);

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// Bind to 127.0.0.1 on config.httpPort (0 = OS-assigned, used by tests)
    /// and start serving. Returns the actually bound port.
    core::Result<quint16> listen();

private:
    void registerRoutes();

    config::Config config_;
    auth::AuthService* authService_;
    QHttpServer server_;
};

} // namespace modulo::server::http
