#include "networkmanager.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>
#include <QJsonDocument>
#include <QProcess>
#include <QHostAddress>
#include<QMessageBox>

void CustomSslServer::incomingConnection(qintptr socketDescriptor)
{
    QSslSocket* sslSocket = new QSslSocket(this);
    sslSocket->setSocketDescriptor(socketDescriptor);

    if (m_manager) {
        m_manager->setupServerSocket(sslSocket);
    }

    addPendingConnection(sslSocket);
}

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_server(nullptr)
    , m_clientSocket(nullptr)
    , m_serverSocket(nullptr)
    , m_isHosting(false)
    , m_isConnected(false)
    , m_keepaliveTimer(nullptr)
{
    m_keepaliveTimer = new QTimer(this);
    m_keepaliveTimer->setInterval(30000);  // Send keepalive every 30 seconds
    connect(m_keepaliveTimer, &QTimer::timeout, this, &NetworkManager::sendKeepalive);

    generateCertificate();
}

NetworkManager::~NetworkManager()
{
    m_keepaliveTimer->stop();

    if(m_clientSocket){
    disconnectFromPeer();
    stopListening();
    }
}

QString NetworkManager::getCertPath() const
{
    QString appDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appDir + "/pawns.crt";
}

QString NetworkManager::getKeyPath() const
{
    QString appDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appDir + "/pawns.key";
}



void NetworkManager::generateCertificate()
{
    QString appDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDir);

    QString certPath = getCertPath();
    QString keyPath = getKeyPath();

    if (QFile::exists(certPath) && QFile::exists(keyPath)) {
        m_certPath = certPath;
        m_keyPath = keyPath;
        return;
    }


    QProcess process;
    QStringList args;
    args << "req" << "-x509"
         << "-newkey" << "rsa:2048"
         << "-keyout" << keyPath
         << "-out" << certPath
         << "-days" << "365"
         << "-nodes"
         << "-subj" << "/CN=Pawns"
         << "-batch";

    process.start("openssl", args);
    bool finished = process.waitForFinished(-1);

    if (!finished || process.exitCode() != 0) {
        return;
    }

    if (QFile::exists(certPath) && QFile::exists(keyPath)) {
        m_certPath = certPath;
        m_keyPath = keyPath;
    } else {
    }
}

void NetworkManager::startListening(quint16 port)
{

    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }

    CustomSslServer* sslServer = new CustomSslServer(this);
    sslServer->setNetworkManager(this);

    if (!sslServer->listen(QHostAddress::AnyIPv4, port)) {
        emit error("Failed to listen on port: " + QString::number(port));
        sslServer->deleteLater();
        return;
    }

    m_server = sslServer;
    m_isHosting = true;
}

void NetworkManager::stopListening()
{
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
        m_isHosting = false;
    }
}

void NetworkManager::setupServerSocket(QSslSocket* sslSocket)
{

    sslSocket->setLocalCertificate(m_certPath);
    sslSocket->setPrivateKey(m_keyPath);
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);


    connect(sslSocket, &QSslSocket::encrypted, this, &NetworkManager::onServerEncrypted);

    connect(sslSocket, &QSslSocket::sslErrors, this, [sslSocket](const QList<QSslError>& errors) {
        for (const auto& error : errors) {
        }
        sslSocket->ignoreSslErrors();
    });

    connect(sslSocket, QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::errorOccurred),
            this, [sslSocket](QAbstractSocket::SocketError e) {
    });

    sslSocket->startServerEncryption();
}

void NetworkManager::connectToPeer(const QString& address, quint16 port)
{

    if (m_isConnected) {
        emit error("Already connected to a peer");
        return;
    }

    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    QSslSocket* ssl = new QSslSocket(this);
    ssl->setPeerVerifyMode(QSslSocket::VerifyNone);

    connect(ssl, &QSslSocket::encrypted, this, &NetworkManager::onClientEncrypted);
    connect(ssl, &QSslSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(ssl, &QSslSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(ssl, &QSslSocket::errorOccurred, this, &NetworkManager::onError);

    connect(ssl, &QSslSocket::sslErrors, this, [ssl](const QList<QSslError>& errors) {
        for (const auto& error : errors) {
        }
        ssl->ignoreSslErrors();
    });

    m_clientSocket = ssl;

    connect(ssl, &QSslSocket::connected, ssl, [ssl]() {
        ssl->startClientEncryption();
    });

    ssl->connectToHost(address, port);
}

void NetworkManager::disconnectFromPeer()
{
    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    if (m_serverSocket) {
        m_serverSocket->disconnectFromHost();
        m_serverSocket->deleteLater();
        m_serverSocket = nullptr;
    }

    stopListening();
    m_isConnected = false;
}

void NetworkManager::sendMessage(const QJsonObject& message)
{
    QSslSocket* socket = nullptr;

    if (m_clientSocket && m_clientSocket->isEncrypted() &&
        m_clientSocket->state() == QAbstractSocket::ConnectedState) {
        socket = m_clientSocket;
    } else if (m_serverSocket && m_serverSocket->isEncrypted() &&
               m_serverSocket->state() == QAbstractSocket::ConnectedState) {
        socket = m_serverSocket;
    }

    if (!socket) {
        emit error("Not connected");
        return;
    }

    QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact);
    data.append('\n');

    qint64 written = socket->write(data);
    socket->flush();

}

bool NetworkManager::isConnected() const
{
    return m_isConnected;
}

bool NetworkManager::isHosting() const
{
    return m_isHosting;
}

QString NetworkManager::getPeerAddress() const
{
    if (m_clientSocket && m_isConnected) {
        return m_clientSocket->peerAddress().toString();
    }
    if (m_serverSocket && m_isConnected) {
        return m_serverSocket->peerAddress().toString();
    }
    return QString();
}

void NetworkManager::onServerEncrypted()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;


    m_isConnected = true;
    m_serverSocket = socket;

    connect(socket, &QSslSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(socket, &QSslSocket::disconnected, this, &NetworkManager::onDisconnected);

    m_keepaliveTimer->start();  // Start sending keepalives
    emit connected();
    emit connectionRequestReceived(socket->peerAddress().toString());
}

void NetworkManager::onClientEncrypted()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;

    m_isConnected = true;
    m_keepaliveTimer->start();  // Start sending keepalives
    emit connected();
}

void NetworkManager::onReadyRead()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;


    m_buffer.append(socket->readAll());

    int pos = m_buffer.indexOf('\n');
    while (pos != -1) {
        QByteArray msgData = m_buffer.left(pos);
        m_buffer.remove(0, pos + 1);

        if (!msgData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(msgData);

            if (doc.isObject()) {
                QJsonObject obj = doc.object();

                if (obj["type"] == "keepalive") {
                } else {
                    emit messageReceived(obj);
                }

            } else {
            }
        }

        pos = m_buffer.indexOf('\n');
    }
}

void NetworkManager::onDisconnected()
{
    m_keepaliveTimer->stop();
    m_isConnected = false;
    emit disconnected();
}

void NetworkManager::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (socket) {
        QString errorMsg = socket->errorString();
        emit error(errorMsg);
    }
}

void NetworkManager::sendKeepalive()
{
    if (!m_isConnected) {
        m_keepaliveTimer->stop();
        return;
    }

    QJsonObject keepalive;
    keepalive["type"] = "keepalive";
    keepalive["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    sendMessage(keepalive);
}
