#include "NetworkManager.h"
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent), m_isHost(false) {
    m_server = new QTcpServer(this);
    m_socket = new QTcpSocket(this);

    connect(m_server, &QTcpServer::newConnection, this, &NetworkManager::onNewConnection);
    connect(m_socket, &QTcpSocket::connected, this, &NetworkManager::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkManager::disconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onSocketReadyRead);
    
    // 连接错误信号
    connect(m_socket, &QTcpSocket::errorOccurred, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_socket->errorString());
    });
}

NetworkManager::~NetworkManager() {
    stopServer();
    disconnectFromHost();
}

bool NetworkManager::startServer(int port) {
    if (m_server->listen(QHostAddress::Any, port)) {
        m_isHost = true;
        m_listenPort = port;
        return true;
    }
    return false;
}

void NetworkManager::stopServer() {
    m_server->close();
    for (auto socket : m_clientSockets) {
        socket->disconnectFromHost();
        socket->deleteLater();
    }
    m_clientSockets.clear();
    m_isHost = false;
    m_listenPort = 0;
}

void NetworkManager::broadcast(const QJsonObject& msg) {
    QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact) + "\n";
    for (auto socket : m_clientSockets) {
        socket->write(data);
    }
}

void NetworkManager::sendToPlayer(int playerId, const QJsonObject& msg) {
    if (playerId >= 0 && playerId < m_clientSockets.size()) {
        QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact) + "\n";
        m_clientSockets[playerId]->write(data);
    }
}

QStringList NetworkManager::getJoinAddresses() const {
    QStringList tailscaleIps;
    QStringList lanIps;
    QStringList otherIps;

    const int port = (m_listenPort > 0) ? m_listenPort : 12345;
    const QHostAddress tailscaleBase(QStringLiteral("100.64.0.0"));

    for (const auto& iface : QNetworkInterface::allInterfaces()) {
        if (!iface.flags().testFlag(QNetworkInterface::IsUp) ||
            !iface.flags().testFlag(QNetworkInterface::IsRunning)) {
            continue;
        }

        const QString ifaceKey = (iface.humanReadableName() + " " + iface.name()).toLower();
        const bool isTailscaleIface = ifaceKey.contains("tailscale");

        for (const auto& entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() != QAbstractSocket::IPv4Protocol) continue;
            if (ip.isNull() || ip.isLoopback()) continue;

            const QString ipStr = ip.toString();
            if (isTailscaleIface && ip.isInSubnet(tailscaleBase, 10)) {
                tailscaleIps << ipStr;
            } else if (ipStr.startsWith("192.168.") || ipStr.startsWith("10.") || ip.isInSubnet(QHostAddress("172.16.0.0"), 12)) {
                lanIps << ipStr;
            } else {
                otherIps << ipStr;
            }
        }
    }

    auto makeAddresses = [port](const QStringList& ips) {
        QStringList out;
        for (const auto& ip : ips) out << QString("%1:%2").arg(ip).arg(port);
        out.removeDuplicates();
        return out;
    };

    QStringList result;
    result << makeAddresses(tailscaleIps);
    result << makeAddresses(lanIps);
    result << makeAddresses(otherIps);
    result << QString("127.0.0.1:%1").arg(port);
    result.removeDuplicates();
    return result;
}

bool NetworkManager::connectToHost(const QString& address, int port) {
    m_isHost = false;
    m_socket->connectToHost(address, port);
    return m_socket->waitForConnected(3000);
}

void NetworkManager::disconnectFromHost() {
    m_socket->disconnectFromHost();
}

void NetworkManager::sendToServer(const QJsonObject& msg) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact) + "\n";
        m_socket->write(data);
    }
}

void NetworkManager::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* clientSocket = m_server->nextPendingConnection();
        m_clientSockets.append(clientSocket);
        
        int playerId = m_clientSockets.size() - 1;
        
        connect(clientSocket, &QTcpSocket::readyRead, this, &NetworkManager::onClientReadyRead);
        connect(clientSocket, &QTcpSocket::disconnected, this, &NetworkManager::onClientDisconnected);
        
        emit playerConnected(playerId);
    }
}

void NetworkManager::onClientReadyRead() {
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    int playerId = m_clientSockets.indexOf(clientSocket);
    while (clientSocket->canReadLine()) {
        QByteArray data = clientSocket->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            emit dataReceivedFromPlayer(playerId, doc.object());
        }
    }
}

void NetworkManager::onClientDisconnected() {
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    int playerId = m_clientSockets.indexOf(clientSocket);
    if (playerId != -1) {
        m_clientSockets.removeAt(playerId);
        emit playerDisconnected(playerId);
    }
    clientSocket->deleteLater();
}

void NetworkManager::onSocketReadyRead() {
    while (m_socket->canReadLine()) {
        QByteArray data = m_socket->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            emit dataReceivedFromServer(doc.object());
        }
    }
}
