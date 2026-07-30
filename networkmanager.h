// networkmanager.h
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QSslServer>
#include <QSslSocket>
#include <QJsonObject>
#include <QTimer>

class CustomSslServer : public QSslServer
{
    Q_OBJECT
public:
    explicit CustomSslServer(QObject* parent = nullptr) : QSslServer(parent), m_manager(nullptr) {}
    void setNetworkManager(class NetworkManager* manager) { m_manager = manager; }

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    class NetworkManager* m_manager;
};

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    void startListening(quint16 port);
    void stopListening();
    void connectToPeer(const QString& address, quint16 port);
    void disconnectFromPeer();
    void sendMessage(const QJsonObject& message);

    bool isConnected() const;
    bool isHosting() const;
    QString getPeerAddress() const;

    void setupServerSocket(QSslSocket* sslSocket);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject& message);
    void connectionRequestReceived(const QString& peerAddress);
    void error(const QString& errorMsg);
    void connectionRejectedFromThirdParty(const QString& peerIP, quint16 peerPort);

private slots:
    void onServerEncrypted();
    void onClientEncrypted();
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void sendKeepalive();
private:
    void generateCertificate();
    QString getCertPath() const;
    QString getKeyPath() const;

    CustomSslServer* m_server;
    QSslSocket* m_clientSocket;
    QSslSocket* m_serverSocket;
    QByteArray m_buffer;
    bool m_isHosting;
    bool m_isConnected;
    QString m_certPath;
    QString m_keyPath;
    QTimer* m_keepaliveTimer;
};

#endif // NETWORKMANAGER_H
