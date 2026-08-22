#include <modulo/server/config/config.h>

#include <QByteArray>
#include <QHash>
#include <QTest>

#include <initializer_list>
#include <utility>

using modulo::server::config::Config;

namespace {

/// Sets MODULO_* variables for one test and restores the previous environment
/// on destruction, so tests cannot leak state into each other.
class ScopedEnvironment {
public:
    explicit ScopedEnvironment(std::initializer_list<std::pair<const char*, const char*>> variables) {
        for (const auto& [name, value] : variables) {
            saved_.insert(name, qgetenv(name));
            if (value == nullptr) {
                qunsetenv(name);
            } else {
                qputenv(name, value);
            }
        }
    }

    ~ScopedEnvironment() {
        for (auto it = saved_.cbegin(); it != saved_.cend(); ++it) {
            if (it.value().isNull()) {
                qunsetenv(it.key());
            } else {
                qputenv(it.key(), it.value());
            }
        }
    }

    ScopedEnvironment(const ScopedEnvironment&) = delete;
    ScopedEnvironment& operator=(const ScopedEnvironment&) = delete;

private:
    QHash<const char*, QByteArray> saved_;
};

} // namespace

class ConfigTest : public QObject {
    Q_OBJECT

private slots:

    void fallsBackToDefaultsWhenNothingIsSet() {
        const ScopedEnvironment env{
            {"MODULO_DB_URL", nullptr}, {"MODULO_HTTP_PORT", nullptr}, {"MODULO_DATA_DIR", nullptr}};

        const auto config = Config::fromEnvironment();
        QVERIFY(config.has_value());
        QVERIFY(config->databaseUrl.isEmpty());
        QCOMPARE(config->httpPort, quint16{8080});
        QCOMPARE(config->dataDir, QStringLiteral("./var/data"));
    }

    void readsEveryVariable() {
        const ScopedEnvironment env{{"MODULO_DB_URL", "postgresql://u:p@localhost:5433/db"},
                                    {"MODULO_HTTP_PORT", "9090"},
                                    {"MODULO_DATA_DIR", "/tmp/modulo-data"}};

        const auto config = Config::fromEnvironment();
        QVERIFY(config.has_value());
        QCOMPARE(config->databaseUrl, QStringLiteral("postgresql://u:p@localhost:5433/db"));
        QCOMPARE(config->httpPort, quint16{9090});
        QCOMPARE(config->dataDir, QStringLiteral("/tmp/modulo-data"));
    }

    void treatsExportedButEmptyVariableAsUnset() {
        const ScopedEnvironment env{{"MODULO_HTTP_PORT", ""}, {"MODULO_DATA_DIR", ""}};

        const auto config = Config::fromEnvironment();
        QVERIFY(config.has_value());
        QCOMPARE(config->httpPort, quint16{8080});
        QCOMPARE(config->dataDir, QStringLiteral("./var/data"));
    }

    void acceptsPortZeroForOsAssignedPorts() {
        const ScopedEnvironment env{{"MODULO_HTTP_PORT", "0"}};

        const auto config = Config::fromEnvironment();
        QVERIFY(config.has_value());
        QCOMPARE(config->httpPort, quint16{0});
    }

    void rejectsMalformedPorts_data() {
        QTest::addColumn<QByteArray>("port");

        QTest::newRow("letters") << QByteArray{"abc"};
        QTest::newRow("out of range") << QByteArray{"70000"};
        QTest::newRow("negative") << QByteArray{"-1"};
        QTest::newRow("trailing garbage") << QByteArray{"80x"};
        QTest::newRow("fractional") << QByteArray{"8080.5"};
    }

    void rejectsMalformedPorts() {
        QFETCH(QByteArray, port);
        const ScopedEnvironment env{{"MODULO_HTTP_PORT", port.constData()}};

        const auto config = Config::fromEnvironment();
        QVERIFY(!config.has_value());
        QCOMPARE(config.error().code, QStringLiteral("config.invalid_port"));
        QVERIFY(config.error().message.contains(QString::fromLatin1(port)));
    }
};

QTEST_GUILESS_MAIN(ConfigTest)
#include "test_config.moc"
