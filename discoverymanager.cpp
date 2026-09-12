#include "discoverymanager.h"
#include <QNetworkInterface>
#include <QHostInfo>
#include <QDebug>
#include"Constants.h"

static const quint16 DISCOVERY_PORT = PawnConstants::listeningPort + 1;
static const QString DISCOVERY_MSG = "PAWNS_DISCOVERY";
static const QString RESPONSE_PREFIX = "PAWNS_RESPONSE:";
static const int PEER_TIMEOUT_SECONDS = 15;

DiscoveryManager::DiscoveryManager(QObject* parent)
    : QObject(parent)
    , m_listenSocket(new QUdpSocket(this))
    , m_broadcastSocket(new QUdpSocket(this))
    , m_cleanupTimer(new QTimer(this))
    , m_myPort(PawnConstants::listeningPort)
{
    m_myName = QHostInfo::localHostName();
    m_cleanupTimer->setInterval(5000);
   // connect(m_cleanupTimer, &QTimer::timeout, this, &DiscoveryManager::cleanupStalePeers);
}

DiscoveryManager::~DiscoveryManager()
{
    stopListening();
}



void DiscoveryManager::startListening()
{
    if (m_listenSocket->bind(QHostAddress::AnyIPv4, DISCOVERY_PORT, QUdpSocket::ShareAddress)) {
        connect(m_listenSocket, &QUdpSocket::readyRead, this, &DiscoveryManager::handleIncomingData);

        m_broadcastSocket->bind(DISCOVERY_PORT, QUdpSocket::ShareAddress);

        m_cleanupTimer->start();
    }
}

void DiscoveryManager::stopListening()
{
    m_listenSocket->close();
    m_cleanupTimer->stop();
    m_peers.clear();
}

void DiscoveryManager::setMyName(const QString& name)
{
    m_myName = name.isEmpty() ? QHostInfo::localHostName() : name;
}

void DiscoveryManager::setMyPort(quint16 port)
{
    m_myPort = port;
}

void DiscoveryManager::discoverPeers()
{
    QByteArray data = DISCOVERY_MSG.toUtf8();

    QHostAddress broadcastAddr = getBroadcastAddress();

    m_broadcastSocket->writeDatagram(data, broadcastAddr, DISCOVERY_PORT);

    m_broadcastSocket->writeDatagram(data, QHostAddress("255.255.255.255"), DISCOVERY_PORT);
}

QList<PeerInfo> DiscoveryManager::getPeers() const
{
    return m_peers.values();
}

bool DiscoveryManager::hasPeer(const QString& ip, quint16 port) const
{
    QString key = ip + ":" + QString::number(port);
    return m_peers.contains(key);
}

void DiscoveryManager::handleIncomingData()
{

    while (m_listenSocket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(m_listenSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        m_listenSocket->readDatagram(data.data(), data.size(), &sender, &senderPort);

        QString message = QString::fromUtf8(data);
        if (message == DISCOVERY_MSG) {
            sendResponse(sender, senderPort);
        } else if (message.startsWith(RESPONSE_PREFIX)) {
            QStringList parts = message.split(":");
            if (parts.size() >= 4) {
                QString ip = parts[1];
                quint16 port = parts[2].toUShort();
                QString name = parts[3];

                if (ip == getLocalIP() && port == m_myPort) {
                    continue;
                }
                addOrUpdatePeer(ip, port, name);
            }
        }
    }
}



void DiscoveryManager::sendResponse(const QHostAddress& sender, quint16 port)
{

    QString response = QString("%1%2:%3:%4")
        .arg(RESPONSE_PREFIX)
        .arg(getLocalIP())
        .arg(m_myPort)
        .arg(m_myName);


    qint64 sent = m_listenSocket->writeDatagram(response.toUtf8(), sender, port);


    if (sent == -1) {
    }
}

void DiscoveryManager::addOrUpdatePeer(const QString& ip, quint16 port, const QString& name)
{
    QString key = ip + ":" + QString::number(port);
    QDateTime now = QDateTime::currentDateTime();

    if (m_peers.contains(key)) {
        PeerInfo& peer = m_peers[key];
        peer.lastSeen = now;
        peer.isOnline = true;
        if (peer.name != name) {
            peer.name = name;
        }
        emit peerUpdated(peer);
    } else {
        PeerInfo peer;
        peer.ip = ip;
        peer.port = port;
        peer.name = name;
        peer.lastSeen = now;
        peer.isOnline = true;
        m_peers[key] = peer;
        emit peerDiscovered(peer);
    }
}

void DiscoveryManager::cleanupStalePeers()
{
    QDateTime now = QDateTime::currentDateTime();
    QList<QString> toRemove;

    for (auto it = m_peers.begin(); it != m_peers.end(); ++it) {
        if (it.value().lastSeen.secsTo(now) > PEER_TIMEOUT_SECONDS) {
            if (it.value().isOnline) {
                it.value().isOnline = false;
                emit peerRemoved(it.value().ip, it.value().port);
            }
            toRemove.append(it.key());
        }
    }

    for (const QString& key : toRemove) {
        m_peers.remove(key);
    }
}

QString DiscoveryManager::getLocalIP() const
{
    for (const QHostAddress& addr : QNetworkInterface::allAddresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
            addr != QHostAddress::LocalHost &&
            addr != QHostAddress::LocalHostIPv6) {
            return addr.toString();
        }
    }
    return "127.0.0.1";
}

QHostAddress DiscoveryManager::getBroadcastAddress() const
{
    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
        if (!iface.isValid() || !(iface.flags() & QNetworkInterface::IsUp)) {
            continue;
        }

        for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
            QHostAddress ip = entry.ip();
            QHostAddress netmask = entry.netmask();
            QHostAddress broadcast = entry.broadcast();

            if (ip.protocol() == QAbstractSocket::IPv4Protocol &&
                ip != QHostAddress::LocalHost &&
                ip != QHostAddress::LocalHostIPv6 &&
                !broadcast.isNull()) {

                return broadcast;
            }
        }
    }

    return QHostAddress("255.255.255.255");
}
