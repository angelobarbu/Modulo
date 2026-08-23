#include <modulo/core/password_policy.h>

#include <QTest>

using namespace modulo::core;

class PasswordPolicyTest : public QObject {
    Q_OBJECT

private slots:

    void acceptsPasswordsWithinBounds_data() {
        QTest::addColumn<QString>("password");

        QTest::newRow("exactly minimum") << QString{kMinPasswordLength, u'a'};
        QTest::newRow("typical") << QStringLiteral("correct horse battery");
        QTest::newRow("exactly maximum") << QString{kMaxPasswordLength, u'z'};
        QTest::newRow("unicode counts characters, not bytes") << QString{kMinPasswordLength, u'é'};
    }

    void acceptsPasswordsWithinBounds() {
        QFETCH(QString, password);
        QVERIFY(validatePassword(password).has_value());
    }

    void rejectsPasswordsOutsideBounds_data() {
        QTest::addColumn<QString>("password");
        QTest::addColumn<QString>("code");

        QTest::newRow("empty") << QString{} << QStringLiteral("password.too_short");
        QTest::newRow("one below minimum")
            << QString{kMinPasswordLength - 1, u'a'} << QStringLiteral("password.too_short");
        QTest::newRow("one above maximum")
            << QString{kMaxPasswordLength + 1, u'a'} << QStringLiteral("password.too_long");
    }

    void rejectsPasswordsOutsideBounds() {
        QFETCH(QString, password);
        QFETCH(QString, code);

        const auto result = validatePassword(password);
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, code);
    }
};

QTEST_GUILESS_MAIN(PasswordPolicyTest)
#include "test_password_policy.moc"
