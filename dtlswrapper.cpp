#include "dtlswrapper.h"
#include <QDebug>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

static bool sslInit = false;

DtlsWrapper::DtlsWrapper(QObject* parent)
    : QObject(parent)
    , m_ctx(nullptr)
    , m_ssl(nullptr)
    , m_bio(nullptr)
    , m_handshakeComplete(false)
    , m_isServer(false)
    , m_peerPort(0)
{
    if (!sslInit) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        sslInit = true;
    }
}

DtlsWrapper::~DtlsWrapper()
{
    if (m_ssl) SSL_free(m_ssl);
    if (m_ctx) SSL_CTX_free(m_ctx);
    if (m_bio) BIO_free(m_bio);
}

bool DtlsWrapper::createContext()
{
    const SSL_METHOD* method = m_isServer ? DTLS_server_method() : DTLS_client_method();
    m_ctx = SSL_CTX_new(method);
    if (!m_ctx) return false;
    
    SSL_CTX_set_verify(m_ctx, SSL_VERIFY_NONE, nullptr);
    return true;
}

void DtlsWrapper::loadCertificates(const QByteArray& cert, const QByteArray& key)
{
    BIO* cbio = BIO_new_mem_buf(cert.constData(), cert.size());
    if (cbio) {
        X509* x509 = PEM_read_bio_X509(cbio, nullptr, nullptr, nullptr);
        if (x509) {
            SSL_CTX_use_certificate(m_ctx, x509);
            X509_free(x509);
        }
        BIO_free(cbio);
    }
    
    BIO* kbio = BIO_new_mem_buf(key.constData(), key.size());
    if (kbio) {
        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(kbio, nullptr, nullptr, nullptr);
        if (pkey) {
            SSL_CTX_use_PrivateKey(m_ctx, pkey);
            EVP_PKEY_free(pkey);
        }
        BIO_free(kbio);
    }
}

bool DtlsWrapper::initServer(const QByteArray& cert, const QByteArray& key)
{
    m_isServer = true;
    if (!createContext()) return false;
    loadCertificates(cert, key);
    
    m_ssl = SSL_new(m_ctx);
    if (!m_ssl) return false;
    
    SSL_set_accept_state(m_ssl);
    m_bio = BIO_new(BIO_s_mem());
    SSL_set_bio(m_ssl, m_bio, m_bio);
    
    return true;
}

bool DtlsWrapper::initClient(const QByteArray& cert, const QByteArray& key)
{
    m_isServer = false;
    if (!createContext()) return false;
    loadCertificates(cert, key);
    
    m_ssl = SSL_new(m_ctx);
    if (!m_ssl) return false;
    
    SSL_set_connect_state(m_ssl);
    m_bio = BIO_new(BIO_s_mem());
    SSL_set_bio(m_ssl, m_bio, m_bio);
    
    return true;
}

/*
void DtlsWrapper::sendClientHello(const QHostAddress& peer, quint16 port)
{
    if (!m_ssl || m_isServer) {
        return;
    }
    
    m_peerAddress = peer;
    m_peerPort = port;
    
    int ret = SSL_do_handshake(m_ssl);
    if (ret == 1) {
        m_handshakeComplete = true;
        emit handshakeComplete();
    } else {
        sendPendingData();
    }
}
*/


void DtlsWrapper::sendClientHello(const QHostAddress& peer, quint16 port)
{

    if (!m_ssl || m_isServer) {
        return;
    }

    m_peerAddress = peer;
    m_peerPort = port;

    int ret = SSL_do_handshake(m_ssl);

    if (ret == 1) {
        m_handshakeComplete = true;
        emit handshakeComplete();
    } else {
        sendPendingData();
    }
}

void DtlsWrapper::processIncoming(const QByteArray& data)
{
    if (!m_ssl) return;
    
    BIO_write(m_bio, data.constData(), data.size());
    
    if (!m_handshakeComplete) {
        int ret = SSL_do_handshake(m_ssl);
        if (ret == 1) {
            m_handshakeComplete = true;
            emit handshakeComplete();
        } else {
            sendPendingData();
        }
    }
}

/*
void DtlsWrapper::sendPendingData()
{
    int pending = BIO_pending(m_bio);
    if (pending > 0) {
        QByteArray data(pending, 0);
        int read = BIO_read(m_bio, data.data(), pending);
        if (read > 0) {
            data.resize(read);
            emit needRetransmit(data);
        }
    }
}
*/

void DtlsWrapper::sendPendingData()
{
    int pending = BIO_pending(m_bio);

    if (pending > 0) {
        QByteArray data(pending, 0);
        int read = BIO_read(m_bio, data.data(), pending);
        if (read > 0) {
            data.resize(read);
            emit needRetransmit(data);
        }
    } else {
    }
}

QByteArray DtlsWrapper::encrypt(const QByteArray& plaintext)
{
    if (!m_ssl || !m_handshakeComplete || plaintext.isEmpty()) {
        return QByteArray();
    }
    
    SSL_write(m_ssl, plaintext.constData(), plaintext.size());
    
    int pending = BIO_pending(m_bio);
    if (pending <= 0) return QByteArray();
    
    QByteArray result(pending, 0);
    int read = BIO_read(m_bio, result.data(), pending);
    if (read <= 0) return QByteArray();
    
    result.resize(read);
    return result;
}

QByteArray DtlsWrapper::decrypt(const QByteArray& ciphertext)
{
    if (!m_ssl || !m_handshakeComplete || ciphertext.isEmpty()) {
        return QByteArray();
    }
    
    BIO_write(m_bio, ciphertext.constData(), ciphertext.size());
    
    char buffer[65536];
    int bytesRead = SSL_read(m_ssl, buffer, sizeof(buffer));
    
    if (bytesRead > 0) {
        return QByteArray(buffer, bytesRead);
    }
    
    return QByteArray();
}
