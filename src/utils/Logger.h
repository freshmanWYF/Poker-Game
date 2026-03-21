#ifndef LOGGER_H
#define LOGGER_H

#include <QtCore/QString>
#include <QtCore/QObject>

class Logger : public QObject {
    Q_OBJECT
public:
    static Logger& instance();
    void log(const QString& message);

signals:
    void messageLogged(const QString& message);

private:
    Logger() = default;
};

#endif // LOGGER_H
