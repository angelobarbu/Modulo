#include <modulo/server/auth/password_hasher.h>

#include <QTest>

using modulo::server::auth::PasswordHasher;

class PasswordHasherTest : public QObject {
    Q_OBJECT

private slots:

    void hashIsArgon2idAndVerifies() {
        const auto hash = PasswordHasher::hash(QStringLiteral("correct horse battery"));
        QVERIFY(hash.has_value());
        QVERIFY(hash->startsWith(QStringLiteral("$argon2id$")));
        QVERIFY(PasswordHasher::verify(*hash, QStringLiteral("correct horse battery")));
    }

    void wrongPasswordDoesNotVerify() {
        const auto hash = PasswordHasher::hash(QStringLiteral("correct horse battery"));
        QVERIFY(hash.has_value());
        QVERIFY(!PasswordHasher::verify(*hash, QStringLiteral("correct horse batteries")));
        QVERIFY(!PasswordHasher::verify(*hash, QString{}));
    }

    void sameHashNeverRepeats() {
        // A fresh random salt per hash: equal passwords must yield different strings.
        const auto first = PasswordHasher::hash(QStringLiteral("same password"));
        const auto second = PasswordHasher::hash(QStringLiteral("same password"));
        QVERIFY(first.has_value() && second.has_value());
        QVERIFY(*first != *second);
    }

    void unicodePasswordsRoundTrip() {
        const QString password = QStringLiteral("pässwörd-日本語-🙂");
        const auto hash = PasswordHasher::hash(password);
        QVERIFY(hash.has_value());
        QVERIFY(PasswordHasher::verify(*hash, password));
    }

    void malformedHashesNeverVerify() {
        QVERIFY(!PasswordHasher::verify(QString{}, QStringLiteral("anything")));
        QVERIFY(!PasswordHasher::verify(QStringLiteral("not a hash"), QStringLiteral("anything")));
        QVERIFY(!PasswordHasher::verify(QString{200, u'x'}, QStringLiteral("anything"))); // longer than STRBYTES
    }

    void freshHashDoesNotNeedRehash() {
        const auto hash = PasswordHasher::hash(QStringLiteral("correct horse battery"));
        QVERIFY(hash.has_value());
        QVERIFY(!PasswordHasher::needsRehash(*hash));
        QVERIFY(PasswordHasher::needsRehash(QStringLiteral("garbage"))); // unparsable → rehash
    }
};

QTEST_GUILESS_MAIN(PasswordHasherTest)
#include "test_password_hasher.moc"
