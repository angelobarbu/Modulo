#include <modulo/server/auth/roles.h>
#include <modulo/server/auth/session_repository.h>
#include <modulo/server/auth/token.h>
#include <modulo/server/auth/user_repository.h>
#include <modulo/server/db/connection_pool.h>
#include <modulo/testing/integration.h>

#include <QTest>

#include <memory>

using namespace modulo;
using namespace modulo::server;

/// Exercises the connection pool and both repositories against the real
/// modulo_test database (schema from db/migrations). Each test function
/// starts from empty users/sessions tables.
class AuthRepositoriesTest : public QObject {
    Q_OBJECT

private slots:

    void init() {
        if (testing::testDatabaseUrl().isEmpty()) {
            return; // every test function QSKIPs itself
        }
        pool_ = std::make_unique<db::ConnectionPool>(testing::testDatabaseUrl().toStdString(), 2);
        auto lease = pool_->acquire();
        pqxx::work tx{lease.connection()};
        tx.exec("TRUNCATE users CASCADE"); // cascades to user_roles and sessions
        tx.commit();
    }

    void cleanup() { pool_.reset(); }

    // --- pool ---------------------------------------------------------------

    void poolOpensLazilyAndReusesConnections() {
        MODULO_REQUIRE_TEST_DATABASE();
        QCOMPARE(pool_->openConnections(), std::size_t{1}); // init() used one

        {
            auto first = pool_->acquire();
            auto second = pool_->acquire();
            QCOMPARE(pool_->openConnections(), std::size_t{2});
            QVERIFY(first->is_open() && second->is_open());
        }
        auto reused = pool_->acquire(); // no third connection is opened
        QCOMPARE(pool_->openConnections(), std::size_t{2});
    }

    // --- users --------------------------------------------------------------

    void createsUsersWithRolesAndFindsThemCaseInsensitively() {
        MODULO_REQUIRE_TEST_DATABASE();
        auth::UserRepository users{*pool_};

        const auto created = users.create(QStringLiteral("Angelo@Example.com"), QStringLiteral("Angelo"),
                                          QStringLiteral("$argon2id$fake"), {auth::Role::Admin, auth::Role::User});
        QVERIFY2(created.has_value(), qPrintable(created ? QString{} : created.error().message));
        QVERIFY(!created->id.isEmpty());
        QCOMPARE(created->roles, (QList<auth::Role>{auth::Role::Admin, auth::Role::User}));
        QVERIFY(!created->disabled);
        QVERIFY(created->createdAt.isValid());

        const auto found = users.findByEmail(QStringLiteral("angelo@example.com"));
        QVERIFY(found.has_value());
        QVERIFY(found->has_value());
        QCOMPARE((*found)->id, created->id);
        QCOMPARE((*found)->email, QStringLiteral("Angelo@Example.com")); // stored as typed, matched case-insensitively
        QCOMPARE((*found)->passwordHash, QStringLiteral("$argon2id$fake"));
        QCOMPARE((*found)->roles, created->roles);

        const auto byId = users.findById(created->id);
        QVERIFY(byId.has_value() && byId->has_value());
        QCOMPARE((*byId)->email, created->email);

        const auto count = users.count();
        QVERIFY(count.has_value());
        QCOMPARE(*count, qint64{1});
    }

    void rejectsDuplicateEmailsWithStableCode() {
        MODULO_REQUIRE_TEST_DATABASE();
        auth::UserRepository users{*pool_};

        QVERIFY(users.create(QStringLiteral("dup@example.com"), QStringLiteral("One"), QStringLiteral("h"), {}));
        const auto duplicate =
            users.create(QStringLiteral("DUP@example.com"), QStringLiteral("Two"), QStringLiteral("h"), {});
        QVERIFY(!duplicate.has_value());
        QCOMPARE(duplicate.error().code, QStringLiteral("auth.email_taken"));

        const auto count = users.count();
        QVERIFY(count.has_value());
        QCOMPARE(*count, qint64{1}); // the failed insert was rolled back
    }

    void duplicateRolesAreAssignedOnce() {
        MODULO_REQUIRE_TEST_DATABASE();
        auth::UserRepository users{*pool_};

        const auto created = users.create(QStringLiteral("r@example.com"), QStringLiteral("R"), QStringLiteral("h"),
                                          {auth::Role::User, auth::Role::User});
        QVERIFY2(created.has_value(), qPrintable(created ? QString{} : created.error().message));
        QCOMPARE(created->roles, (QList<auth::Role>{auth::Role::User}));
    }

    void malformedIdsAreNotFoundNotErrors() {
        MODULO_REQUIRE_TEST_DATABASE();
        auth::UserRepository users{*pool_};
        auth::SessionRepository sessions{*pool_};

        const auto user = users.findById(QStringLiteral("not-a-uuid"));
        QVERIFY(user.has_value() && !user->has_value());
        QCOMPARE(sessions.revoke(QStringLiteral("not-a-uuid")).error().code, QStringLiteral("auth.session_not_found"));
        QCOMPARE(sessions.touch(QStringLiteral("not-a-uuid"), QDateTime::currentDateTimeUtc()).error().code,
                 QStringLiteral("auth.session_not_found"));
        const auto revoked = sessions.revokeAllForUser(QStringLiteral("not-a-uuid"));
        QVERIFY(revoked.has_value());
        QCOMPARE(*revoked, qint64{0});
    }

