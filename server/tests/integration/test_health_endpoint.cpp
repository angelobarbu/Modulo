#include <modulo/api/error.h>
#include <modulo/api/health.h>
#include <modulo/core/version.h>
#include <modulo/server/config/config.h>
#include <modulo/server/http/server.h>
#include <modulo/testing/integration.h>

#include <QJsonDocument>
#include <QTest>

#include <memory>

using namespace modulo;

class HealthEndpointTest : public QObject {
    Q_OBJECT

private slots:

    /// Fresh server on an OS-assigned loopback port for every test function.
    void init() {
        server_ = std::make_unique<server::http::Server>(server::config::Config{.httpPort = 0});
        const auto port = server_->listen();
        QVERIFY(port.has_value());
        baseUrl_ = QUrl{QStringLiteral("http://127.0.0.1:%1").arg(*port)};
    }

    void cleanup() { server_.reset(); }

    void healthReportsOkAndTheServerVersion() {
        MODULO_REQUIRE_TEST_DATABASE();

        const auto response = testing::httpGet(url(QStringLiteral("/api/v1/health")));
        QCOMPARE(response.status, 200);

        const auto document = QJsonDocument::fromJson(response.body);
        QVERIFY(document.isObject());
        const auto health = api::HealthResponse::fromJson(document.object());
        QVERIFY(health.has_value());
        QCOMPARE(health->status, QStringLiteral("ok"));
        QCOMPARE(health->version, core::version());
    }

    void unknownRoutesAnswerWithTheJsonErrorEnvelope() {
        MODULO_REQUIRE_TEST_DATABASE();

        const auto response = testing::httpGet(url(QStringLiteral("/api/v1/does-not-exist")));
        QCOMPARE(response.status, 404);

        const auto document = QJsonDocument::fromJson(response.body);
        QVERIFY(document.isObject());
        const auto error = api::ErrorResponse::fromJson(document.object());
        QVERIFY(error.has_value());
        QCOMPARE(error->code, QStringLiteral("not_found"));
    }

private:
    QUrl url(const QString& path) const { return baseUrl_.resolved(QUrl{path}); }

    std::unique_ptr<server::http::Server> server_;
    QUrl baseUrl_;
};

QTEST_GUILESS_MAIN(HealthEndpointTest)
#include "test_health_endpoint.moc"
