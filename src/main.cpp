#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QDirIterator>
#include <QQuickStyle>

#include <iostream>
#include <atomic>

#include "src/application/Consumer.hpp"
#include "src/application/UnifiedArgParser.hpp"
#ifdef _WIN32
#include "src/application/Producer.hpp"
#endif

int main(int argc, char* argv[])
{
    try {
        const Config config = ArgParser::parse(argc, argv);
        // a temporary bool (testing)
        std::atomic<bool> running = true;

        if (!config.gui) {
            // Terminal mode
            if (config.producer) {
#ifdef _WIN32
                Producer producer;
                producer.initialize(config.ip, config.port);
                producer.run(running);
#else
                std::cerr << "Producer mode is not yet implemented on Linux. Sorry!\n";
#endif
            }
            else {
#ifdef _WIN32
                Consumer consumer;
                consumer.initialize(config.port);
                consumer.run(running);
#else
                Consumer consumer;
                consumer.initialize(config.port);
                consumer.run(running);
#endif
            }

            return 0;
        }

        // GUI mode
        QGuiApplication app(argc, argv);
        QQuickStyle::setStyle("Fusion");
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
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}