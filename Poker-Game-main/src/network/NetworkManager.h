#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QtCore/QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QList>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject* parent = nullptr);
    virtual ~NetworkManager();

    // 房主端接口
    bool startServer(int port = 12345);
    void stopServer();
    void broadcast(const QJsonObject& msg);
    void sendToPlayer(int playerId, const QJsonObject& msg);
    QStringList getJoinAddresses() const;
    int listenPort() const { return m_listenPort; }

    // 客户端接口
    bool connectToHost(const QString& address, int port = 12345);
    void disconnectFromHost();
    void sendToServer(const QJsonObject& msg);

    bool isHost() const { return m_isHost; }
    bool isConnected() const { return m_socket->state() == QAbstractSocket::ConnectedState; }
    QList<QTcpSocket*> getClientSockets() const { return m_clientSockets; }

signals:
    // 房主端信号
    void playerConnected(int playerId);
    void playerDisconnected(int playerId);
    void dataReceivedFromPlayer(int playerId, const QJsonObject& msg);

    // 客户端信号
    void connected();
    void disconnected();
    void dataReceivedFromServer(const QJsonObject& msg);
    void errorOccurred(const QString& error);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();
    void onSocketReadyRead();

private:
    bool m_isHost;
    QTcpServer* m_server;
    QTcpSocket* m_socket; // 客户端模式下的套接字
    QList<QTcpSocket*> m_clientSockets; // 房主模式下的所有客户端套接字
    int m_listenPort = 0;
};

#endif // NETWORKMANAGER_H
