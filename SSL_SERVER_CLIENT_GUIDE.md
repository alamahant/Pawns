# SSL Server/Client Class Creation Guide

## Core Principle
**Always set certificates BEFORE the SSL handshake starts**, not after the connection is established.

## Server-Side (QSslServer)

### ❌ DON'T DO THIS
```cpp
// BAD: Trying to set certs in onNewConnection()
void onNewConnection() {
    QSslSocket* socket = m_server->nextPendingConnection();
    socket->setLocalCertificate(certPath);  // ← Too late! Handshake already started
    socket->startServerEncryption();
}

✅ DO THIS
C++

// GOOD: Subclass QSslServer and override incomingConnection()
class CustomSslServer : public QSslServer {
protected:
    void incomingConnection(qintptr socketDescriptor) override {
        QSslSocket* sslSocket = new QSslSocket(this);
        sslSocket->setSocketDescriptor(socketDescriptor);
        
        // Set cert/key IMMEDIATELY, BEFORE adding to pending
        sslSocket->setLocalCertificate(m_certPath);
        sslSocket->setPrivateKey(m_keyPath);
        sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
        
        connect(sslSocket, &QSslSocket::encrypted, ...);
        addPendingConnection(sslSocket);  // ← Add AFTER setup
    }
};

Key Rules
1. Use String Paths, Not Objects
C++

// ✅ GOOD - Works reliably
sslSocket->setLocalCertificate(m_certPath);  // QString path
sslSocket->setPrivateKey(m_keyPath);         // QString path

// ❌ AVOID - Can fail silently
QSslCertificate cert(certData, QSsl::Pem);
sslSocket->setLocalCertificate(cert);        // Object

2. Order Matters
C++

// Correct order:
sslSocket->setLocalCertificate(certPath);
sslSocket->setPrivateKey(keyPath);
sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
sslSocket->startServerEncryption();  // ← LAST

3. Connect Signals Before Handshake
C++

connect(sslSocket, &QSslSocket::encrypted, this, &MyClass::onEncrypted);
connect(sslSocket, &QSslSocket::sslErrors, this, [](const QList<QSslError>& errors) {
    for (const auto& e : errors) qDebug() << e.errorString();
});
sslSocket->startServerEncryption();  // Signals already connected

4. Handle Self-Signed Certs
C++

// For self-signed certificates, ignore SSL errors:
sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
ssl->ignoreSslErrors();  // Client side

// Hostname mismatch warning is normal with self-signed certs
// Can suppress with:
if (error.error() != QSslError::HostNameMismatch) {
    qDebug() << error.errorString();
}

Client-Side (QSslSocket)
✅ Standard Pattern
C++

QSslSocket* ssl = new QSslSocket(this);
ssl->setPeerVerifyMode(QSslSocket::VerifyNone);

connect(ssl, &QSslSocket::encrypted, this, &MyClass::onEncrypted);
connect(ssl, &QSslSocket::sslErrors, this, [ssl](const QList<QSslError>& errors) {
    ssl->ignoreSslErrors();
});

// Connect to host FIRST
connect(ssl, &QSslSocket::connected, ssl, [ssl]() {
    ssl->startClientEncryption();  // Then start encryption
});

ssl->connectToHost(address, port);

Complete Checklist

    Subclass QSslServer and override incomingConnection()
    Set certificate path (not object) with setLocalCertificate(QString)
    Set private key path (not object) with setPrivateKey(QString)
    Set setPeerVerifyMode(QSslSocket::VerifyNone) for self-signed
    Connect signals BEFORE calling startServerEncryption()
    Add socket with addPendingConnection() only after full setup
    Use ssl->ignoreSslErrors() for self-signed certificates
    Store cert/key paths as member variables, not local paths

Certificate Generation
C++

void generateCertificate(const QString& certPath, const QString& keyPath) {
    if (QFile::exists(certPath) && QFile::exists(keyPath)) return;
    
    QProcess process;
    QStringList args;
    args << "req" << "-x509"
         << "-newkey" << "rsa:2048"
         << "-keyout" << keyPath
         << "-out" << certPath
         << "-days" << "365"
         << "-nodes"
         << "-subj" << "/CN=MyApp"
         << "-batch";
    
    process.start("openssl", args);
    process.waitForFinished(-1);
}

Real Example: NetworkManager

See networkmanager.h and networkmanager.cpp for a complete working implementation using:

    CustomSslServer subclass
    Proper incomingConnection() override
    String-based cert/key paths
    Self-signed certificate handling

TL;DR: Subclass QSslServer, set certs in incomingConnection() before addPendingConnection(), use string paths, ignore SSL errors for self-signed certs.