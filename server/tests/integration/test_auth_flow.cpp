#include <modulo/api/auth.h>
#include <modulo/api/error.h>
#include <modulo/server/auth/auth_service.h>
#include <modulo/server/config/config.h>
#include <modulo/server/db/connection_pool.h>
#include <modulo/server/http/server.h>
#include <modulo/testing/integration.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <memory>

using namespace modulo;
using namespace modulo::server;

/// End-to-end authentication over real HTTP: server with auth routes on an
/// OS-assigned port, real client, real modulo_test database.
class AuthFlowTest : public QObject {
    Q_OBJECT

private slots:

    void init() {
        if (testing::testDatabaseUrl().isEmpty()) {
            return; // every test function QSKIPs itself
        }
        pool_ = std::make_unique<db::ConnectionPool>(testing::testDatabaseUrl().toStdString(), 2);
        {
            auto lease = pool_->acquire();
            pqxx::work tx{lease.connection()};
            tx.exec("TRUNCATE users CASCADE");
            tx.commit();
        }
        service_ = std::make_unique<auth::AuthService>(*pool_);
        server_ = std::make_unique<http::Server>(config::Config{.httpPort = 0}, service_.get());
        const auto port = server_->listen();
        QVERIFY(port.has_value());
        baseUrl_ = QUrl{QStringLiteral("http://127.0.0.1:%1").arg(*port)};
    }

    void cleanup() {
        server_.reset();
        service_.reset();
        pool_.reset();
    }

    void registerLoginMeLogout() {
        MODULO_REQUIRE_TEST_DATABASE();

        // register -> 201, first account is admin + user
        const auto registered = post(QStringLiteral("/api/v1/auth/register"),
                                     api::RegisterRequest{.email = QStringLiteral("angelo@example.com"),
                                                          .displayName = QStringLiteral("Angelo"),
                                                          .password = QStringLiteral("correct horse battery")}
                                         .toJson());
        QCOMPARE(registered.status, 201);
        const auto user = api::UserDto::fromJson(object(registered.body));
        QVERIFY(user.has_value());
        QCOMPARE(user->email, QStringLiteral("angelo@example.com"));
        QCOMPARE(user->roles, (QStringList{QStringLiteral("admin"), QStringLiteral("user")}));

        // login -> 200 with token
        const auto loggedIn = post(QStringLiteral("/api/v1/auth/login"),
                                   api::LoginRequest{.email = QStringLiteral("ANGELO@example.com"),
                                                     .password = QStringLiteral("correct horse battery")}
                                       .toJson());
        QCOMPARE(loggedIn.status, 200);
        const auto login = api::LoginResponse::fromJson(object(loggedIn.body));
        QVERIFY(login.has_value());
        QCOMPARE(login->token.size(), 43);
        QCOMPARE(login->user.id, user->id);

        // me -> 200 for the bearer
        const auto me = testing::httpGet(url(QStringLiteral("/api/v1/auth/me")), login->token);
        QCOMPARE(me.status, 200);
        QCOMPARE(api::UserDto::fromJson(object(me.body))->id, user->id);

        // logout -> 204, then the same token is rejected
        QCOMPARE(testing::httpRequest("POST", url(QStringLiteral("/api/v1/auth/logout")), {}, login->token).status,
                 204);
        const auto afterLogout = testing::httpGet(url(QStringLiteral("/api/v1/auth/me")), login->token);
        QCOMPARE(afterLogout.status, 401);
        QCOMPARE(errorCode(afterLogout.body), QStringLiteral("auth.unauthenticated"));
    }

    void protectedRoutesRejectMissingAndBogusTokens() {
        MODULO_REQUIRE_TEST_DATABASE();

        const auto missing = testing::httpGet(url(QStringLiteral("/api/v1/auth/me")));
        QCOMPARE(missing.status, 401);
        QCOMPARE(errorCode(missing.body), QStringLiteral("auth.unauthenticated"));

        const auto bogus = testing::httpGet(url(QStringLiteral("/api/v1/auth/me")), QStringLiteral("not-a-token"));
        QCOMPARE(bogus.status, 401);
    }

