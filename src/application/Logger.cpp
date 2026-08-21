#include "Logger.hpp"

Logger::Logger(QObject* parent)
	: QObject(parent)
{
}

void Logger::info(const QString& message)
{
	log("[INFO]: ", message);
}

void Logger::warn(const QString& message)
{
	log("[WARN]: ", message);
}

void Logger::error(const QString& message)
{
	log("[ERRO]: ", message);
}

void Logger::log(const QString& prefix, const QString& message)
{
	emit messageLogged(prefix + message);
}

Logger& logger()
{
	static Logger instance;
	return instance;
}
