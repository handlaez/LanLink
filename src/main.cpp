#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QDirIterator>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    engine.loadFromModule("LanLinkApp", "Main");

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML module LanLinkApp/Main";
        return -1;
    }

    return app.exec();
}