#include "Constants.h"
#include<QStandardPaths>
#include<QApplication>

namespace PawnConstants {
    int SQUARE_SIZE = 65; // 60
    QColor LIGHT_SQUARE{240, 240, 240};
    QColor DARK_SQUARE{90, 90, 90};
    const QString APPDIR = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Pawns"; // + QApplication::applicationName();
    bool isRemotePlay = false;
    bool colorNegotiated = false;
    bool iAMWhite = false;;
    bool remotePlayStarted = false;
    bool startDialogPlayer1IsWhite = true;
    bool startDialogPlayer1IsMachine = false;

    bool startDialogPlayer2IsMachine = false;
    bool isE2EPlay = false;
    bool isDarkTheme = false;
    QString pieceSetDirectory = "" ;
    bool useDefaultPieces = true;
    const QString saveDirPath = APPDIR + "/saved_games";
    bool isH2HPlay = false;
    bool showBoardMarkings = true;
    bool startDialogClocksEnabled = false;

    //const QSize startingSize = QSize(860, 860);
#ifdef FLATPAK_BUILD
    const QSize startingSize = QSize(910, 860);
#else
    const QSize startingSize = QSize(860, 860);
#endif
    //const QSize oneDockOpen = QSize(1000, 860);
#ifdef FLATPAK_BUILD
    const QSize oneDockOpen = QSize(1050, 860);
#else
    const QSize oneDockOpen = QSize(1000, 860);
#endif
    //const QSize bothDocksOpen = QSize(1300, 860);

#ifdef FLATPAK_BUILD
    const QSize bothDocksOpen = QSize(1350, 860);
#else
    const QSize bothDocksOpen = QSize(1300, 860);
#endif

    bool isEngineLoggingEnabled = false;
    const QString saveHistoryDirPath = APPDIR + "/saved_histories";
    const QString savePGNDirPath = APPDIR + "/saved_PGN";
    bool isServerListening = false;
    quint16 listeningPort = 12345;
    bool isAudioChatEnabled = true;

}
