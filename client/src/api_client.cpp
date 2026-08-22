#include <modulo/api/health.h>
#include <modulo/client/api_client.h>

#include <QJsonDocument>
#include <QNetworkReply>

namespace modulo::client {

namespace {

QUrl defaultBaseUrl() {
    return QUrl{qEnvironmentVariable("MODULO_API_URL", QStringLiteral("http://127.0.0.1:8080"))};
}

} // namespace

ApiClient::ApiClient(QObject* parent) : QObject{parent}, baseUrl_{defaultBaseUrl()}, serverStatus_{tr("connecting…")} {
}

void ApiClient::checkHealth() {
    const QNetworkRequest request{baseUrl_.resolved(QUrl{QStringLiteral("/api/v1/health")})};
    auto* reply = network_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();

        bool reachable = false;
        QString status;
        if (reply->error() == QNetworkReply::NoError) {
            const auto document = QJsonDocument::fromJson(reply->readAll());
            const auto health = document.isObject()
                                    ? api::HealthResponse::fromJson(document.object())
                                    : core::makeError(QStringLiteral("api.invalid_field"),
                                                      QStringLiteral("response body is not a JSON object"));
            if (health && health->status == QStringLiteral("ok")) {
                reachable = true;
                status = tr("server %1 (v%2)").arg(health->status, health->version);
            } else {
                status = tr("invalid response from server");
            }
        } else {
            status = tr("server unreachable");
        }

        if (reachable != serverReachable_ || status != serverStatus_) {
            serverReachable_ = reachable;
            serverStatus_ = status;
            emit healthChanged();
        }
    });
}

} // namespace modulo::client
