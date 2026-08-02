#ifndef CHATMANAGER_H
#define CHATMANAGER_H

#include <QObject>
#include <QJsonObject>

class NetworkManager;

class ChatManager : public QObject
{
    Q_OBJECT

public:
    explicit ChatManager(NetworkManager* networkManager, QObject* parent = nullptr);

    void sendChatMessage(const QString& message);
    void sendConnectionRequest();
    void sendGameRequest();
    void sendColorChoice(bool chooseWhite);
    void sendMove(const QString& from, const QString& to);
    void sendFEN(const QString& fen);

signals:
    void chatReceived(const QString& sender, const QString& message);
    void connectionRequestReceived(const QString& sender);
    void connectionAccepted();
    void connectionRejected();
    void gameRequestReceived(const QString& sender);
    void gameAccepted();
    void gameRejected();
    void colorChoiceReceived(bool isWhite);
    void gameStarted(const QString& whitePlayer, const QString& blackPlayer);
    void moveReceived(const QString& from, const QString& to);
    void fenReceived(const QString& fen);
    void connectedToPeer();
    void gameStoppedByPeer();
    void peerDisconnected();
    void peerResigned();
    void drawOfferReceived();
    void drawResponseReceived(bool accepted);
    void clockSettingsReceived(bool enabled, int minutes, int increment);
private:
    void handleConnectionRequest(const QJsonObject& message);
    void handleConnectionResponse(const QJsonObject& message);
    void handleChatMessage(const QJsonObject& message);
    void handleGameRequest(const QJsonObject& message);
    void handleGameResponse(const QJsonObject& message);
    void handleColorChoice(const QJsonObject& message);
    void handleGameStart(const QJsonObject& message);
    void handleMove(const QJsonObject& message);
    void handleFEN(const QJsonObject& message);
    void handlePeerResigned(const QJsonObject& message);

    NetworkManager* networkManager;
    QString myName;
    QString peerName;
    void handleStopGame(const QJsonObject& message);
    void handlePeerDisconnect(const QJsonObject& message);

public:
    void sendConnectionResponse(bool accept);
    void sendGameResponse(bool accept);

public slots:
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const QJsonObject& message);
    void onError(const QString& errorMessage);
    void onConnectionRequestReceived(const QString& peerAddress);
    void handleClockSettings(const QJsonObject& message);
};

#endif // CHATMANAGER_H
