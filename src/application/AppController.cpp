#include <qmetaobject.h>
#include <string>

#include "AppController.hpp"

AppController::AppController(QObject* parent) : QObject(parent){}

AppController::~AppController()
{
	stop();
}

void AppController::start(const QString& ip, int port) {
    if (isStreaming_) return;

    setStatusText("Initializing...");
    setStreaming(true);

    std::string ipStd = ip.toStdString();
    uint16_t portNum = static_cast<uint16_t>(port);

    workerThread_ = QThread::create([this, ipStd, portNum]() {
#ifdef _WIN32
        Producer app;
        if (!app.initialize(ipStd.c_str(), portNum)) {
            QMetaObject::invokeMethod(this, [this]() {
                setStatusText("Failed to initialize Producer.");
                setStreaming(false);
                });
            return;
        }
        QMetaObject::invokeMethod(this, [this]() { setStatusText("Streaming..."); });
        app.run();
#else
        Consumer app;
        if (!app.initialize(portNum)) {
            QMetaObject::invokeMethod(this, [this]() {
                setStatusText("Failed to initialize Consumer.");
                setStreaming(false);
                });
            return;
        }
        QMetaObject::invokeMethod(this, [this]() { setStatusText("Receiving..."); });
        app.run();
#endif

        QMetaObject::invokeMethod(this, [this]() {
            setStatusText("Stopped");
            setStreaming(false);
            });
        });

    workerThread_->start();
}

void AppController::stop() {
    if (!isStreaming_ || !workerThread_) return;

    workerThread_->requestInterruption();
    workerThread_->quit();
    workerThread_->wait();

    delete workerThread_;
    workerThread_ = nullptr;

    setStreaming(false);
    setStatusText("Idle");
}

void AppController::setStreaming(bool streaming) {
    if (isStreaming_ != streaming) {
        isStreaming_ = streaming;
        emit isStreamingChanged();
    }
}

void AppController::setStatusText(const QString& text) {
    if (statusText_ != text) {
        statusText_ = text;
        emit statusTextChanged();
    }
}