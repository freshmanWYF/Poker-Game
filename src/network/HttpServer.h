#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QtCore/QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

class HttpServer : public QObject {
    Q_OBJECT
public:
    explicit HttpServer(QObject* parent = nullptr);
    ~HttpServer();

    bool start(int port = 8080);
    void stop();
    int serverPort() const { return m_port; }
    QString getLocalIP() const;
    QString serverUrl() const;
    static QByteArray generateQRCodePNG(const QString& text, int size = 256);

signals:
    void serverStarted(const QString& url);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    void serveFile(QTcpSocket* client, const QString& path);
    void sendResponse(QTcpSocket* client, const QByteArray& body,
                      const QString& contentType, int statusCode = 200);
    void send404(QTcpSocket* client);

    QTcpServer* m_server;
    int m_port = 0;
};

#endif // HTTPSERVER_H