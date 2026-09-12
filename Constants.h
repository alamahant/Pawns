#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QColor>
#include<QDir>
#include<QString>
#include<QSize>

namespace PawnConstants {
    extern int SQUARE_SIZE;
    extern QColor LIGHT_SQUARE;
    extern QColor DARK_SQUARE;;
    extern const QString APPDIR;
    extern bool isRemotePlay;
    extern bool colorNegotiated;
    extern bool iAMWhite;
    extern bool remotePlayStarted;
    extern bool startDialogPlayer1IsWhite;
    extern bool startDialogPlayer1IsMachine;
    extern bool startDialogPlayer2IsMachine;
    extern bool isH2HPlay;
    extern bool isE2EPlay;
    extern bool isDarkTheme;
    extern QString pieceSetDirectory;
    extern bool useDefaultPieces;
    extern const QString saveDirPath;
    extern const QString saveHistoryDirPath;
    extern const QString savePGNDirPath;

    extern bool showBoardMarkings;
    extern bool startDialogClocksEnabled;
    extern const QSize startingSize;
    extern const QSize oneDockOpen;
    extern const QSize bothDocksOpen;
    extern bool isEngineLoggingEnabled;
    extern bool isServerListening;
    extern quint16 listeningPort;
    extern bool isAudioChatEnabled;

}

#endif // CONSTANTS_H
