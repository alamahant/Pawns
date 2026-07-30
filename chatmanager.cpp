#include "chatmanager.h"
#include "networkmanager.h"
#include <QDebug>

ChatManager::ChatManager(NetworkManager* nm, QObject* parent)
    : QObject(parent)
    , networkManager(nm)
{

}

void ChatManager::onMessageReceived(const QJsonObject& message)
{
    QString type = message["type"].toString();
    if (type == "keepalive") {
            return;
    } else if (type == "connect_request") {
        handleConnectionRequest(message);
    } else if (type == "connect_response") {
        handleConnectionResponse(message);
    } else if (type == "chat") {
        handleChatMessage(message);
    } else if (type == "game_request") {
        handleGameRequest(message);
    } else if (type == "game_response") {
        handleGameResponse(message);
    } else if (type == "color_choice") {
        handleColorChoice(message);
    } else if (type == "game_start") {
        handleGameStart(message);
    } else if (type == "game_stop") {
        handleStopGame(message);
    }  else if (type == "peer_disconnect") {
        handlePeerDisconnect(message);

    } else if (type == "move") {
        handleMove(message);
    } else if (type == "fen") {
        handleFEN(message);
    }
}

void ChatManager::sendConnectionRequest()
{
    QJsonObject msg;
    msg["type"] = "connect_request";
    msg["sender"] = myName;
    networkManager->sendMessage(msg);
}

void ChatManager::sendChatMessage(const QString& message)
{
    QJsonObject msg;
    msg["type"] = "chat";
    msg["sender"] = myName;
    msg["message"] = message;
    networkManager->sendMessage(msg);
}

void ChatManager::sendGameRequest()
{
    QJsonObject msg;
    msg["type"] = "game_request";
    msg["sender"] = myName;
    networkManager->sendMessage(msg);
}

void ChatManager::sendColorChoice(bool chooseWhite)
{
    QJsonObject msg;
    msg["type"] = "color_choice";
    msg["sender"] = myName;
    msg["color"] = chooseWhite ? "white" : "black";
    networkManager->sendMessage(msg);
}

void ChatManager::sendMove(const QString& from, const QString& to)
{
    QJsonObject msg;
    msg["type"] = "move";
    msg["from"] = from;
    msg["to"] = to;
    networkManager->sendMessage(msg);
}

void ChatManager::sendFEN(const QString& fen)
{
    QJsonObject msg;
    msg["type"] = "fen";
    msg["fen"] = fen;
    networkManager->sendMessage(msg);
}

void ChatManager::handleConnectionRequest(const QJsonObject& message)
{
    QString sender = message["sender"].toString();
    peerName = sender;
    emit connectionRequestReceived(sender);
}

void ChatManager::handleConnectionResponse(const QJsonObject& message)
{
    bool accepted = message["accepted"].toBool();
    if (accepted) {
        emit connectionAccepted();
    } else {
        emit connectionRejected();
    }
}

void ChatManager::handleChatMessage(const QJsonObject& message)
{
    QString sender = message["sender"].toString();
    QString text = message["message"].toString();
    emit chatReceived(sender, text);
}

void ChatManager::handleGameRequest(const QJsonObject& message)
{
    QString sender = message["sender"].toString();
    peerName = sender;
    emit gameRequestReceived(sender);
}

void ChatManager::handleGameResponse(const QJsonObject& message)
{
    bool accepted = message["accepted"].toBool();
    if (accepted) {
        emit gameAccepted();
    } else {
        emit gameRejected();
    }
}

void ChatManager::handleColorChoice(const QJsonObject& message)
{
    QString color = message["color"].toString();
    emit colorChoiceReceived(color == "white");
}

void ChatManager::handleGameStart(const QJsonObject& message)
{
    QString white = message["white"].toString();
    QString black = message["black"].toString();
    emit gameStarted(white, black);
}

void ChatManager::handleMove(const QJsonObject& message)
{
    QString from = message["from"].toString();
    QString to = message["to"].toString();
    emit moveReceived(from, to);
}

void ChatManager::handleFEN(const QJsonObject& message)
{
    QString fen = message["fen"].toString();
    emit fenReceived(fen);
}


void ChatManager::sendConnectionResponse(bool accept)
{
    QJsonObject msg;
    msg["type"] = "connect_response";
    msg["accepted"] = accept;
    networkManager->sendMessage(msg);
}

void ChatManager::sendGameResponse(bool accept)
{
    QJsonObject msg;
    msg["type"] = "game_response";
    msg["accepted"] = accept;
    networkManager->sendMessage(msg);
}

void ChatManager::onConnected()
{

    emit connectedToPeer();


}

void ChatManager::onDisconnected()
{
}



void ChatManager::onError(const QString& errorMessage)
{
}

void ChatManager::onConnectionRequestReceived(const QString& peerAddress)
{
    emit connectionRequestReceived(peerAddress);
}

void ChatManager::handleStopGame(const QJsonObject& message)
{
    emit gameStoppedByPeer();
}

void ChatManager::handlePeerDisconnect(const QJsonObject& message)
{
    emit peerDisconnected();
}
