#include <modulo/api/json.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

namespace json = modulo::api::json;

class JsonHelpersTest : public QObject {
    Q_OBJECT

private slots:

    void requireStringReturnsPresentStrings() {
        const QJsonObject object{{QStringLiteral("name"), QStringLiteral("modulo")}};

        const auto value = json::requireString(object, QLatin1StringView{"name"});
        QVERIFY(value.has_value());
        QCOMPARE(*value, QStringLiteral("modulo"));
    }

    void requireStringNeverUsesSilentDefaults_data() {
        QTest::addColumn<QString>("key");

        // Every non-string shape QJson would otherwise coerce to "".
        QTest::newRow("missing key") << QStringLiteral("missing");
        QTest::newRow("number") << QStringLiteral("count");
        QTest::newRow("object") << QStringLiteral("nested");
        QTest::newRow("array") << QStringLiteral("list");
        QTest::newRow("null") << QStringLiteral("nothing");
    }

    void requireStringNeverUsesSilentDefaults() {
        QFETCH(QString, key);
        const QJsonObject object{{QStringLiteral("count"), 3},
                                 {QStringLiteral("nested"), QJsonObject{}},
                                 {QStringLiteral("list"), QJsonArray{}},
                                 {QStringLiteral("nothing"), QJsonValue::Null}};

        const auto value = json::requireString(object, QLatin1StringView{key.toLatin1()});
        QVERIFY(!value.has_value());
        QCOMPARE(value.error().code, QStringLiteral("api.invalid_field"));
        QVERIFY(value.error().message.contains(key));
    }

    void requireObjectDistinguishesObjects() {
        const QJsonObject object{{QStringLiteral("inner"), QJsonObject{{QStringLiteral("k"), QStringLiteral("v")}}},
                                 {QStringLiteral("text"), QStringLiteral("not an object")}};

        const auto inner = json::requireObject(object, QLatin1StringView{"inner"});
        QVERIFY(inner.has_value());
        QCOMPARE(inner->value(QLatin1StringView{"k"}).toString(), QStringLiteral("v"));

        const auto text = json::requireObject(object, QLatin1StringView{"text"});
        QVERIFY(!text.has_value());
        QCOMPARE(text.error().code, QStringLiteral("api.invalid_field"));
    }
};

QTEST_GUILESS_MAIN(JsonHelpersTest)
#include "test_json.moc"
