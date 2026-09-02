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

Q_INVOKABLE void AppController::startProducer(const QString& ip, int port)
{
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

    const std::string ipStd = ip.toStdString();
    const uint16_t portNum = static_cast<uint16_t>(port);

    running_.store(true);

    workerThread_ = QThread::create([this, ipStd, portNum]() {
        Producer app;

        if (!app.initialize(ipStd.c_str(), portNum)) {
            QMetaObject::invokeMethod(this, [this]() {
                setStatusText("Failed to initialize Producer.");
                setStreaming(false);
                });
            return;
        }

        QMetaObject::invokeMethod(this, [this]() {
            setStatusText("Streaming...");
            });

        app.run(running_);

        QMetaObject::invokeMethod(this, [this]() {
            setStatusText("Stopped");
            setStreaming(false);
            });
        });

    workerThread_->start();
}

Q_INVOKABLE void AppController::startConsumer(int port)
{
    if (isStreaming_)
        return;

    if (port < 1 || port > 65535) {
        setStatusText("Invalid port.");
        return;
    }

    setStatusText("Initializing...");
    setStreaming(true);

    const uint16_t portNum = static_cast<uint16_t>(port);

    running_.store(true);

    workerThread_ = QThread::create([this, portNum]() {
        Consumer app;

        if (!app.initialize(portNum)) {
            QMetaObject::invokeMethod(this, [this]() {
                setStatusText("Failed to initialize Consumer.");
                setStreaming(false);
                });
            return;
        }

        QMetaObject::invokeMethod(this, [this]() {
            setStatusText("Receiving...");
            });

        app.run(running_);

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
