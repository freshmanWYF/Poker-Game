#include "Logger.h"
#include <QtCore/QDebug>
#include <QtCore/QDateTime>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::log(const QString& message) {
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logMsg = "[" + timeStr + "] " + message;
    qDebug() << logMsg;
    emit messageLogged(logMsg);
}
