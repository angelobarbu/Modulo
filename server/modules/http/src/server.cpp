#include <modulo/api/error.h>
#include <modulo/api/health.h>
#include <modulo/core/version.h>
#include <modulo/server/http/server.h>

#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QTcpServer>

#include <memory>
#include <utility>

namespace modulo::server::http {

namespace {

QByteArray toBody(const QJsonObject& json) {
    return QJsonDocument{json}.toJson(QJsonDocument::Compact);
}

QHttpServerResponse jsonResponse(const QJsonObject& body, QHttpServerResponse::StatusCode status) {
    return QHttpServerResponse{"application/json", toBody(body), status};
}

} // namespace

Server::Server(config::Config config) : config_{std::move(config)} {
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

    return port;
}

void Server::registerRoutes() {
    server_.route("/api/v1/health", QHttpServerRequest::Method::Get, [] {
        const api::HealthResponse health{.status = QStringLiteral("ok"), .version = core::version()};
        return jsonResponse(health.toJson(), QHttpServerResponse::StatusCode::Ok);
    });

    // Anything unrouted gets the uniform error envelope instead of Qt's
    // default HTML 404 page.
    server_.setMissingHandler(&server_, [](const QHttpServerRequest&, QHttpServerResponder& responder) {
        const api::ErrorResponse error{.code = QStringLiteral("not_found"),
                                       .message = QStringLiteral("resource not found")};
        responder.write(toBody(error.toJson()), "application/json", QHttpServerResponder::StatusCode::NotFound);
    });
}

} // namespace modulo::server::http
