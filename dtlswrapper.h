#ifndef DTLSWRAPPER_H
#define DTLSWRAPPER_H

#include <QObject>
#include <QHostAddress>
#include <QByteArray>

// OpenSSL forward declarations
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_st SSL;
typedef struct bio_st BIO;

class DtlsWrapper : public QObject
{
    Q_OBJECT

public:
    explicit DtlsWrapper(QObject* parent = nullptr);
    ~DtlsWrapper();

    bool initServer(const QByteArray& cert, const QByteArray& key);
    bool initClient(const QByteArray& cert, const QByteArray& key);
    
    void sendClientHello(const QHostAddress& peer, quint16 port);
    void processIncoming(const QByteArray& data);
    
    QByteArray encrypt(const QByteArray& plaintext);
    QByteArray decrypt(const QByteArray& ciphertext);
    
    bool isHandshakeComplete() const { return m_handshakeComplete; }

signals:
    void handshakeComplete();
    void handshakeFailed(const QString& error);
    void needRetransmit(const QByteArray& data);

private:
    SSL_CTX* m_ctx;
    SSL* m_ssl;
    BIO* m_bio;
    bool m_handshakeComplete;
    bool m_isServer;
    QHostAddress m_peerAddress;
    quint16 m_peerPort;
    
    bool createContext();
    void loadCertificates(const QByteArray& cert, const QByteArray& key);
    void sendPendingData();
};

#endif