    void missingUsersAreNulloptNotErrors() {
        MODULO_REQUIRE_TEST_DATABASE();
        auth::UserRepository users{*pool_};

        const auto byEmail = users.findByEmail(QStringLiteral("nobody@example.com"));
        QVERIFY(byEmail.has_value());
        QVERIFY(!byEmail->has_value());

        const auto byId = users.findById(QStringLiteral("00000000-0000-0000-0000-000000000000"));
        QVERIFY(byId.has_value());
        QVERIFY(!byId->has_value());
    }

    // --- sessions -----------------------------------------------------------

    void sessionLifecycle() {
        MODULO_REQUIRE_TEST_DATABASE();
        auth::UserRepository users{*pool_};
        auth::SessionRepository sessions{*pool_};
        const auto user = users.create(QStringLiteral("s@example.com"), QStringLiteral("S"), QStringLiteral("h"), {});
        QVERIFY(user.has_value());

        const QString token = auth::token::generate();
        const QByteArray digest = auth::token::digest(token);
        const QDateTime expires = QDateTime::currentDateTimeUtc().addDays(30);

        const auto created = sessions.create(user->id, digest, expires);
        QVERIFY2(created.has_value(), qPrintable(created ? QString{} : created.error().message));
        QCOMPARE(created->userId, user->id);
        QVERIFY(!created->revoked);
        // Millisecond precision survives the ISO-in / epoch-out round trip.
        QCOMPARE(created->expiresAt.toMSecsSinceEpoch(), expires.toMSecsSinceEpoch());

        const auto active = sessions.findActiveByDigest(digest);
        QVERIFY(active.has_value() && active->has_value());
        QCOMPARE((*active)->id, created->id);

        // The raw token is never a lookup key - only its digest is.
        const auto byRawToken = sessions.findActiveByDigest(token.toUtf8());
        QVERIFY(byRawToken.has_value() && !byRawToken->has_value());

        const QDateTime later = expires.addDays(1);
        QVERIFY(sessions.touch(created->id, later).has_value());
        const auto touched = sessions.findActiveByDigest(digest);
        QVERIFY(touched.has_value() && touched->has_value());
        QCOMPARE((*touched)->expiresAt.toMSecsSinceEpoch(), later.toMSecsSinceEpoch());
        QVERIFY((*touched)->lastSeenAt >= created->lastSeenAt);

        QVERIFY(sessions.revoke(created->id).has_value());
        const auto afterRevoke = sessions.findActiveByDigest(digest);
        QVERIFY(afterRevoke.has_value() && !afterRevoke->has_value());

        // Touching or revoking again reports the session as gone.
        QCOMPARE(sessions.revoke(created->id).error().code, QStringLiteral("auth.session_not_found"));
        QCOMPARE(sessions.touch(created->id, later).error().code, QStringLiteral("auth.session_not_found"));
    }

    void expiredSessionsAreNotActive() {
        MODULO_REQUIRE_TEST_DATABASE();
        auth::UserRepository users{*pool_};
        auth::SessionRepository sessions{*pool_};
        const auto user = users.create(QStringLiteral("e@example.com"), QStringLiteral("E"), QStringLiteral("h"), {});
        QVERIFY(user.has_value());

        const QByteArray digest = auth::token::digest(auth::token::generate());
        const auto created = sessions.create(user->id, digest, QDateTime::currentDateTimeUtc().addSecs(60));
        QVERIFY(created.has_value());

        // Expire it behind the repository's back. The schema forbids creating an
        // already-expired session and requires expires_at > created_at, so both
        // timestamps move into the past.
        {
            auto lease = pool_->acquire();
            pqxx::work tx{lease.connection()};
            tx.exec("UPDATE sessions SET created_at = now() - interval '2 seconds', "
                    "expires_at = now() - interval '1 second' WHERE id = $1::uuid",
                    pqxx::params{created->id.toStdString()});
            tx.commit();
        }

        const auto active = sessions.findActiveByDigest(digest);
        QVERIFY(active.has_value() && !active->has_value());
    }

    void revokeAllForUserOnlyTouchesThatUser() {
        MODULO_REQUIRE_TEST_DATABASE();
        auth::UserRepository users{*pool_};
        auth::SessionRepository sessions{*pool_};
        const auto alice = users.create(QStringLiteral("a@example.com"), QStringLiteral("A"), QStringLiteral("h"), {});
        const auto bob = users.create(QStringLiteral("b@example.com"), QStringLiteral("B"), QStringLiteral("h"), {});
        QVERIFY(alice.has_value() && bob.has_value());

        const QDateTime expires = QDateTime::currentDateTimeUtc().addDays(1);
        QVERIFY(sessions.create(alice->id, auth::token::digest(auth::token::generate()), expires));
        QVERIFY(sessions.create(alice->id, auth::token::digest(auth::token::generate()), expires));
        const QByteArray bobDigest = auth::token::digest(auth::token::generate());
        QVERIFY(sessions.create(bob->id, bobDigest, expires));

        const auto revoked = sessions.revokeAllForUser(alice->id);
        QVERIFY(revoked.has_value());
        QCOMPARE(*revoked, qint64{2});

        const auto bobStillActive = sessions.findActiveByDigest(bobDigest);
        QVERIFY(bobStillActive.has_value() && bobStillActive->has_value());

        const auto again = sessions.revokeAllForUser(alice->id);
        QVERIFY(again.has_value());
        QCOMPARE(*again, qint64{0});
    }

private:
    std::unique_ptr<db::ConnectionPool> pool_;
};

QTEST_GUILESS_MAIN(AuthRepositoriesTest)
#include "test_auth_repositories.moc"
