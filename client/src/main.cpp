// modulo_client — Modulo QML desktop application.

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include <cstdlib>

int main(int argc, char* argv[]) {
    QGuiApplication app{argc, argv};
    QGuiApplication::setApplicationName(QStringLiteral("Modulo"));
    QGuiApplication::setOrganizationName(QStringLiteral("Modulo"));

    // Material is the base Controls style; the Modulo dark theme layers on top.
    QQuickStyle::setStyle(QStringLiteral("Material"));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);
    engine.loadFromModule("Modulo", "Main");

    return app.exec();
}
