#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QTimer>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>
#include "../core/GameEngine.h"

class WebSocketServer : public QObject {
    Q_OBJECT
public:
    WebSocketServer(GameEngine* engine, QObject* parent = nullptr);
    ~WebSocketServer();

    bool start(int port = 12347);
    void stop();
    int clientCount() const { return m_clients.size(); }

    // 告知某 clientId 对应的玩家索引
    void syncPlayerIndex(int clientId, int playerId);

    // 发送欢迎消息（包含正确的游戏玩家索引）
    void sendWelcome(int clientId, int playerId);

    // 发送 JSON 给指定 clientId
    void sendToClient(int clientId, const QJsonObject& obj);

signals:
    void clientConnected(int clientId, const QString& name);
    void clientDisconnected(int clientId);
    void actionReceived(int clientId, const QJsonObject& data); // 由 GameController 处理

private slots:
    void onNewConnection();
    void onTextMessage(const QString& message);
    void onSocketError(QAbstractSocket::SocketError error);
    void onDisconnected();
    void heartbeat();

private:
    void handleJoin(QWebSocket* socket, const QJsonObject& data);
    void handleAction(QWebSocket* socket, const QJsonObject& data);
    void sendJson(QWebSocket* socket, const QJsonObject& obj);
    void sendJsonToAll(const QJsonObject& obj);

    // clientId → 玩家索引 映射
    QMap<int, int> m_clientToPlayerMap;

    QWebSocketServer* m_server;
    GameEngine* m_engine;
    QMap<int, QWebSocket*> m_clients;
    QMap<QWebSocket*, int> m_reverseMap;
    int m_nextClientId = 1;
    QTimer* m_heartbeatTimer = nullptr;
};

#endif // WEBSOCKETSERVER_H
