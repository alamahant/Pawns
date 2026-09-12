#include "voicemanager.h"
#include <QDebug>
#include <QCryptographicHash>
#include <QCoreApplication>
#include "Constants.h"
#include <QSslConfiguration>


VoiceManager::VoiceManager(QObject* parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_audioSource(nullptr)
    , m_audioSink(nullptr)
    , m_audioOutputDevice(nullptr)
    , m_probeDevice(nullptr)
    , m_timeoutTimer(new QTimer(this))
    , m_playbackTimer(new QTimer(this))
    , m_peerTalkingTimer(new QTimer(this))
    , m_peerPort(0)
    , m_connected(false)
    , m_isTalking(false)
    , m_muted(false)
    , m_peerTalking(false)
    , m_audioReady(false)
    , m_isBufferInitialized(false)
    , m_nextPlaySeq(0)
    , m_dtls(QDtls(QSslSocket::SslClientMode))
    , m_handshakeState(QDtls::HandshakeNotStarted)
{

    m_timeoutTimer->setInterval(5000);
    m_timeoutTimer->setSingleShot(true);

    m_peerTalkingTimer->setSingleShot(true);
    m_peerTalkingTimer->setInterval(1000);
    connect(m_peerTalkingTimer, &QTimer::timeout,
            this, &VoiceManager::onPeerTalkingTimeout);

    m_playbackTimer->setInterval(20);
    connect(m_playbackTimer, &QTimer::timeout,
            this, &VoiceManager::playBufferedAudio);

    connect(m_socket, &QUdpSocket::readyRead,
            this, &VoiceManager::processIncomingAudio);

}

VoiceManager::~VoiceManager()
{
    stopSession();
}


void VoiceManager::startSession(const QString& peerIP, quint16 port, bool isServer)
{

    if (m_connected) {
        stopSession();
    }


    m_peerAddress = QHostAddress(peerIP);
    m_peerPort = port;

    // Bind socket
    if (isServer) {
        if (!m_socket->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress)) {
            emit error("Failed to bind UDP socket");
            return;
        }
    } else {
        if (!m_socket->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress)) {
            emit error("Failed to bind client socket");
            return;
        }
    }

    // Setup audio
    if (!m_audioReady) {
        if (!setupAudioDevices()) {
            emit error("Failed to initialize audio devices");
            return;
        }
    }

    // ============ SETUP DTLS ============
    // Recreate m_dtls with correct mode
    m_dtls.~QDtls();
    new (&m_dtls) QDtls(isServer ? QSslSocket::SslServerMode : QSslSocket::SslClientMode, this);
    m_dtls.setPeer(m_peerAddress, m_peerPort);

    // Load certificates
    QString certPath = PawnConstants::APPDIR + "/pawns.crt";
    QString keyPath = PawnConstants::APPDIR + "/pawns.key";
    QFile certFile(certPath);
    QFile keyFile(keyPath);

    if (!certFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) {
        emit error("Failed to load certificates");
        return;
    }

    QByteArray certData = certFile.readAll();
    QByteArray keyData = keyFile.readAll();
    certFile.close();
    keyFile.close();

    QSslCertificate cert(certData, QSsl::Pem);
    QSslKey key(keyData, QSsl::Rsa, QSsl::Pem);

    // Build configuration
    QSslConfiguration config = QSslConfiguration::defaultDtlsConfiguration();
    config.setLocalCertificate(cert);
    config.setPrivateKey(key);
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setProtocol(QSsl::DtlsV1_2);
    config.setDtlsCookieVerificationEnabled(false);
    m_dtls.setDtlsConfiguration(config);

    // Reconnect signals after recreating
    disconnect(&m_dtls, nullptr, this, nullptr);


    // Client: Start handshake immediately
    if (!isServer) {
        if (!m_dtls.doHandshake(m_socket)) {
            emit error("Failed to start handshake: " + m_dtls.dtlsErrorString());
            return;
        }
    } else {
    }

    m_connected = false;
    m_handshakeState = QDtls::HandshakeInProgress;
}

void VoiceManager::stopSession()
{

    if (m_isTalking) {
        stopTalking();
    }

    cleanupAudio();

    if (m_dtls.isConnectionEncrypted()) {
        m_dtls.shutdown(m_socket);
    }

    if (m_socket->isOpen()) {
        m_socket->close();
    }

    m_timeoutTimer->stop();
    m_playbackTimer->stop();
    m_peerTalkingTimer->stop();

    m_sendBuffer.clear();
    m_playbackBuffer.clear();
    resetJitterBuffer();

    m_connected = false;
    m_audioReady = false;
    m_peerTalking = false;
    m_handshakeState = QDtls::HandshakeNotStarted;

    emit disconnected();
    emit connectedChanged(false);
}