    void wrongPasswordAndUnknownEmailLookIdentical() {
        MODULO_REQUIRE_TEST_DATABASE();
        registerAccount(QStringLiteral("a@example.com"));

        const auto wrong = post(
            QStringLiteral("/api/v1/auth/login"),
            api::LoginRequest{.email = QStringLiteral("a@example.com"), .password = QStringLiteral("definitely not it")}
                .toJson());
        const auto unknown = post(QStringLiteral("/api/v1/auth/login"),
                                  api::LoginRequest{.email = QStringLiteral("nobody@example.com"),
                                                    .password = QStringLiteral("definitely not it")}
                                      .toJson());
        QCOMPARE(wrong.status, 401);
        QCOMPARE(unknown.status, 401);
        QCOMPARE(wrong.body, unknown.body); // identical envelope, no account enumeration
        QCOMPARE(errorCode(wrong.body), QStringLiteral("auth.invalid_credentials"));
    }

    void duplicateEmailIs409AndBadInputIs400() {
        MODULO_REQUIRE_TEST_DATABASE();
        registerAccount(QStringLiteral("dup@example.com"));

        const auto duplicate = post(QStringLiteral("/api/v1/auth/register"),
                                    api::RegisterRequest{.email = QStringLiteral("DUP@example.com"),
                                                         .displayName = QStringLiteral("Again"),
                                                         .password = QStringLiteral("correct horse battery")}
                                        .toJson());
        QCOMPARE(duplicate.status, 409);
        QCOMPARE(errorCode(duplicate.body), QStringLiteral("auth.email_taken"));

        const auto shortPassword = post(QStringLiteral("/api/v1/auth/register"),
                                        api::RegisterRequest{.email = QStringLiteral("new@example.com"),
                                                             .displayName = QStringLiteral("New"),
                                                             .password = QStringLiteral("short")}
                                            .toJson());
        QCOMPARE(shortPassword.status, 400);
        QCOMPARE(errorCode(shortPassword.body), QStringLiteral("password.too_short"));

        const auto notJson = testing::httpRequest("POST", url(QStringLiteral("/api/v1/auth/register")), "{oops");
        QCOMPARE(notJson.status, 400);
        QCOMPARE(errorCode(notJson.body), QStringLiteral("api.invalid_json"));
    }

    void secondAccountIsPlainUser() {
        MODULO_REQUIRE_TEST_DATABASE();
        registerAccount(QStringLiteral("first@example.com"));

        const auto second = post(QStringLiteral("/api/v1/auth/register"),
                                 api::RegisterRequest{.email = QStringLiteral("second@example.com"),
                                                      .displayName = QStringLiteral("Second"),
                                                      .password = QStringLiteral("correct horse battery")}
                                     .toJson());
        QCOMPARE(second.status, 201);
        QCOMPARE(api::UserDto::fromJson(object(second.body))->roles, QStringList{QStringLiteral("user")});
    }

private:
    QUrl url(const QString& path) const { return baseUrl_.resolved(QUrl{path}); }

    testing::HttpResponse post(const QString& path, const QJsonObject& body) const {
        return testing::httpRequest("POST", url(path), QJsonDocument{body}.toJson(QJsonDocument::Compact));
    }

    void registerAccount(const QString& email) {
        const auto response = post(QStringLiteral("/api/v1/auth/register"),
                                   api::RegisterRequest{.email = email,
                                                        .displayName = QStringLiteral("User"),
                                                        .password = QStringLiteral("correct horse battery")}
                                       .toJson());
        QCOMPARE(response.status, 201);
    }

    static QJsonObject object(const QByteArray& body) { return QJsonDocument::fromJson(body).object(); }

    static QString errorCode(const QByteArray& body) {
        const auto error = api::ErrorResponse::fromJson(object(body));
        return error ? error->code : QStringLiteral("<no envelope>");
    }

    std::unique_ptr<db::ConnectionPool> pool_;
    std::unique_ptr<auth::AuthService> service_;
    std::unique_ptr<http::Server> server_;
    QUrl baseUrl_;
};

QTEST_GUILESS_MAIN(AuthFlowTest)
#include "test_auth_flow.moc"
