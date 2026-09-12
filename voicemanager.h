#ifndef VOICEMANAGER_H
#define VOICEMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QAudioSource>
#include <QAudioSink>
#include <QTimer>
#include "voiceprobe.h"
#include<QMediaDevices>
#include<QByteArray>
#include<QMap>
#include<QtEndian>
#include<QMutex>
#include<QTimer>
#include <QDtls>
#include <QSslCertificate>
#include <QSslKey>

class VoiceManager : public QObject
{
    Q_OBJECT


public:
    explicit VoiceManager(QObject* parent = nullptr);
    ~VoiceManager();

    // Public methods
    void startSession(const QString& peerIP, quint16 port, bool isserver);
    void stopSession();
    void startTalking();
    void stopTalking();
    void setMuted(bool muted);

    // Getters
    bool isConnected() const { return m_connected; }
    bool isTalking() const { return m_isTalking; }
    bool isMuted() const { return m_muted; }
    bool isPeerTalking() const { return m_peerTalking; }
    bool isAudioReady() const { return m_audioReady; }
    void resetState();

signals:
    void connected();
    void disconnected();
    void connectedChanged(bool connected);
    void talkingStarted();
    void talkingStopped();
    void talkingChanged(bool talking);
    void peerTalkingChanged(bool peerTalking);
    void mutedChanged(bool muted);
    void audioReady();
    void error(const QString& message);
    void peerSpeaking();

private slots:
    void processIncomingAudio();
    void playBufferedAudio();
    void onPeerTalkingTimeout();
    void onConnectionTimeout();
    void sendAudioChunk(const QByteArray& data);

private:
    // Core components
    QUdpSocket* m_socket;
    QAudioSource* m_audioSource;
    QAudioSink* m_audioSink;
    QIODevice* m_audioOutputDevice;
    AudioProbeDevice* m_probeDevice;

    // Timers
    QTimer* m_timeoutTimer;
    QTimer* m_playbackTimer;
    QTimer* m_peerTalkingTimer;

    // Network
    QHostAddress m_peerAddress;
    quint16 m_peerPort;

    // Audio buffers
    QByteArray m_sendBuffer;
    QByteArray m_playbackBuffer;
    QMap<quint16, QByteArray> m_jitterMap;
    QMutex m_bufferMutex;

    // State
    bool m_connected;
    bool m_isTalking;
    bool m_muted;
    bool m_peerTalking;
    bool m_audioReady;
    bool m_isBufferInitialized;
    quint16 m_nextPlaySeq;


    // Constants
    static constexpr int FRAME_SIZE = 640;        // 20ms @ 16kHz 16-bit mono
    static constexpr int JITTER_FRAMES = 3;       // 60ms jitter buffer
    static constexpr int MAX_JITTER_GAP = 50;     // Max sequence gap before reset

    // Private methods
    bool setupAudioDevices();
    void cleanupAudio();
    void resetJitterBuffer();
    bool writeAudioToDevice(const QByteArray& pcmData);

    QDtls m_dtls;
    QDtls::HandshakeState m_handshakeState;
};

#endif // VOICEMANAGER_H
