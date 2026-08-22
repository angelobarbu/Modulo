#include <modulo/api/health.h>

#include <QJsonObject>
#include <QTest>

using modulo::api::HealthResponse;

class HealthDtoTest : public QObject {
    Q_OBJECT

private slots:

    void roundTripsThroughJson() {
        const HealthResponse original{.status = QStringLiteral("ok"), .version = QStringLiteral("1.2.3")};

        const QJsonObject json = original.toJson();
        QCOMPARE(json.value(QLatin1StringView{"status"}).toString(), QStringLiteral("ok"));
        QCOMPARE(json.value(QLatin1StringView{"version"}).toString(), QStringLiteral("1.2.3"));

        const auto parsed = HealthResponse::fromJson(json);
        QVERIFY(parsed.has_value());
        QCOMPARE(parsed->status, original.status);
        QCOMPARE(parsed->version, original.version);
    }

    void rejectsInvalidWireData_data() {
        QTest::addColumn<QJsonObject>("json");
        QTest::addColumn<QString>("offendingField");

        QTest::newRow("missing version") << QJsonObject{{QStringLiteral("status"), QStringLiteral("ok")}}
                                         << QStringLiteral("version");
        QTest::newRow("status is a number, not coerced")
            << QJsonObject{{QStringLiteral("status"), 42}, {QStringLiteral("version"), QStringLiteral("1.0.0")}}
            << QStringLiteral("status");
        QTest::newRow("empty object") << QJsonObject{} << QStringLiteral("status");
    }

    void rejectsInvalidWireData() {
        QFETCH(QJsonObject, json);
        QFETCH(QString, offendingField);

        const auto result = HealthResponse::fromJson(json);
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, QStringLiteral("api.invalid_field"));
        QVERIFY(result.error().message.contains(offendingField));
    }
};

QTEST_GUILESS_MAIN(HealthDtoTest)
#include "test_health_dto.moc"
