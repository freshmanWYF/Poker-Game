#include "WebSocketServer.h"
#include "../utils/Logger.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QTimer>

WebSocketServer::WebSocketServer(GameEngine* engine, QObject* parent)
    : QObject(parent), m_server(new QWebSocketServer("PokerGame", QWebSocketServer::NonSecureMode, this)), m_engine(engine) {
    connect(m_server, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &WebSocketServer::heartbeat);
}

WebSocketServer::~WebSocketServer() { stop(); }

bool WebSocketServer::start(int port) {
    if (m_server->isListening()) return true;
    if (!m_server->listen(QHostAddress::AnyIPv4, port)) return false;

    // 心跳定时器：每 10 秒 ping 一次所有客户端，检测断连
    m_heartbeatTimer->start(10000);

    Logger::instance().log(QString("WebSocket 服务器已启动，端口: %1").arg(m_server->serverPort()));
    return true;
}

void WebSocketServer::stop() {
    if (m_heartbeatTimer) m_heartbeatTimer->stop();
    const auto sockets = m_sockets;
    for (auto* socket : sockets) {
        socket->disconnect(this);
        socket->close();
        socket->deleteLater();
    }
    m_clients.clear();
    m_reverseMap.clear();
    m_sockets.clear();
    m_awaitingPong.clear();
    if (m_server->isListening()) m_server->close();
}

void WebSocketServer::heartbeat() {
    const auto sockets = m_sockets;
    for (auto* socket : sockets) {
        if (m_awaitingPong.contains(socket)) {
            socket->abort();
            continue;
        }
        m_awaitingPong.insert(socket);
        socket->ping();
    }
}

void WebSocketServer::onNewConnection() {
    while (auto* raw = m_server->nextPendingConnection()) {
        QWebSocket* socket = raw;
        socket->setParent(this);
        socket->setMaxAllowedIncomingMessageSize(16384);
        m_sockets.insert(socket);
        connect(socket, &QWebSocket::pong, this, [this, socket](quint64, const QByteArray&) {
            m_awaitingPong.remove(socket);
        });
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

    m_sockets.remove(socket);
    m_awaitingPong.remove(socket);
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
    if (m_reverseMap.contains(socket)) return;
    if (m_engine->getCurrentPhase() != GameConstants::Settlement ||
        m_clients.size() >= GameConstants::MAX_PLAYERS - 1) {
        sendJson(socket, {{"type", "error"}, {"message", "本局进行中或房间已满，请等待下一局再加入。"}});
        socket->close();
        return;
    }
    QString name = data["name"].toString().simplified().left(24);
    if (name.isEmpty()) name = "手机玩家";
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

void WebSocketServer::rejectClient(int clientId, const QString& reason) {
    auto* socket = m_clients.value(clientId, nullptr);
    if (!socket) return;
    sendJson(socket, {{"type", "error"}, {"message", reason}});
    socket->close();
}
