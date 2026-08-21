#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <QObject>
#include <QString>

class Logger : public QObject
{
    Q_OBJECT

public:
    explicit Logger(QObject* parent = nullptr);

    void info(const QString& message);
    void warn(const QString& message);
    void error(const QString& message);

signals:
    void messageLogged(const QString& message);

private:
    void log(const QString& prefix, const QString& message);
};

Logger& logger();

#endif