void VoiceManager::startTalking()
{

    if (!m_connected) {
        emit error("Not connected to peer");
        return;
    }

    if (m_muted) {
        emit error("Microphone is muted");
        return;
    }

    if (!m_audioReady) {
        emit error("Audio system not ready");
        return;
    }

    if (m_isTalking) {
        return;
    }

    m_probeDevice = new AudioProbeDevice(this);
    connect(m_probeDevice, &AudioProbeDevice::audioChunkReady,
            this, &VoiceManager::sendAudioChunk);

    if (!m_probeDevice->open(QIODevice::WriteOnly)) {
        emit error("Failed to open audio capture device");
        delete m_probeDevice;
        m_probeDevice = nullptr;
        return;
    }

    m_audioSource->start(m_probeDevice);
    m_isTalking = true;

    emit talkingStarted();
    emit talkingChanged(true);
}

void VoiceManager::stopTalking()
{
    if (!m_isTalking) {
        return;
    }

    if (m_audioSource) {
        m_audioSource->stop();
    }

    if (m_probeDevice) {
        m_probeDevice->close();
        m_probeDevice->deleteLater();
        m_probeDevice = nullptr;
    }

    m_isTalking = false;
    m_sendBuffer.clear();

    emit talkingStopped();
    emit talkingChanged(false);
}

void VoiceManager::setMuted(bool muted)
{
    if (m_muted == muted) {
        return;
    }

    m_muted = muted;

    if (m_muted && m_isTalking) {
        stopTalking();
    }

    emit mutedChanged(muted);
}


bool VoiceManager::setupAudioDevices()
{

    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();


    if (!inputDevice.isFormatSupported(format)) {
        emit error("Audio input format not supported");
        return false;
    }

    if (!outputDevice.isFormatSupported(format)) {
        emit error("Audio output format not supported");
        return false;
    }

    m_audioSink = new QAudioSink(outputDevice, format, this);
    if (!m_audioSink) {
        return false;
    }

    m_audioSink->setVolume(0.30);
    m_audioOutputDevice = m_audioSink->start();

    if (!m_audioOutputDevice) {
        delete m_audioSink;
        m_audioSink = nullptr;
        return false;
    }

    m_audioSource = new QAudioSource(inputDevice, format, this);
    if (!m_audioSource) {
        delete m_audioSink;
        m_audioSink = nullptr;
        return false;
    }

    m_audioSource->setVolume(0.30);

    m_audioReady = true;

    emit audioReady();
    return true;
}

void VoiceManager::cleanupAudio()
{

    if (m_audioSource) {
        if (m_audioSource->state() != QAudio::StoppedState) {
            m_audioSource->stop();
        }
        delete m_audioSource;
        m_audioSource = nullptr;
    }

    if (m_audioSink) {
        if (m_audioSink->state() != QAudio::StoppedState) {
            m_audioSink->stop();
        }
        delete m_audioSink;
        m_audioSink = nullptr;
    }

    m_audioOutputDevice = nullptr;

    if (m_probeDevice) {
        m_probeDevice->close();
        m_probeDevice->deleteLater();
        m_probeDevice = nullptr;
    }

    m_audioReady = false;
}

void VoiceManager::sendAudioChunk(const QByteArray& data)
{
    if (!m_connected || !m_isTalking || data.isEmpty() ||
        m_dtls.handshakeState() != QDtls::HandshakeComplete) {
        return;
    }

    m_sendBuffer.append(data);

    while (m_sendBuffer.size() >= FRAME_SIZE) {
        QByteArray pcmFrame = m_sendBuffer.left(FRAME_SIZE);
        m_sendBuffer.remove(0, FRAME_SIZE);

        static quint16 seq = 0;
        quint16 netSeq = qToBigEndian(seq);

        QByteArray packet;
        packet.append(reinterpret_cast<const char*>(&netSeq), sizeof(netSeq));
        packet.append(pcmFrame);

        qint64 sent = m_dtls.writeDatagramEncrypted(m_socket, packet);
        if (sent == -1) {
        }
        seq++;
    }
}

