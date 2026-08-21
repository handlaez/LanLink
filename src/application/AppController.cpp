#include <qmetaobject.h>
#include <string>
#include <qhostaddress.h>

#include "AppController.hpp"
#include "Logger.hpp"

AppController::AppController(QObject* parent) : QObject(parent)
{
    connect(
        &logger(),
        &Logger::messageLogged,
        this,
        &AppController::appendLog,
        Qt::QueuedConnection
    );
}

AppController::~AppController()
{
	stop();
}

void AppController::start(const QString& ip, int port) {
    if (isStreaming_) 
        return;

    QHostAddress address;
    if (!address.setAddress(ip)) {
        setStatusText("Invalid IP address.");
        return;
    }

    if (port < 1 || port > 65535) {
        setStatusText("Invalid port.");
        return;
    }

    setStatusText("Initializing...");
    setStreaming(true);

    std::string ipStd = ip.toStdString();
    uint16_t portNum = static_cast<uint16_t>(port);

    running_.store(true);
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
        app.run(running_);
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
        app.run(running_);
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

    running_.store(false);
    if (workerThread_) {
        workerThread_->wait();
    }

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

void AppController::appendLog(const QString& message)
{
    logLines_.append(message);

    constexpr qsizetype maxLogLines = 1000;

    if (logLines_.size() > maxLogLines)
        logLines_.removeFirst();

    emit logLinesChanged();
}
