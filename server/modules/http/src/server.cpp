#include <modulo/api/error.h>
#include <modulo/api/health.h>
#include <modulo/core/version.h>
#include <modulo/server/http/auth_routes.h>
#include <modulo/server/http/logging.h>
#include <modulo/server/http/responses.h>
#include <modulo/server/http/server.h>

#include <QHttpHeaders>
#include <QHttpServerResponder>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QTcpServer>

#include <memory>
#include <utility>

namespace modulo::server::http {

Q_LOGGING_CATEGORY(lcHttp, "modulo.http")

namespace {

QByteArray methodName(QHttpServerRequest::Method method) {
    switch (method) {
    case QHttpServerRequest::Method::Get:
        return "GET";
    case QHttpServerRequest::Method::Post:
        return "POST";
    case QHttpServerRequest::Method::Put:
        return "PUT";
    case QHttpServerRequest::Method::Delete:
        return "DELETE";
    case QHttpServerRequest::Method::Patch:
        return "PATCH";
    default:
        return "OTHER";
    }
}

} // namespace

Server::Server(config::Config config, auth::AuthService* authService)
    : config_{std::move(config)}, authService_{authService} {
    registerRoutes();
}

core::Result<quint16> Server::listen() {
    // Bind to loopback only: in development the API must never be reachable
    // from the network; production exposure goes through a reverse proxy.
    auto tcpServer = std::make_unique<QTcpServer>();
    if (!tcpServer->listen(QHostAddress::LocalHost, config_.httpPort)) {
        return core::makeError(
            QStringLiteral("http.bind_failed"),
            QStringLiteral("cannot listen on 127.0.0.1:%1: %2").arg(config_.httpPort).arg(tcpServer->errorString()));
    }

    const quint16 port = tcpServer->serverPort();
    if (!server_.bind(tcpServer.get())) {
        return core::makeError(QStringLiteral("http.bind_failed"),
                               QStringLiteral("QHttpServer refused the socket on port %1").arg(port));
    }
    tcpServer.release(); // ownership transferred to server_ by bind()

    qCInfo(lcHttp).noquote() << QStringLiteral("listening on 127.0.0.1:%1 (auth routes: %2)")
                                    .arg(port)
                                    .arg(authService_ != nullptr ? QStringLiteral("on") : QStringLiteral("off"));
    return port;
}

void Server::registerRoutes() {
    server_.route(QStringLiteral("/api/v1/health"), QHttpServerRequest::Method::Get, [] {
        const api::HealthResponse health{.status = QStringLiteral("ok"), .version = core::version()};
        return jsonResponse(health.toJson());
    });

    if (authService_ != nullptr) {
        registerAuthRoutes(server_, *authService_);
    }

    // Anything unrouted gets the uniform error envelope instead of Qt's
    // default HTML 404 page.
    server_.setMissingHandler(&server_, [](const QHttpServerRequest&, QHttpServerResponder& responder) {
        const api::ErrorResponse error{.code = QStringLiteral("not_found"),
                                       .message = QStringLiteral("resource not found")};
        responder.write(QJsonDocument{error.toJson()}.toJson(QJsonDocument::Compact), "application/json",
                        QHttpServerResponder::StatusCode::NotFound);
    });

    // Security headers on every response + one debug log line per request.
    server_.addAfterRequestHandler(&server_, [](const QHttpServerRequest& request, QHttpServerResponse& response) {
        QHttpHeaders headers = response.headers();
        headers.append(QHttpHeaders::WellKnownHeader::CacheControl, "no-store");
        headers.append("X-Content-Type-Options", "nosniff");
        headers.append("X-Frame-Options", "DENY");
        headers.append("Referrer-Policy", "no-referrer");
        response.setHeaders(std::move(headers));

        qCDebug(lcHttp).noquote() << methodName(request.method()) << request.url().path()
                                  << static_cast<int>(response.statusCode());
    });
}

} // namespace modulo::server::http
