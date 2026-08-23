#include <modulo/server/auth/roles.h>

#include <QTest>

using namespace modulo::server::auth;

class RolesTest : public QObject {
    Q_OBJECT

private slots:

    void namesAndIdsMatchTheSchemaCatalogue() {
        // Ids are the rows seeded by 0002_auth.sql; names are the wire form.
        QCOMPARE(static_cast<qint16>(Role::Admin), qint16{1});
        QCOMPARE(static_cast<qint16>(Role::User), qint16{2});
        QCOMPARE(roleName(Role::Admin), QStringLiteral("admin"));
        QCOMPARE(roleName(Role::User), QStringLiteral("user"));
    }

    void roundTripsThroughNamesAndIds() {
        for (const Role role : {Role::Admin, Role::User}) {
            QCOMPARE(roleFromName(roleName(role)), std::optional{role});
            QCOMPARE(roleFromId(static_cast<qint16>(role)), std::optional{role});
        }
    }

    void rejectsUnknownValues() {
        QVERIFY(!roleFromName(QStringLiteral("root")).has_value());
        QVERIFY(!roleFromName(QStringLiteral("Admin")).has_value()); // names are exact, lower-case
        QVERIFY(!roleFromId(0).has_value());
        QVERIFY(!roleFromId(3).has_value());
    }
};

QTEST_GUILESS_MAIN(RolesTest)
#include "test_roles.moc"
