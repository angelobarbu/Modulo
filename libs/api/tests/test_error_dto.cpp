#include <modulo/api/error.h>

#include <QJsonObject>
#include <QTest>

using modulo::api::ErrorResponse;

class ErrorDtoTest : public QObject {
    Q_OBJECT

private slots:

    void serializesToTheUniformEnvelope() {
        const ErrorResponse error{.code = QStringLiteral("not_found"), .message = QStringLiteral("resource not found")};

        const QJsonObject json = error.toJson();
        QCOMPARE(json.size(), 1); // nothing outside the "error" envelope
        const QJsonObject envelope = json.value(QLatin1StringView{"error"}).toObject();
        QCOMPARE(envelope.value(QLatin1StringView{"code"}).toString(), QStringLiteral("not_found"));
        QCOMPARE(envelope.value(QLatin1StringView{"message"}).toString(), QStringLiteral("resource not found"));

        const auto parsed = ErrorResponse::fromJson(json);
        QVERIFY(parsed.has_value());
        QCOMPARE(parsed->code, error.code);
        QCOMPARE(parsed->message, error.message);
    }

    void rejectsInvalidWireData_data() {
        QTest::addColumn<QJsonObject>("json");
        QTest::addColumn<QString>("offendingField");

        QTest::newRow("flat object without the envelope")
            << QJsonObject{{QStringLiteral("code"), QStringLiteral("x")},
                           {QStringLiteral("message"), QStringLiteral("y")}}
            << QStringLiteral("error");
        QTest::newRow("envelope missing the code")
            << QJsonObject{{QStringLiteral("error"), QJsonObject{{QStringLiteral("message"), QStringLiteral("y")}}}}
            << QStringLiteral("code");
        QTest::newRow("envelope is not an object")
            << QJsonObject{{QStringLiteral("error"), QStringLiteral("oops")}} << QStringLiteral("error");
    }

    void rejectsInvalidWireData() {
        QFETCH(QJsonObject, json);
        QFETCH(QString, offendingField);

        const auto result = ErrorResponse::fromJson(json);
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, QStringLiteral("api.invalid_field"));
        QVERIFY(result.error().message.contains(offendingField));
    }
};

QTEST_GUILESS_MAIN(ErrorDtoTest)
#include "test_error_dto.moc"
