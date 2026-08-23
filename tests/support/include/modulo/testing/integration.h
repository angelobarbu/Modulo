#pragma once

// Fixtures shared by integration tests (server/tests/integration).
//
// Integration tests are OPT-IN: they run only when MODULO_TEST_DB_URL is set
// (see .env.example). Otherwise every test function QSKIPs, and CTest reports
// the binary as skipped (the toolkit maps Qt Test's "SKIP   :" output line) —
// `ctest --preset unit` / `all` therefore never require Docker.
//
// Test binaries use QTEST_GUILESS_MAIN, which provides the QCoreApplication
// event loop that QHttpServer and QNetworkAccessManager need.

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QByteArray>
#include <QString>
#include <QTest>
#include <QTimer>
#include <QUrl>

/// First statement of every integration test function: skips the test when
/// MODULO_TEST_DB_URL is unset. A macro because QSKIP must return from the
/// test function itself.
#define MODULO_REQUIRE_TEST_DATABASE()                                                                                 \
    if (modulo::testing::testDatabaseUrl().isEmpty()) {                                                                \
        QSKIP("MODULO_TEST_DB_URL is not set; integration tests are opt-in");                                          \
    }

namespace modulo::testing {

inline QString testDatabaseUrl() {
    return qEnvironmentVariable("MODULO_TEST_DB_URL");
}

struct HttpResponse {
    int status = 0;
    QByteArray body;
};

/// Blocking HTTP request against an in-process server, with a timeout so a
/// dead server fails the test instead of hanging it. `body` is sent as JSON;
/// `bearer`, when non-empty, becomes the Authorization header.
inline HttpResponse httpRequest(const QByteArray& method, const QUrl& url, const QByteArray& body = {},
                                const QString& bearer = {}, int timeoutMs = 5000) {
    QNetworkAccessManager network;
    QNetworkRequest request{url};
    if (!body.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    }
    if (!bearer.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + bearer.toLatin1());
    }
    QNetworkReply* reply = network.sendCustomRequest(request, method, body);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();

    HttpResponse response;
    response.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.body = reply->readAll();
    reply->deleteLater();
    return response;
}

inline HttpResponse httpGet(const QUrl& url, const QString& bearer = {}, int timeoutMs = 5000) {
    return httpRequest("GET", url, {}, bearer, timeoutMs);
}

} // namespace modulo::testing
