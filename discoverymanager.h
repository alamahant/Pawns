#ifndef DISCOVERYMANAGER_H
#define DISCOVERYMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QMap>
#include <QDateTime>

struct PeerInfo {
    QString ip;
    quint16 port;
    QString name;
    QDateTime lastSeen;
    bool isOnline;
};

class DiscoveryManager : public QObject
{
    Q_OBJECT

public:
    explicit DiscoveryManager(QObject* parent = nullptr);
    ~DiscoveryManager();

    void startListening();
    void stopListening();
    void discoverPeers();
    void setMyName(const QString& name);
    void setMyPort(quint16 port);

    QList<PeerInfo> getPeers() const;
    bool hasPeer(const QString& ip, quint16 port) const;

signals:
    void peerDiscovered(const PeerInfo& peer);
    void peerUpdated(const PeerInfo& peer);
    void peerRemoved(const QString& ip, quint16 port);

private slots:
    void handleIncomingData();
    void cleanupStalePeers();

private:
    QUdpSocket* m_listenSocket;
    QUdpSocket* m_broadcastSocket;
    QTimer* m_cleanupTimer;
    QMap<QString, PeerInfo> m_peers;

    QString m_myName;
    quint16 m_myPort;
    QHostAddress getBroadcastAddress() const;

    QString getLocalIP() const;
    void sendResponse(const QHostAddress& sender, quint16 port);
    void addOrUpdatePeer(const QString& ip, quint16 port, const QString& name);
};

#endif // DISCOVERYMANAGER_H
