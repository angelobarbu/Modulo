#include <modulo/server/auth/token.h>

#include <QRegularExpression>
#include <QSet>
#include <QTest>

namespace token = modulo::server::auth::token;

class TokenTest : public QObject {
    Q_OBJECT

private slots:

    void generatedTokenIsUnpaddedBase64Url() {
        const QString value = token::generate();
        QCOMPARE(value.size(), 43); // ceil(32 * 4 / 3) without '=' padding
        const QRegularExpression alphabet{QStringLiteral("^[A-Za-z0-9_-]+$")};
        QVERIFY(alphabet.match(value).hasMatch());
    }

    void generatedTokensAreUnique() {
        QSet<QString> seen;
        for (int i = 0; i < 1000; ++i) {
            seen.insert(token::generate());
        }
        QCOMPARE(seen.size(), 1000);
    }

    void digestIsDeterministicSha256() {
        const QString value = token::generate();
        const QByteArray first = token::digest(value);
        QCOMPARE(first.size(), token::kDigestBytes);
        QCOMPARE(token::digest(value), first);

        // Known answer: SHA-256("abc")
        QCOMPARE(token::digest(QStringLiteral("abc")).toHex(),
                 QByteArray{"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"});
    }

    void differentTokensHaveDifferentDigests() {
        QVERIFY(token::digest(token::generate()) != token::digest(token::generate()));
    }
};

QTEST_GUILESS_MAIN(TokenTest)
#include "test_token.moc"
