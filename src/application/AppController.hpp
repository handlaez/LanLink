#ifndef APP_CONTROLER_HPP
#define APP_CONTROLER_HPP

#include <QObject>
#include <QThread>
#include <QString>
#include <qqmlintegration.h>
#include <atomic>

#ifdef _WIN32
#include "Producer.hpp"
#else
#include "Consumer.hpp"
#endif

class AppController : public QObject {
    Q_OBJECT
    QML_ELEMENT
        Q_PROPERTY(bool isStreaming READ isStreaming NOTIFY isStreamingChanged)
        Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController();

    bool isStreaming() const { return isStreaming_; }
    QString statusText() const { return statusText_; }

    Q_INVOKABLE void start(const QString& ip, int port);
    Q_INVOKABLE void stop();

signals:
    void isStreamingChanged();
    void statusTextChanged();

private:
    void setStreaming(bool streaming);
    void setStatusText(const QString& text);

    std::atomic<bool> running_{ false };

    QThread* workerThread_ = nullptr;
    bool isStreaming_ = false;
    QString statusText_ = "Idle";
};

#endif