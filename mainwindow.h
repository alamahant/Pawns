#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include<QComboBox>
#include<QGraphicsView>
#include"chessboard.h"
#include<QFile>
#include<QFileDialog>
#include<QJsonObject>
#include<QJsonDocument>
#include<QDir>
#include"scenariobuilder.h"
#include<QResizeEvent>
#include<QEvent>
#include"networkmanager.h"
#include"chatmanager.h"
#include"contactdialog.h"
#include"newgamedialog.h"
#include"upnpmanagerdialog.h"
#include<QTableWidget>
#include<QProgressBar>
#include <QMediaPlayer>
#include <QAudioOutput>
#include"replaydialog.h"
#include"sounds.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    Ui::MainWindow *ui;
    QWidget* centralWidget;
    void setupConnections();
    QTextEdit* moveHistory;
    QLabel* gameStatus;
    QComboBox* difficultySelect;
    QPushButton* confirmMove;

    void setupUI();
    void setupBottomPanel();
    void createMenus();
    QGraphicsView* view = nullptr;
    ChessBoard* board = nullptr;
    void setupChessBoard();
    void setCheckStatus(const QString& status);

private:
    QLabel* checkStatusLabel;
    StockfishEngine* engine = nullptr;
    QPushButton* colorToggle;  // Add to UI elements
    QPushButton* revertButton;
    ScenarioBuilder* scenarioBuilder = nullptr;

private slots:
    void handleConfirmMove();
    void newGame();
    void setDifficulty(int level);
    void updateMoveHistory(const QString& move);
    void makeEngineMove();
    void togglePlayerColor();
    void onFirstMove();
    void saveGame();
    void loadGame();
    void revertPiecePosition();
    void dumpFen();

    //Scenario builder slots
    void onPlacePiece(const QString& piece, const QString& position);
    void onRemovePiece(const QString& position);
    void onClearBoard();
    void onStartPosition();
    void onSetPosition(const QString& fen);
    void onScenarioBuilderClosed();
    void onTurnChanged(bool isWhiteTurn);
    void onStartGameFromDialog();
    void onShowStarDialog();
    void onConnectedToPeer();
private:
    QLabel* capturedBottomLabel;
    QLabel* capturedTopLabel;

    void updateCapturedDisplay();
    QMap<QString, QPixmap> pieceIcons;

    void loadPieceIcons();
    QString buildCapturedHtml(const QStringList& pieces);

    // zoom
    qreal currentZoom = 1.0;
    const qreal zoomStep = 0.1;
    const qreal minZoom = 0.1;
    const qreal maxZoom = 5.0;

    void applyZoom();
    void debugBoardState();

    // remote play dock
private:
    void setupRemoteDock();

    // Remote dock widgets
    QDockWidget* remoteDock;
    QListWidget* contactList;
    QTextEdit* chatDisplay;
    QLineEdit* chatInput;
    QPushButton* sendBtn;
    QPushButton* listenBtn;
    QPushButton* connectBtn;
    QPushButton* startGameBtn;
    QPushButton* stopLocalGameButton;
    QPushButton* resignRemoteButton;
    QPushButton* addContactBtn;
    QPushButton* editContactBtn;
    QPushButton* deleteContactBtn;
    QLabel* connectionStatusLabel;

    QString currentContactName;

    // chat manager nm and dock slots and helpers
private slots:
    // Remote play slots
    void onListenClicked();
    void onConnect();
    void onStartRemoteGame();
    void onStopRemoteGame();
    void onAddContact();
    void onEditContact();
    void onDeleteContact();
    void onSendChatMessage();
    void onContactSelected(QListWidgetItem* item);
    void onPeerResigned();

    // ChatManager signal slots
    void onConnectionRequestReceived(const QString& sender);
    void onConnectionAccepted();
    void onConnectionRejected();
    void onGameRequestReceived(const QString& sender);
    void onGameAccepted();
    void onGameRejected();
    void onGameStarted(const QString& whitePlayer, const QString& blackPlayer);
    void onChatReceived(const QString& sender, const QString& message);
    void onMoveReceived(const QString& from, const QString& to);
    void onFENReceived(const QString& fen);
    void onGameStoppedByPeer();
    void onPeerDisconnected();
    void onPieceSetChanged(const QString& setName);
    void onBoardColorChanged(const QString& colorName);
    void onSetBackgroundColor();

private:
    // Remote play components
    NetworkManager* networkManager = nullptr;
    ChatManager* chatManager = nullptr;


    // Contact helpers
    Contact getContact(const QString& name);
    void addContact(const Contact& contact);
    void updateContact(const QString& oldName, const Contact& contact);
    void deleteContact(const QString& name);
    void refreshContactList();
    void setupNetworkConnections();
    QString getExternalIP();
    QLineEdit* ipEdit;
    QSpinBox* portSpin;
    NewGameDialog *newGameDialog;
    QAction* viewRemoteDockAct;
    void stopGameLocally(const QString& reason);
    QPushButton* upnBtn;
    UPnPManagerDialog* upnpDialog = nullptr;
    QLabel *contactNameLabel;
    QComboBox* pieceSetCombo;
    void populatePieceSets();
    QComboBox* boardColorCombo;
    QAction* showFENAction;

    // leftdock
private:
    void setupLeftDock();

    // Left dock widgets
    QLabel* timeLabel;
    QLabel* moveLabel;
    QLabel* whiteClockLabel;
    QLabel* blackClockLabel;
    QTextEdit* consoleEdit;
    QTableWidget* moveTable;
    QAction* viewLeftDockAct;
    bool isleftdockvisible = false;
    bool isremotedockvisible = false;
    QDockWidget* gameInfoDock;
    int moveCount = 0;
    QAction* showBackgroundColorAction;
    QAction* setBackgroundColorAction;

    QColor backgroundColor = QColor();
    bool showBackgroundColor = false;
    QAction* showBoardMarkingsAction;

    // Clock widgets
    QProgressBar* whiteClockBar;
    QProgressBar* blackClockBar;

    QTimer* m_clockTimer;

    // Clock data
    int m_whiteTime = 0;      // seconds remaining
    int m_blackTime = 0;
    int m_clockMinutes = 10;
    int m_clockIncrement = 0;
    bool m_clocksEnabled = false;
    QPushButton* stopRemoteGameButton;
    QPushButton* resignLocalButton;
    QPushButton* drawButton;
private slots:
    void updateClocks();
    void startClocks(int minutes, int increment);
    void stopClocks();
    void resetClocks();
    void updateClockColor(QProgressBar* bar, int timeLeft);
    QString formatTime(int seconds);
    void onResign();
    void onDrawOffer();
public slots:
    void onDrawResponseReceived(bool accepted);
    void onDrawOfferReceived();
    void onSquareSize();

    // sound player
private:
    QMediaPlayer* m_soundPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;


    QAction* playSoundAction;
    bool playSounds = true;
    // PGN
private slots:
    void savePGN();
    void copyPGN();
    void onReplayGame();
    void saveHistory();
    void openFolder();
    void createPawnsSymlink();
    void onClockSettingsReceived(bool enabled, int minutes, int increment);

private:
    QString generatePGN();
    QString convertToPGN(const QString& moveText);
    ReplayDialog* replayDialog = nullptr;
    QStringList getMoveHistoryFromTable();
    void playSound(enum SOUNDTYPE soundtype);
    StockfishEngine* createNewEngine();
    void updateButtons(bool gameIsRunning);
    QAction* highlightMoveAction;
    bool shouldHighlightMoves = true;
};
#endif // MAINWINDOW_H
