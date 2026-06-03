#include "WebSocketServer.h"
#include "../utils/Logger.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QTimer>

WebSocketServer::WebSocketServer(GameEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine), m_server(new QWebSocketServer("PokerGame", QWebSocketServer::NonSecureMode, this)) {}

WebSocketServer::~WebSocketServer() { stop(); }

bool WebSocketServer::start(int port) {
    if (m_server->isListening()) m_server->close();
    if (!m_server->listen(QHostAddress::AnyIPv4, port)) return false;
    connect(m_server, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);

    // 心跳定时器：每 5 秒 ping 一次所有客户端，检测断连
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &WebSocketServer::heartbeat);
    m_heartbeatTimer->start(5000);

    Logger::instance().log(QString("WebSocket 服务器已启动，端口: %1").arg(port));
    return true;
}

void WebSocketServer::stop() {
    if (m_heartbeatTimer) m_heartbeatTimer->stop();
    for (auto* socket : m_clients.values()) {
        socket->close();
    }
    m_clients.clear();
    m_reverseMap.clear();
    if (m_server->isListening()) m_server->close();
}

void WebSocketServer::heartbeat() {
    for (auto it = m_clients.constBegin(); it != m_clients.constEnd(); ++it) {
        QWebSocket* socket = it.value();
        if (socket && socket->isValid()) {
            socket->ping();
        }
    }
}

void WebSocketServer::onNewConnection() {
    while (auto* raw = m_server->nextPendingConnection()) {
        QWebSocket* socket = raw;
        connect(socket, &QWebSocket::textMessageReceived, this, &WebSocketServer::onTextMessage);
        connect(socket, &QWebSocket::errorOccurred, this, &WebSocketServer::onSocketError);
        connect(socket, &QWebSocket::disconnected, this, &WebSocketServer::onDisconnected);
        Logger::instance().log("手机客户端已连接 (WebSocket)");
    }
}

void WebSocketServer::onSocketError(QAbstractSocket::SocketError) {
    auto* socket = qobject_cast<QWebSocket*>(sender());
    if (socket) {
        Logger::instance().log(QString("WebSocket 错误: %1").arg(socket->errorString()));
    }
}

void WebSocketServer::onDisconnected() {
    auto* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    int clientId = m_reverseMap.value(socket, -1);
    if (clientId >= 0) {
        m_clients.remove(clientId);
        m_reverseMap.remove(socket);
        emit clientDisconnected(clientId);
    }
    socket->deleteLater();
}

void WebSocketServer::onTextMessage(const QString& message) {
    auto* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    QJsonObject data = QJsonDocument::fromJson(message.toUtf8()).object();
    QString type = data["type"].toString();

    if (type == "join") {
        handleJoin(socket, data);
    } else if (type == "action") {
        handleAction(socket, data);
    }
}

void WebSocketServer::handleJoin(QWebSocket* socket, const QJsonObject& data) {
    QString name = data["name"].toString("手机玩家");
    int clientId = m_nextClientId++;
    m_clients[clientId] = socket;
    m_reverseMap[socket] = clientId;

    // 注意：不发送 welcome 消息，由 GameController 通过 sendWelcome 发送正确的玩家索引
    // 通知 GameController 处理加入逻辑（此时 m_reverseMap 已有映射）
    emit clientConnected(clientId, name);

    Logger::instance().log(QString("手机玩家 %1 已加入，clientId=%2").arg(name).arg(clientId));
}

void WebSocketServer::handleAction(QWebSocket* socket, const QJsonObject& data) {
    int clientId = m_reverseMap.value(socket, -1);
    if (clientId < 0) return;

    // 转发动作给 GameController
    emit actionReceived(clientId, data);
}

void WebSocketServer::sendJson(QWebSocket* socket, const QJsonObject& obj) {
    if (socket && socket->isValid()) {
        socket->sendTextMessage(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}

void WebSocketServer::sendJsonToAll(const QJsonObject& obj) {
    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    for (auto* socket : m_clients.values()) {
        if (socket && socket->isValid()) {
            socket->sendTextMessage(data);
        }
    }
}

void WebSocketServer::syncPlayerIndex(int clientId, int playerId) {
    m_clientToPlayerMap[clientId] = playerId;
}

void WebSocketServer::sendWelcome(int clientId, int playerId) {
    QWebSocket* socket = m_clients.value(clientId, nullptr);
    if (!socket) return;
    QJsonObject welcome;
    welcome["type"] = "welcome";
    welcome["id"] = playerId;
    sendJson(socket, welcome);
    Logger::instance().log(QString("[WS] 发送 welcome: clientId=%1 → playerId=%2").arg(clientId).arg(playerId));
}

void WebSocketServer::sendToClient(int clientId, const QJsonObject& obj) {
    QWebSocket* socket = m_clients.value(clientId, nullptr);
    if (socket && socket->isValid()) {
        socket->sendTextMessage(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}
