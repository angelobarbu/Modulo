#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QQmlEngine>
#include <QUrl>

namespace modulo::client {

/// Minimal client for the Modulo REST API, exposed to QML as `ApiClient`.
///
/// The API base URL comes from the MODULO_API_URL environment variable
/// (default http://127.0.0.1:8080).
class ApiClient : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool serverReachable READ serverReachable NOTIFY healthChanged)
    Q_PROPERTY(QString serverStatus READ serverStatus NOTIFY healthChanged)

public:
    explicit ApiClient(QObject* parent = nullptr);

    [[nodiscard]] bool serverReachable() const { return serverReachable_; }

    [[nodiscard]] QString serverStatus() const { return serverStatus_; }

    /// GET /api/v1/health; the outcome lands in the properties above.
    Q_INVOKABLE void checkHealth();

signals:
    void healthChanged();

private:
    QNetworkAccessManager network_;
    QUrl baseUrl_;
    bool serverReachable_ = false;
    QString serverStatus_;
};

} // namespace modulo::client