void VoiceManager::processIncomingAudio()
{


    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;
        m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // ============ HANDSHAKE ============
        if (m_dtls.handshakeState() != QDtls::HandshakeComplete) {
            // ============ ALWAYS update peer from the sender ============
            m_peerAddress = sender;
            m_peerPort = senderPort;
            m_dtls.setPeer(m_peerAddress, m_peerPort);

            if (!m_dtls.doHandshake(m_socket, datagram)) {
                emit error("Handshake failed: " + m_dtls.dtlsErrorString());
                stopSession();
                return;
            }

            if (m_dtls.handshakeState() == QDtls::HandshakeComplete) {
                m_connected = true;
                m_handshakeState = QDtls::HandshakeComplete;
                m_playbackTimer->start();
                emit connected();
                emit connectedChanged(true);
            }
            continue;
        }
        // ============ AUDIO ============
        //if (sender != m_peerAddress) {
        if (sender != m_peerAddress || senderPort != m_peerPort) {
            continue;
        }

        QByteArray decrypted = m_dtls.decryptDatagram(m_socket, datagram);
        if (decrypted.isEmpty() || decrypted.size() < 3) {
            continue;
        }

        quint16 seq;
        memcpy(&seq, decrypted.constData(), sizeof(seq));
        seq = qFromBigEndian(seq);

        QByteArray pcmData = decrypted.mid(2);
        if (pcmData.isEmpty()) {
            continue;
        }

        if (pcmData.size() % 2 != 0) {
            pcmData.chop(1);
        }
        if (pcmData.isEmpty()) {
            continue;
        }

        {
            QMutexLocker locker(&m_bufferMutex);
            m_jitterMap.insert(seq, pcmData);
            if (!m_isBufferInitialized) {
                m_nextPlaySeq = seq;
                m_isBufferInitialized = true;
            }
        }

        if (!m_peerTalking) {
            m_peerTalking = true;
            emit peerTalkingChanged(true);
            emit peerSpeaking();
        }
        m_peerTalkingTimer->start();
    }
}

void VoiceManager::playBufferedAudio()
{
    if (!m_audioSink || !m_audioOutputDevice) {
        return;
    }

    if (m_audioSink->state() == QAudio::StoppedState) {
        m_audioOutputDevice = m_audioSink->start();
        if (!m_audioOutputDevice) {
            return;
        }
    }

    QMutexLocker locker(&m_bufferMutex);

    if (m_jitterMap.isEmpty()) {
        if (m_isBufferInitialized) {
            m_isBufferInitialized = false;
        }
        return;
    }

    if (!m_isBufferInitialized || m_jitterMap.size() < JITTER_FRAMES) {
        return;
    }

    QByteArray pcmToPlay;

    if (m_jitterMap.contains(m_nextPlaySeq)) {
        pcmToPlay = m_jitterMap.take(m_nextPlaySeq);
        m_nextPlaySeq++;
    } else {
        quint16 lowestSeq = m_jitterMap.firstKey();
        quint16 gap = static_cast<quint16>(lowestSeq - m_nextPlaySeq);

        if (gap > MAX_JITTER_GAP) {
            resetJitterBuffer();
            return;
        }

        pcmToPlay = m_jitterMap.take(lowestSeq);
        m_nextPlaySeq = lowestSeq + 1;
    }

    if (!pcmToPlay.isEmpty() && m_audioOutputDevice) {
        qint64 written = m_audioOutputDevice->write(pcmToPlay);
        if (written == -1) {
        }
    }
}

void VoiceManager::resetJitterBuffer()
{
    m_jitterMap.clear();
    m_isBufferInitialized = false;
    m_nextPlaySeq = 0;
}



void VoiceManager::onConnectionTimeout()
{
    if (!m_connected) {
        emit error("Voice connection timeout");
        stopSession();
    }
}

void VoiceManager::onPeerTalkingTimeout()
{
    if (m_peerTalking) {
        m_peerTalking = false;
        emit peerTalkingChanged(false);
    }
}

bool VoiceManager::writeAudioToDevice(const QByteArray& pcmData)
{
    if (!m_audioOutputDevice || pcmData.isEmpty()) {
        return false;
    }

    qint64 written = m_audioOutputDevice->write(pcmData);
    if (written != pcmData.size()) {
        return false;
    }

    return true;
}

void VoiceManager::resetState()
{
    // Reset DTLS

    m_handshakeState = QDtls::HandshakeNotStarted;

    // Reset connection flags
    m_connected = false;
    m_isTalking = false;
    m_peerTalking = false;

    // Reset buffers
    m_jitterMap.clear();
    m_isBufferInitialized = false;
    m_nextPlaySeq = 0;
    m_sendBuffer.clear();
    m_playbackBuffer.clear();

    // Reset peer info
    m_peerAddress = QHostAddress();
    m_peerPort = 0;

    // Reset audio devices
    cleanupAudio();

    // Emit signals
    //emit connectedChanged(false);
    //emit talkingChanged(false);
    //emit peerTalkingChanged(false);
}
