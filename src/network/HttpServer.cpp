#include "HttpServer.h"
#include "../utils/Logger.h"
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QColor>
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>

#ifdef HAS_QRENCODE
#include <qrencode.h>
#endif

HttpServer::HttpServer(QObject* parent) : QObject(parent), m_server(new QTcpServer(this)) {
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
}
HttpServer::~HttpServer() { stop(); }

bool HttpServer::start(int port) {
    if (m_server->isListening()) return true;
    if (!m_server->listen(QHostAddress::AnyIPv4, port)) return false;
    m_port = m_server->serverPort();
    Logger::instance().log(QString("HTTP 服务器已启动: %1").arg(serverUrl()));
    emit serverStarted(serverUrl());
    return true;
}

void HttpServer::stop() {
    m_server->close();
    m_port = 0;
    const auto sockets = m_buffers.keys();
    for (auto* socket : sockets) socket->abort();
    m_buffers.clear();
}

QString HttpServer::getLocalIP() const {
    QStringList candidates;
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto& iface : interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            iface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            for (const auto& addr : iface.addressEntries()) {
                QString ip = addr.ip().toString();
                if (ip.contains('.') && !ip.startsWith("169.254")) {
                    const QString key = iface.name().toLower();
                    const bool virtualInterface = key.startsWith("docker") || key.startsWith("veth") ||
                        key.startsWith("br-") || key.startsWith("virbr") || key.startsWith("tailscale");
                    const bool privateIP = addr.ip().isInSubnet(QHostAddress("10.0.0.0"), 8) ||
                        addr.ip().isInSubnet(QHostAddress("172.16.0.0"), 12) ||
                        addr.ip().isInSubnet(QHostAddress("192.168.0.0"), 16);
                    if (privateIP && !virtualInterface) candidates.prepend(ip);
                    else candidates.append(ip);
                }
            }
        }
    }
    if (!candidates.isEmpty()) return candidates.first();
    return QString("127.0.0.1");
}

QString HttpServer::serverUrl() const {
    return QString("http://%1:%2").arg(getLocalIP()).arg(m_port);
}

void HttpServer::onNewConnection() {
    while (auto* sock = m_server->nextPendingConnection()) {
        m_buffers.insert(sock, {});
        connect(sock, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
        connect(sock, &QTcpSocket::disconnected, this, [this, sock]() {
            m_buffers.remove(sock);
            sock->deleteLater();
        });
        QTimer::singleShot(10000, sock, [sock]() { sock->abort(); });
    }
}

void HttpServer::onReadyRead() {
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    if (!m_buffers.contains(client)) return;
    m_buffers[client].append(client->readAll());
    if (m_buffers[client].size() > 16384) { client->abort(); return; }
    if (!m_buffers[client].contains("\r\n\r\n")) return;
    const QByteArray request = m_buffers.take(client);
    QStringList lines = QString::fromLatin1(request).split("\r\n");
    if (lines.isEmpty()) return;

    // 解析请求行: GET /path HTTP/1.1
    QStringList parts = lines.first().split(' ');
    if (parts.size() < 2) { send404(client); return; }
    QString method = parts[0];
    QString path = QUrl(parts[1]).path();

    if (method != "GET") {
        sendResponse(client, "Method Not Allowed", "text/plain", 405);
        return;
    }

    if (path == "/") path = "/index.html";

    // 去掉前缀斜杠
    QString relPath = path.mid(1);
    serveFile(client, relPath);
}

void HttpServer::serveFile(QTcpSocket* client, const QString& relPath) {
    static const QMap<QString, QString> types{
        {"index.html", "text/html"}, {"style.css", "text/css"}, {"game.js", "application/javascript"}};
    if (!types.contains(relPath)) { send404(client); return; }
    QFile file(":/web/" + relPath);
    if (!file.open(QIODevice::ReadOnly)) { send404(client); return; }
    sendResponse(client, file.readAll(), types[relPath], 200);
}

void HttpServer::sendResponse(QTcpSocket* client, const QByteArray& body,
                              const QString& contentType, int statusCode) {
    const char* statusLine = (statusCode == 200) ? "HTTP/1.1 200 OK" :
                             (statusCode == 405) ? "HTTP/1.1 405 Method Not Allowed" :
                             "HTTP/1.1 404 Not Found";

    QByteArray response;
    response.append(QByteArray(statusLine) + "\r\n");
    response.append("Content-Type: " + contentType.toLatin1() + "; charset=utf-8\r\n");
    response.append("Content-Length: " + QByteArray::number(body.size()) + "\r\n");
    response.append("Connection: close\r\n");
    response.append("Cache-Control: no-store\r\n");
    response.append("X-Content-Type-Options: nosniff\r\n");
    response.append("\r\n");
    response.append(body);

    client->write(response);
    client->disconnectFromHost();
}

void HttpServer::send404(QTcpSocket* client) {
    sendResponse(client, "404 Not Found", "text/plain", 404);
}

QByteArray HttpServer::generateQRCodePNG(const QString& text, int size) {
#ifdef HAS_QRENCODE
    QRcode* qr = QRcode_encodeString(text.toUtf8().constData(), 0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!qr) return QByteArray();

    int modules = qr->width;
    int moduleSize = qMax(1, size / (modules + 8));
    int imgSize = moduleSize * (modules + 8);

    QImage image(imgSize, imgSize, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setPen(Qt::black);
    painter.setBrush(Qt::black);

    for (int y = 0; y < modules; ++y) {
        for (int x = 0; x < modules; ++x) {
            if (qr->data[y * modules + x] & 0x01) {
                painter.fillRect((x + 4) * moduleSize, (y + 4) * moduleSize, moduleSize, moduleSize, Qt::black);
            }
        }
    }
    painter.end();
    QRcode_free(qr);

    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "PNG");
    return ba;
#else
    Q_UNUSED(text) Q_UNUSED(size)
    return QByteArray();
#endif
}
