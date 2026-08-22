#include <modulo/core/version.h>

#include <QRegularExpression>
#include <QTest>

class VersionTest : public QObject {
    Q_OBJECT

private slots:

    void reportsTheCMakeProjectVersion() {
        // Both the library and this test receive MODULO_VERSION from the toolkit.
        QCOMPARE(modulo::core::version(), QStringLiteral(MODULO_VERSION));
    }

    void hasSemanticVersionShape() {
        const QRegularExpression semver{QStringLiteral(R"(^\d+\.\d+\.\d+$)")};
        QVERIFY(semver.match(modulo::core::version()).hasMatch());
    }
};

QTEST_GUILESS_MAIN(VersionTest)
#include "test_version.moc"
