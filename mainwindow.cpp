#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include<QTimer>
#include<QBuffer>
#include<QPainter>
#include<QDockWidget>
#include<QLineEdit>
#include<QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QSettings>
#include"Constants.h"
#include"newgamedialog.h"
#include<QSettings>
#include<QHeaderView>
#include<QColorDialog>
#include<QColor>
#include<QRegularExpression>
#include<QSplitter>
#include<QApplication>
#include<QClipboard>
#include<QDesktopServices>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , engine(new StockfishEngine(this))
    , networkManager(new NetworkManager(this))
    , chatManager(new ChatManager(networkManager, this))
    , newGameDialog(new NewGameDialog(this))
    , upnpDialog(new UPnPManagerDialog(this))
    , m_soundPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , replayDialog(new ReplayDialog(this))
{
    ui->setupUi(this);

    m_soundPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.5);
    QSettings settings;

    int savedSize = settings.value("squareSize", 65).toInt();
    PawnConstants::SQUARE_SIZE = savedSize;

    PawnConstants::showBoardMarkings = settings.value("showboardmarkings", true).toBool();

    setupUI();


    createMenus();

    setupConnections();

    resize(PawnConstants::startingSize);

    setupRemoteDock();

    setupLeftDock();

    isremotedockvisible = settings.value("remoteDockVisible", false).toBool();
    remoteDock->setVisible(isremotedockvisible);
    viewRemoteDockAct->setChecked(isremotedockvisible);

    isleftdockvisible= settings.value("leftDockVisible", false).toBool();
    gameInfoDock->setVisible(isleftdockvisible);
    viewLeftDockAct->setChecked(isleftdockvisible);

    backgroundColor = settings.value("backgroundcolor").value<QColor>();
    showBackgroundColor = settings.value("showbackground", false).toBool();
    showBackgroundColorAction->setChecked(showBackgroundColor);
    if(showBackgroundColor){
        if (backgroundColor.isValid()) {
            view->setBackgroundBrush(QBrush(backgroundColor));
        }
    }

    showBoardMarkingsAction->setChecked(PawnConstants::showBoardMarkings);

    playSounds = settings.value("playsounds", true).toBool();
    playSoundAction->setChecked(playSounds);

    setDifficulty(0);

    setWindowTitle("Pawns");
    populatePieceSets();
    loadPieceIcons();

    updateCapturedDisplay();

    setupNetworkConnections();

    refreshContactList();

    connect(newGameDialog, &NewGameDialog::difficultyChanged, this, [this](int level){
        difficultySelect->setCurrentIndex(level);
    });

    connect(newGameDialog, &NewGameDialog::remoteDockVisible, this, [this](bool visible){
        if(visible){
            if(isleftdockvisible){
            resize(PawnConstants::bothDocksOpen);
            }else if(!isleftdockvisible){
                resize(PawnConstants::oneDockOpen);
            }
        }else{
            resize(PawnConstants::startingSize);
        }
        remoteDock->setVisible(visible);
        isremotedockvisible = visible;
    });
    onShowStarDialog();

    connect(engine, &StockfishEngine::engineOutput, this, [this](const QString& line){
            consoleEdit->append(line);
            consoleEdit->ensureCursorVisible();
    });


    connect(replayDialog, &ReplayDialog::loadCurrentHistoryRequested, this, [this]{
        if (!replayDialog) {
            replayDialog = new ReplayDialog(this);
        }

        QStringList currentHistory = getMoveHistoryFromTable();

        if (currentHistory.isEmpty()) {
                QMessageBox::information(this, "Replay", "No moves in current game.");
                return;
            }

        QStringList moves;

        for (const QString& line : currentHistory) {
            // Remove move numbers like "1." "2." etc.
            QString cleanLine = line;
            QRegularExpression re("^\\d+\\.\\s*");
            cleanLine.remove(re);  // ← STRIPS NUMBERS HERE!

            if (!cleanLine.isEmpty()) {
                moves.append(cleanLine);
            }
        }

        replayDialog->resetBoard();

        replayDialog->loadMoves(moves);
    });

    connect(replayDialog, &ReplayDialog::playSound, this, [this](SOUNDTYPE soundtype){
        playSound(soundtype);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    delete engine;
    delete board;
    if(upnpDialog){
        upnpDialog->closeAllPorts();

    }
}

void MainWindow::setupUI() {

    centralWidget = new QWidget(this);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    QWidget* topPanel = new QWidget(this);
    topPanel->setFixedHeight(35);
    QHBoxLayout* topLayout = new QHBoxLayout(topPanel);
    topLayout->setContentsMargins(10, 5, 10, 5);

    QLabel* topLabel = new QLabel("Captured:", this);
    capturedTopLabel = new QLabel(this);
    capturedTopLabel->setStyleSheet("font-size: 20px;");
    topLayout->addWidget(topLabel);
    topLayout->addWidget(capturedTopLabel);
    topLayout->addStretch();
    mainLayout->addWidget(topPanel);



    view = new QGraphicsView(this);

    view->installEventFilter(this);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(view);


    QWidget* whiteCapturedPanel = new QWidget(this);
    whiteCapturedPanel->setFixedHeight(35);
    QHBoxLayout* whiteCapturedLayout = new QHBoxLayout(whiteCapturedPanel);
    whiteCapturedLayout->setContentsMargins(10, 5, 10, 5);

    QLabel* whiteCapturedLabel = new QLabel("Captured:", this);
    capturedBottomLabel = new QLabel(this);
    capturedBottomLabel->setStyleSheet("font-size: 20px;");
    whiteCapturedLayout->addWidget(whiteCapturedLabel);
    whiteCapturedLayout->addWidget(capturedBottomLabel);
    whiteCapturedLayout->addStretch();
    mainLayout->addWidget(whiteCapturedPanel);

    QWidget* bottomPanel = new QWidget(this);
    bottomPanel->setFixedHeight(80);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(5, 5, 5, 5);
    bottomLayout->setSpacing(10);

    moveHistory = new QTextEdit(this);
    moveHistory->setReadOnly(true);
    moveHistory->setMaximumHeight(60);
    bottomLayout->addWidget(moveHistory);

    gameStatus = new QLabel("White to move", this);
    bottomLayout->addWidget(gameStatus);

    checkStatusLabel = new QLabel("STATUS", this);
    checkStatusLabel->setAlignment(Qt::AlignCenter);
    checkStatusLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    bottomLayout->addWidget(checkStatusLabel);

    confirmMove = new QPushButton("Confirm", this);
    bottomLayout->addWidget(confirmMove);

    revertButton = new QPushButton("Undo", this);
    bottomLayout->addWidget(revertButton);

    mainLayout->addWidget(bottomPanel);
    setCentralWidget(centralWidget);

    setupChessBoard();

    scenarioBuilder = new ScenarioBuilder(this);
    if(board){
        scenarioBuilder->setBoard(board);
    }
}

void MainWindow::setupConnections() {

    connect(revertButton, &QPushButton::clicked, this, &MainWindow::revertPiecePosition);
    connect(confirmMove, &QPushButton::clicked, this, &MainWindow::handleConfirmMove);
    connect(difficultySelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::setDifficulty);


    connect(scenarioBuilder, &ScenarioBuilder::placePieceRequested,
            this, &MainWindow::onPlacePiece);
    connect(scenarioBuilder, &ScenarioBuilder::removePieceRequested,
            this, &MainWindow::onRemovePiece);
    connect(scenarioBuilder, &ScenarioBuilder::clearBoardRequested,
            this, &MainWindow::onClearBoard);
    connect(scenarioBuilder, &ScenarioBuilder::startPositionRequested,
            this, &MainWindow::onStartPosition);
    connect(scenarioBuilder, &ScenarioBuilder::setPositionRequested,
            this, &MainWindow::onSetPosition);
    connect(scenarioBuilder, &ScenarioBuilder::closeRequested,this,  &MainWindow::onScenarioBuilderClosed);
    connect(scenarioBuilder, &ScenarioBuilder::turnChanged,
            this, &MainWindow::onTurnChanged);
    connect(scenarioBuilder, &ScenarioBuilder::fenCopyRequested, this, [this]{
        showFENAction->trigger();
    });
    connect(scenarioBuilder, &ScenarioBuilder::saveRequested, this, &MainWindow::saveGame);

}

void MainWindow::createMenus() {

    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openFolderAction = fileMenu->addAction("&Open Data Directory");
        connect(openFolderAction, &QAction::triggered, this, &MainWindow::openFolder);
        fileMenu->addSeparator();

        fileMenu->addSeparator();
        QAction *createSymlinkAction = fileMenu->addAction("Create Shortcut to Pawns Data");
        connect(createSymlinkAction, &QAction::triggered, this, &MainWindow::createPawnsSymlink);
        fileMenu->addSeparator();

        QAction *exitAction = fileMenu->addAction("E&xit");
        connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu* gameMenu = menuBar()->addMenu(tr("&Game"));
    QAction* newGameAct = new QAction(tr("&New Game"), this);
    connect(newGameAct, &QAction::triggered, this, &MainWindow::onShowStarDialog);
    gameMenu->addAction(newGameAct);

    QAction* saveGameAct = new QAction(tr("&Save Game"), this);
    connect(saveGameAct, &QAction::triggered, this, &MainWindow::saveGame);

    QAction* loadGameAct = new QAction(tr("&Load Game"), this);
    connect(loadGameAct, &QAction::triggered, this, &MainWindow::loadGame);

    gameMenu->addAction(saveGameAct);
    gameMenu->addAction(loadGameAct);

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    viewRemoteDockAct = new QAction(tr("&Show Remote Dock"), this);
    viewRemoteDockAct->setCheckable(true);
    viewRemoteDockAct->setChecked(false);
    connect(viewRemoteDockAct, &QAction::toggled, this, [this](bool toggled){
        QSettings settings;
        settings.setValue("remoteDockVisible", toggled);
        if(toggled){
            if(isleftdockvisible){
            resize(PawnConstants::bothDocksOpen);
            }else if(!isleftdockvisible){
                resize(PawnConstants::oneDockOpen);
            }
        }else{
            resize(PawnConstants::startingSize);
        }
        remoteDock->setVisible(toggled);
        isremotedockvisible = toggled;
    });
    viewMenu->addAction(viewRemoteDockAct);

    viewMenu->addSeparator();

    viewLeftDockAct = new QAction(tr("&Show Left Dock"), this);
    viewLeftDockAct->setCheckable(true);
    viewLeftDockAct->setChecked(false);
    connect(viewLeftDockAct, &QAction::toggled, this, [this](bool toggled){
        QSettings settings;
        settings.setValue("leftDockVisible", toggled);
        if(toggled){
            if(isremotedockvisible){
            resize(PawnConstants::bothDocksOpen);
            }else if(!isremotedockvisible){
                resize(PawnConstants::oneDockOpen);
            }
        }else{
            resize(PawnConstants::startingSize);
        }
        gameInfoDock->setVisible(toggled);
        isleftdockvisible = toggled;
    });
    viewMenu->addAction(viewLeftDockAct);
    viewMenu->addSeparator();

    setBackgroundColorAction = new QAction("&Set Background Color", this);
    connect(setBackgroundColorAction, &QAction::triggered, this, &MainWindow::onSetBackgroundColor);
    viewMenu->addAction(setBackgroundColorAction);

    showBackgroundColorAction = new QAction("&Show Background Color", this);
    showBackgroundColorAction->setCheckable(true);
    showBackgroundColorAction->setChecked(false);
    connect(showBackgroundColorAction, &QAction::toggled, this, [this](bool toggled){
        QSettings settings;
        settings.setValue("showbackground", toggled);
        showBackgroundColor = toggled;
        if(toggled){
            view->setBackgroundBrush(QBrush(backgroundColor));

        }else{
           if(view) view->setBackgroundBrush(QBrush());
        }
    });
    viewMenu->addAction(showBackgroundColorAction);

    viewMenu->addSeparator();
    showBoardMarkingsAction = new QAction("&Show Board Markings", this);
    showBoardMarkingsAction->setCheckable(true);

    connect(showBoardMarkingsAction, &QAction::toggled, this, [this](bool toggled){
        PawnConstants::showBoardMarkings = toggled;
        QSettings settings;
        settings.setValue("showboardmarkings",toggled);
        if(toggled){
            if (board) {
                    board->redrawMarkings();  // ← Live update
                }
        }else {
            if (board) {
                    board->clearMarkings();  // ← Live update
                }
        }
    });
    viewMenu->addAction(showBoardMarkingsAction);
    viewMenu->addSeparator();

    //
    QAction* squareSizeAction = new QAction(tr("Square Size"), this);
    connect(squareSizeAction, &QAction::triggered, this, &MainWindow::onSquareSize);
    viewMenu->addAction(squareSizeAction);
    //

    //viewMenu->addSeparator();
    QAction* zoomAction = new QAction("&Zoom (Ctrl + Mousewheel)", this);
    //viewMenu->addAction(zoomAction);

    QMenu* toolsMenu = menuBar()->addMenu(tr("&Tools"));
    showFENAction = new QAction("Show Current FEN", this);
    connect(showFENAction, &QAction::triggered, this, &MainWindow::dumpFen);
    toolsMenu->addAction(showFENAction);

    toolsMenu->addSeparator();
    QAction* builderAction = new QAction("Scenario Builder", this);
    connect(builderAction, &QAction::triggered, this, [this]{
        if(scenarioBuilder){

            PawnConstants::isE2EPlay = false;
            PawnConstants::isRemotePlay = false;
            PawnConstants::iAMWhite = false;
            PawnConstants::remotePlayStarted = false;
            PawnConstants::isH2HPlay = false;
            moveHistory->clear();
            gameStatus->setText("White to move");
            checkStatusLabel->clear();


            if (board) {
                if(newGameDialog){
                    newGameDialog->p1ColorCombo()->setCurrentIndex(0);  // ← Use setCurrentIndex
                }
                onClearBoard();

                // ===== FIX: Flip if Player1 is Black =====
                board->setBoardFlipped(!PawnConstants::startDialogPlayer1IsWhite);

                scenarioBuilder->setBoard(board);
            }
            scenarioBuilder->getWhiteTurn()->click();
            scenarioBuilder->show();
            scenarioBuilder->raise();
        }
    });
    toolsMenu->addAction(builderAction);

    toolsMenu->addSeparator();
    QAction* saveHistoryAct = new QAction(tr("&Save History"), this);
    connect(saveHistoryAct, &QAction::triggered, this, &MainWindow::saveHistory);
    toolsMenu->addAction(saveHistoryAct);


    toolsMenu->addSeparator();
    QAction* replayAct = new QAction(tr("&Replay Game"), this);
    connect(replayAct, &QAction::triggered, this, &MainWindow::onReplayGame);
    toolsMenu->addAction(replayAct);

    QMenu* settingsMenu = menuBar()->addMenu(tr("&Settings"));

    playSoundAction = new QAction("&Play Audio", this);
    playSoundAction->setCheckable(true);
    playSoundAction->setChecked(true);
    connect(playSoundAction, &QAction::toggled, this, [this](bool toggled){
        QSettings settings;
        settings.setValue("playsounds", toggled);
        playSounds = toggled;
    });
    settingsMenu->addAction(playSoundAction);

    QAction* resetSettingsAction = new QAction("&Reset Settings", this);
    connect(resetSettingsAction, &QAction::triggered, this, [this](){
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Reset Settings",
            "Are you sure you want to reset all settings?\n\n"
            "This will delete all saved preferences and restart the application.",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            QSettings settings;
            settings.clear();

            QMessageBox::information(this, "Settings Reset",
                "Settings have been reset.\n\n"
                "The application will now restart.",
                QMessageBox::Ok);

            // Restart application
            qApp->quit();
            QProcess::startDetached(qApp->applicationFilePath(), QStringList());
        }
    });
    settingsMenu->addAction(resetSettingsAction);

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));


    QWidget* cornerWidget = new QWidget(this);
    QHBoxLayout* cornerLayout = new QHBoxLayout(cornerWidget);
    cornerLayout->setContentsMargins(0, 0, 5, 0);
    cornerLayout->setSpacing(5);

    stopGameBtn = new QPushButton("Stop", this);

    //stopGameBtn->setStyleSheet("QPushButton { color: red; font-weight: bold; font-size: 14px; }");
    stopGameBtn->setIcon(QIcon(":/icons/alert-triangle.svg"));

    //stopGameBtn->setFixedWidth(40);
    stopGameBtn->setToolTip("Stop Game");
    connect(stopGameBtn, &QPushButton::clicked, this, &MainWindow::onStopGame);

    resignButton = new QPushButton("Resign", this);
    resignButton->setIcon(QIcon(":/icons/twitter.svg"));
    //resignButton->setFixedWidth(40);
    resignButton->setToolTip("Resign");
    connect(resignButton, &QPushButton::clicked, this, &MainWindow::onResignGame);

    drawButton = new QPushButton("Draw", this);
    drawButton->setIcon(QIcon(":/icons/flag.svg"));

    //drawButton->setFixedWidth(40);
    drawButton->setToolTip("Offer Draw");
    connect(drawButton, &QPushButton::clicked, this, &MainWindow::onDrawOffer);

    cornerLayout->addWidget(stopGameBtn);
    cornerLayout->addWidget(resignButton);
    cornerLayout->addWidget(drawButton);
    cornerLayout->addSpacing(5);
    QLabel* levelLabel = new QLabel("Level:", this);
    difficultySelect = new QComboBox(this);

    for (int i = 0; i <= 20; i++) {
        difficultySelect->addItem(QString::number(i), i);
    }
    difficultySelect->setCurrentIndex(0);

    pieceSetCombo = new QComboBox(this);
    pieceSetCombo->addItem("default");
    connect(pieceSetCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::onPieceSetChanged);

    colorToggle = new QPushButton("Play as White", this);
    colorToggle->setEnabled(true);
    colorToggle->setVisible(false);
    connect(colorToggle, &QPushButton::clicked, this, &MainWindow::togglePlayerColor);

    boardColorCombo = new QComboBox(this);
    boardColorCombo->addItem("Classic");
    boardColorCombo->addItem("Wood");
    boardColorCombo->addItem("Green");
    boardColorCombo->addItem("Blue");
    boardColorCombo->addItem("Gray");
    boardColorCombo->addItem("Brown");
    boardColorCombo->setCurrentText("Classic");
    connect(boardColorCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::onBoardColorChanged);



    cornerLayout->addWidget(levelLabel);
    cornerLayout->addWidget(difficultySelect);
    cornerLayout->addWidget(pieceSetCombo);
    cornerLayout->addWidget(boardColorCombo);

    cornerLayout->addWidget(colorToggle);
    menuBar()->setCornerWidget(cornerWidget);

}



void MainWindow::setupChessBoard() {

    board = new ChessBoard(this);
    engine = createNewEngine();
    board->setStockfishEngine(engine);  // Add engine connection here
    connect(board,&ChessBoard::firstMoveMade,this,&MainWindow::onFirstMove);
    connect(board, &ChessBoard::gameStatusChanged, this, &MainWindow::setCheckStatus);

    view->setScene(board);
    view->centerOn(0, 0);
}

/*
void MainWindow::newGame() {


    bool newIsWhite = PawnConstants::startDialogPlayer1IsWhite;

    if (board) {
        board->setPlayerColor(newIsWhite);
    }

    colorToggle->setText(newIsWhite ? "Play as White" : "Play as Black");
    colorToggle->setEnabled(true);
    setDifficulty(difficultySelect->currentIndex());

    if (board) {
        delete board;
        board = nullptr;
    }

    setupChessBoard();
    updateCapturedDisplay();

    moveTable->setRowCount(0);
    moveCount = 0;

    if (PawnConstants::startDialogClocksEnabled) {
        int minutes = newGameDialog->minutes();
        int increment = newGameDialog->increment();
        startClocks(minutes, increment);
    } else {
        stopClocks();
        resetClocks();
    }

    if (board) {
        board->setPlayerColor(newIsWhite);
        board->setBoardFlipped(!newIsWhite);
        colorToggle->setText(newIsWhite ? "Play as White" : "Play as Black");
        board->generateFen();  // ← Force engine update
    }
    board->setGameOver(false);
    gameStatus->setText(newIsWhite ? "White to move" : "Black to move");

    if (PawnConstants::isE2EPlay) {
        QTimer::singleShot(500, this, &MainWindow::makeEngineMove);
    }
    if(!PawnConstants::startDialogPlayer1IsMachine && PawnConstants::startDialogPlayer2IsMachine && ! PawnConstants::startDialogPlayer1IsWhite){
        makeEngineMove();
    }

}
*/


void MainWindow::newGame() {
    bool newIsWhite = PawnConstants::startDialogPlayer1IsWhite;

    if (board) {
        delete board;
        board = nullptr;
    }

    setupChessBoard();

    // ===== USE PawnConstants DIRECTLY =====
    board->setPlayerColor(PawnConstants::startDialogPlayer1IsWhite);
    board->setBoardFlipped(!PawnConstants::startDialogPlayer1IsWhite);

    setDifficulty(difficultySelect->currentIndex());
    updateCapturedDisplay();

    moveTable->setRowCount(0);
    moveCount = 0;

    if (PawnConstants::startDialogClocksEnabled) {
        int minutes = newGameDialog->minutes();
        int increment = newGameDialog->increment();
        startClocks(minutes, increment);
    } else {
        stopClocks();
        resetClocks();
    }

    board->generateFen();
    board->setGameOver(false);
    gameStatus->setText("White to move");
    if (PawnConstants::isE2EPlay) {
        QTimer::singleShot(500, this, &MainWindow::makeEngineMove);
    }
    if(!PawnConstants::startDialogPlayer1IsMachine && PawnConstants::startDialogPlayer2IsMachine && !PawnConstants::startDialogPlayer1IsWhite){
        makeEngineMove();
    }
}




void MainWindow::setDifficulty(int level) {
    if (board && board->getEngine()) {
        board->getEngine()->setDifficulty(level);
    }
}



void MainWindow::updateMoveHistory(const QString& move) {
    moveHistory->append(move);

    moveCount++;

    QString from, to;
    QRegularExpression re("moves from (\\w{2}) to (\\w{2})");
    QRegularExpressionMatch match = re.match(move);
    if (match.hasMatch()) {
        from = match.captured(1);
        to = match.captured(2);
    } else {
        from = move;
        to = "";
    }

    int row = (moveCount - 1) / 2;
    int col = (moveCount - 1) % 2;  // 0 = White, 1 = Black

    if (col == 0) {
        moveTable->insertRow(row);
        moveTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1) + ". " + from + to));
    } else {
        moveTable->setItem(row, 1, new QTableWidgetItem(from + to));
    }
}


void MainWindow::handleConfirmMove()
{
    if (board->isGameOver()) {
        gameStatus->setText("Game is over! Start a new game.");
        return;
    }

    ChessPiece* selectedPiece = board->getSelectedPiece();
    if (selectedPiece) {
        bool isWhitePiece = (selectedPiece->getColor() == ChessPiece::White);
        if (isWhitePiece != board->isWhiteTurn) {
            gameStatus->setText("Not your turn!");
            return;
        }

        QString fromPos = selectedPiece->getOriginalPosition();
        QString toPos = selectedPiece->getPosition();

        if (board->makeMove(fromPos, toPos)) {
            board->highlightMove(fromPos, toPos);
            updateMoveHistory(board->getLastFormattedMove());
            updateCapturedDisplay();

            //playMoveSound();
            playSound(SOUNDTYPE::SOUND_MOVE);

            board->hasAnyPieceBeenDragged = false;

            if (PawnConstants::isRemotePlay) {
                chatManager->sendMove(fromPos, toPos);
                gameStatus->setText("Waiting for opponent...");
            }  else if (PawnConstants::isH2HPlay) {
                gameStatus->setText(board->isWhiteTurn ? "White to move" : "Black to move");

            }   else {
                //gameStatus->setText("Black thinking...");
                // ===== ENGINE IS THE OPPOSITE OF HUMAN =====
                bool engineIsWhite = !board->playerIsWhite;
                gameStatus->setText(engineIsWhite ? "White thinking..." : "Black thinking...");
                QTimer::singleShot(500, this, &MainWindow::makeEngineMove);
            }
        }
    }
}

void MainWindow::makeEngineMove() {

    if (board->isGameOver()) {
        if (engine) {
            engine->sendCommand("stop");
            engine->stopThinking();
        }
        return;
    }

    if (PawnConstants::isRemotePlay) {
        return;
    }
    if (!engine) {
        return;
    }


    if (board->playerIsWhite) {
        gameStatus->setText("White (Engine) thinking...");
    } else {
        gameStatus->setText("Black (Engine) thinking...");
    }

    QString bestMove = engine->getBestMove(1000); // 1 second think time

    if (!bestMove.isEmpty() && bestMove != "none" && bestMove.length() >= 4) {


        QString fromPos = bestMove.mid(0, 2);
        QString toPos = bestMove.mid(2, 2);

        board->applyEngineMove(bestMove);


        debugBoardState();

        updateMoveHistory(board->getLastFormattedMove());

        playSound(SOUNDTYPE::SOUND_MOVE);

        if(!PawnConstants::isE2EPlay) board->highlightMove(fromPos, toPos);

        updateCapturedDisplay();

        if (board->playerIsWhite) {
            gameStatus->setText("White to move");
        } else {
            gameStatus->setText("Black to move");
        }

        if (PawnConstants::isE2EPlay && !board->isGameOver()) {
            if (board->isWhiteTurn) {
                gameStatus->setText("White to move");
            } else {
                gameStatus->setText("Black to move");
            }

            QTimer::singleShot(500, this, &MainWindow::makeEngineMove);
        }

    } else {
        gameStatus->setText("Engine could not find a move!");
    }
}


void MainWindow::setCheckStatus(const QString& status) {

    if (status.contains("Promotion")|| status.contains("EnPassant") || status.contains("Castling")) {
        moveHistory->append(status);
    }

    if (status.contains("Checkmate")) {
        checkStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; }");
        checkStatusLabel->setText("CHECKMATE!");

        QString winner;
        if (status.contains("Black wins")) {
            winner = "Black";
        } else if (status.contains("White wins")) {
            winner = "White";
        } else {
            winner = board->isWhiteTurn ? "Black" : "White";
        }

        playSound(SOUNDTYPE::SOUND_CHECKMATE);

        QMessageBox::information(this, "Game Over", QString("%1 wins by checkmate!").arg(winner));
    } else if (status.contains("Check")) {
        checkStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; }");
        checkStatusLabel->setText("CHECK!");



    } else {
        checkStatusLabel->clear();
        checkStatusLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    }
}



void MainWindow::togglePlayerColor() {
    bool newIsWhite;

    if (PawnConstants::isRemotePlay) {
        newIsWhite = PawnConstants::iAMWhite;
    } else {
        newIsWhite = !board->playerIsWhite;

    }

    board->setPlayerColor(newIsWhite);
    colorToggle->setText(newIsWhite ? "Play as White" : "Play as Black");
    board->setBoardFlipped(!newIsWhite);

    if (!newIsWhite && !PawnConstants::isRemotePlay) {
        gameStatus->setText("White (Engine) to move");
        makeEngineMove();
        gameStatus->setText("Black to move");
    }
    colorToggle->setDisabled(true);
    updateCapturedDisplay();
}



void MainWindow::onFirstMove()
{
    if(colorToggle->isEnabled())
        colorToggle->setDisabled((true));

}


void MainWindow::saveGame() {

    if(!QDir(PawnConstants::saveDirPath).exists()) {
        QDir().mkpath(PawnConstants::saveDirPath);
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString defaultName = PawnConstants::saveDirPath + "/game_" + timestamp + ".chess";

    QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Save Chess Game"),
                defaultName,
                tr("Chess Game (*.chess);;All Files (*)"),
                nullptr,
                QFileDialog::DontUseNativeDialog
                );

    if (fileName.isEmpty()) return;

    QJsonObject gameState;
    gameState["fen"] = board->getCurrentFen();
    gameState["playerIsWhite"] = board->playerIsWhite;

    gameState["moveHistory"] = moveHistory->toPlainText();
    gameState["isWhiteTurn"] = board->isWhiteTurn;
    gameState["gameStatus"] = gameStatus->text();
    gameState["difficulty"] = difficultySelect->currentIndex();
    gameState["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(gameState).toJson());
    }
}



void MainWindow::loadGame() {

    if(!QDir(PawnConstants::saveDirPath).exists()) {
        QDir().mkpath(PawnConstants::saveDirPath);
    }

    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Load Chess Game"),
                PawnConstants::saveDirPath,     // starting directory
                tr("Chess Game (*.chess);;All Files (*)"),
                nullptr,
                QFileDialog::DontUseNativeDialog
                );

    if (fileName.isEmpty()) return;

    moveHistory->clear();
    gameStatus->clear();
    checkStatusLabel->clear();

    if (board) {
        delete board;
        board = nullptr;
    }

    setupChessBoard();

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject gameState = QJsonDocument::fromJson(file.readAll()).object();

        board->setupPosition(gameState["fen"].toString());
        board->setPlayerColor(gameState["playerIsWhite"].toBool());
        board->setBoardFlipped(!gameState["playerIsWhite"].toBool());
        moveHistory->setText(gameState["moveHistory"].toString());
        gameStatus->setText(gameState["gameStatus"].toString());
        difficultySelect->setCurrentIndex(gameState["difficulty"].toInt());
        setDifficulty(gameState["difficulty"].toInt());

        /*
        if (!board->playerIsWhite && gameState["isWhiteTurn"].toBool()) {
            makeEngineMove();
        }
        */

        bool isEngineTurn = (board->playerIsWhite && !gameState["isWhiteTurn"].toBool()) ||  // You're White, Black's turn = engine
                            (!board->playerIsWhite && gameState["isWhiteTurn"].toBool());    // You're Black, White's turn = engine

        if (isEngineTurn) {
            QMessageBox::information(this, "Loading Game",
                "Game loaded successfully.\n\n"
                "It's the engine's turn to move.",
                QMessageBox::Ok);

            makeEngineMove();
        }
    }
}

void MainWindow::revertPiecePosition() {
    ChessPiece* selectedPiece = board->getSelectedPiece();
    if (!selectedPiece) {
        return;
    }


    QString originalPos = selectedPiece->getOriginalPosition();

    QString currentPos = selectedPiece->getPosition();

    if (originalPos == currentPos) {
        return;
    }

    int fromCol = currentPos[0].toLatin1() - 'a';
    int fromRow = '8' - currentPos[1].toLatin1();
    int toCol = originalPos[0].toLatin1() - 'a';
    int toRow = '8' - originalPos[1].toLatin1();

    board->getPieces()[toRow][toCol] = board->getPieces()[fromRow][fromCol];
    board->getPieces()[fromRow][fromCol] = nullptr;

    selectedPiece->setPos(toCol * PawnConstants::SQUARE_SIZE,
                          toRow * PawnConstants::SQUARE_SIZE);
    selectedPiece->setInitialPosition(originalPos);
    board->hasAnyPieceBeenDragged = false;   // Reset
    board->setSelectedPiece(nullptr);
    selectedPiece->setSelected(false);


    gameStatus->setText("Piece returned to original position");
}




void MainWindow::debugBoardState() {
    return;
    qWarning() << "══════════════════════════════════════════";
    qWarning() << "BOARD STATE:";
    for (int row = 0; row < 8; row++) {
        QString line = QString("%1 ").arg(8 - row);
        for (int col = 0; col < 8; col++) {
            ChessPiece* piece = board->getPiecesRef()[row][col];
            if (piece) {
                line += piece->getFenChar();
                line += " ";
            } else {
                line += ". ";
            }
        }
        qWarning() << line;
    }
    qWarning() << "  a b c d e f g h";
    qWarning() << "FEN:" << board->getCurrentFen();
    qWarning() << "══════════════════════════════════════════";
}

void MainWindow::dumpFen() {
    if (board) {
        board->dumpFen();
    }
}

void MainWindow::onPlacePiece(const QString& piece, const QString& position)
{

    if (!board) return;

    QStringList parts = piece.split(' ');
    if (parts.size() < 3) {
        return;
    }

    QString colorStr = parts[1];  // "White" or "Black"
    QString typeStr = parts[2];   // "King", "Queen", etc.


    ChessPiece::PieceColor color = (colorStr == "White") ? ChessPiece::White : ChessPiece::Black;

    ChessPiece::PieceType type;
    if (typeStr == "King") type = ChessPiece::King;
    else if (typeStr == "Queen") type = ChessPiece::Queen;
    else if (typeStr == "Rook") type = ChessPiece::Rook;
    else if (typeStr == "Bishop") type = ChessPiece::Bishop;
    else if (typeStr == "Knight") type = ChessPiece::Knight;
    else if (typeStr == "Pawn") type = ChessPiece::Pawn;
    else {
        return;
    }

    int col = position[0].toLatin1() - 'a';
    int row = 8 - position[1].digitValue();

    board->removePieceFromBoard(row, col);

    ChessPiece* newPiece = new ChessPiece(type, color);
    board->addItem(newPiece);
    board->loadPieceImage(newPiece);
    newPiece->setPos(col * PawnConstants::SQUARE_SIZE, row * PawnConstants::SQUARE_SIZE);
    newPiece->setInitialPosition(position);

    board->placePieceOnBoard(newPiece, row, col);
    board->generateFen();
}

void MainWindow::onRemovePiece(const QString& position)
{
    if (!board) return;

    int col = position[0].toLatin1() - 'a';
    int row = 8 - position[1].digitValue();

    board->removePieceFromBoard(row, col);
    board->generateFen();
}

void MainWindow::onClearBoard()
{
    if (!board) return;
    board->clearBoard();
    board->generateFen();
}

void MainWindow::onStartPosition()
{
    if (!board) return;
    board->setupPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    gameStatus->setText("White to move");
    checkStatusLabel->clear();

    board->generateFen();
}

void MainWindow::onSetPosition(const QString& fen)
{
    if (!board) return;
    board->setupPosition(fen);  // ← Use the parameter
    board->generateFen();       // ← Update engine
    gameStatus->setText(board->isWhiteTurn ? "White to move" : "Black to move");
    board->updateGameStatus();
}



void MainWindow::onScenarioBuilderClosed()
{
    if (!board) return;

    // ===== REINITIALIZE ENGINE =====
    if (engine) {
        engine->stopThinking();
        delete engine;
        engine = nullptr;
    }

    engine = createNewEngine();
    board->setStockfishEngine(engine);

    // ===== UPDATE ENGINE POSITION =====
    board->generateFen();

    // ===== SET PLAYER COLOR =====
    // Default: White at bottom (Human plays White)
    PawnConstants::startDialogPlayer1IsWhite = true;
    board->setPlayerColor(true);
    //board->setBoardFlipped(false);
    //colorToggle->setText("Play as White");

    // ===== UPDATE UI =====
    gameStatus->setText(board->isWhiteTurn ? "White to move" : "Black to move");
    checkStatusLabel->clear();
    board->updateGameStatus();
    updateCapturedDisplay();

    // ===== CLEAR MOVE HISTORY =====
    moveHistory->clear();
    moveTable->setRowCount(0);
    moveCount = 0;

    // ===== RESET GAME STATE =====
    board->setGameOver(false);

    // ===== CHECK IF ENGINE SHOULD MOVE FIRST =====
    if (board->isWhiteTurn == false) {
        // It's Black's turn, but we are White, so engine (Black) moves first

        // Set difficulty before engine moves
        setDifficulty(difficultySelect->currentIndex());

        // Make engine move (Black)
        QTimer::singleShot(500, this, &MainWindow::makeEngineMove);

    } else {
        setDifficulty(difficultySelect->currentIndex());

    }

}



void MainWindow::onTurnChanged(bool isWhiteTurn)
{
    if (!board) return;

    board->isWhiteTurn = isWhiteTurn;
    board->generateFen();

    if (isWhiteTurn) {
        gameStatus->setText("White to move");
    } else {
        gameStatus->setText("Black to move");
    }
}

void MainWindow::onStartGameFromDialog()
{
    if(PawnConstants::startDialogPlayer1IsMachine && PawnConstants::startDialogPlayer2IsMachine){
        PawnConstants::isE2EPlay = true;
    }

    if(!PawnConstants::startDialogPlayer1IsMachine && !PawnConstants::startDialogPlayer2IsMachine){
        PawnConstants::isH2HPlay = true;
    }


    newGame();
}

void MainWindow::onShowStarDialog()
{
    /*
    PawnConstants::isRemotePlay = false;
    PawnConstants::iAMWhite = false;
    PawnConstants::remotePlayStarted = false;
    PawnConstants::startDialogPlayer1IsWhite = false;
    PawnConstants::startDialogPlayer1IsMachine = false;
    PawnConstants::startDialogPlayer2IsMachine = false;
    PawnConstants::isE2EPlay = false;
    moveHistory->clear();
    gameStatus->setText("White to move");
    checkStatusLabel->clear();
*/

    PawnConstants::isE2EPlay = false;
    PawnConstants::isRemotePlay = false;
    PawnConstants::iAMWhite = false;
    PawnConstants::remotePlayStarted = false;
    PawnConstants::isH2HPlay = false;
    moveHistory->clear();
    gameStatus->setText("White to move");
    checkStatusLabel->clear();

    QTimer::singleShot(100, this, [this]() {
        newGameDialog->onColorChanged(newGameDialog->p1ColorCombo()->currentIndex());
        if (newGameDialog->exec() == QDialog::Accepted) {
            onStartGameFromDialog();

        }
    });
}


void MainWindow::loadPieceIcons()
{
    int iconSize = 30;

    QString directory;
    if (PawnConstants::useDefaultPieces) {
        directory = ":/piecesets/default/";
    } else {
        directory = PawnConstants::pieceSetDirectory;
    }

    QStringList pieceTypes = {"wk", "wq", "wr", "wb", "wn", "wp", "bk", "bq", "br", "bb", "bn", "bp"};

    for (const QString& type : pieceTypes) {
        QString path = directory + type + ".png";
        QPixmap pixmap(path);
        if (!pixmap.isNull()) {
            pieceIcons[type] = pixmap.scaled(iconSize, iconSize,
                                             Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            pieceIcons[type] = QPixmap(":/piecesets/default/" + type + ".png").scaled(iconSize, iconSize,
                                             Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }
}

QString MainWindow::buildCapturedHtml(const QStringList& pieces)
{
    if (pieces.isEmpty()) return "";

    QString html;
    for (const QString& key : pieces) {
        if (pieceIcons.contains(key)) {
            QByteArray bytes;
            QBuffer buffer(&bytes);
            pieceIcons[key].save(&buffer, "PNG");
            QString base64 = bytes.toBase64();
            html += QString("<img src='data:image/png;base64,%1' width='28' height='28'> ")
                    .arg(base64);
        }
    }
    return html;
}


void MainWindow::updateCapturedDisplay()
{
    if (!board) return;

    QStringList whiteCaptures = board->getCapturedWhiteList();  // White pieces captured by Black
    QStringList blackCaptures = board->getCapturedBlackList();  // Black pieces captured by White

    QStringList topPieces;     // Opponent's captures (top)
    QStringList bottomPieces;  // Your captures (bottom)

    if (board->playerIsWhite) {
        topPieces = whiteCaptures;    // White pieces captured by Black (engine) → TOP
        bottomPieces = blackCaptures; // Black pieces captured by White (you) → BOTTOM
    } else {
        topPieces = blackCaptures;    // Black pieces captured by White (engine) → TOP
        bottomPieces = whiteCaptures; // White pieces captured by Black (you) → BOTTOM
    }

    capturedTopLabel->clear();
    capturedBottomLabel->clear();

    QPixmap topPixmap;
    QPixmap bottomPixmap;
    int spacing = 2;

    if (!topPieces.isEmpty()) {
        int totalWidth = topPieces.size() * 30 + (topPieces.size() - 1) * spacing;
        topPixmap = QPixmap(totalWidth, 30);
        topPixmap.fill(Qt::transparent);

        QPainter painter(&topPixmap);
        int x = 0;
        for (const QString& key : topPieces) {
            if (pieceIcons.contains(key)) {
                painter.drawPixmap(x, 0, 30, 30, pieceIcons[key]);
                x += 30 + spacing;
            }
        }
        painter.end();
        capturedTopLabel->setPixmap(topPixmap);
    } else {
        capturedTopLabel->setText("(none)");
        capturedTopLabel->setStyleSheet("color: gray; font-style: italic;");
    }

    if (!bottomPieces.isEmpty()) {
        int totalWidth = bottomPieces.size() * 30 + (bottomPieces.size() - 1) * spacing;
        bottomPixmap = QPixmap(totalWidth, 30);
        bottomPixmap.fill(Qt::transparent);

        QPainter painter(&bottomPixmap);
        int x = 0;
        for (const QString& key : bottomPieces) {
            if (pieceIcons.contains(key)) {
                painter.drawPixmap(x, 0, 30, 30, pieceIcons[key]);
                x += 30 + spacing;
            }
        }
        painter.end();
        capturedBottomLabel->setPixmap(bottomPixmap);
    } else {
        capturedBottomLabel->setText("(none)");
        capturedBottomLabel->setStyleSheet("color: gray; font-style: italic;");
    }
}

/*
void MainWindow::applyZoom()
{
    if (!view) return;

    QTransform transform;
    transform.scale(currentZoom, currentZoom);
    view->setTransform(transform);
    view->centerOn(0, 0);
}
*/

void MainWindow::applyZoom()
{
    return;
    if (!view) return;

    QTransform transform;

    if (board && board->isFlipped) {
        transform.scale(-currentZoom, -currentZoom);
        view->centerOn(width() / 2, height() / 2);
    } else {
        transform.scale(currentZoom, currentZoom);
        view->centerOn(0, 0);
    }

    view->setTransform(transform);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == view && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);

        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            qreal delta = wheelEvent->angleDelta().y();
            if (delta > 0) {
                currentZoom = qMin(currentZoom + zoomStep, maxZoom);
            } else {
                currentZoom = qMax(currentZoom - zoomStep, minZoom);
            }
            applyZoom();
            return true;  // Event handled
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    if (view && board) {
        view->centerOn(0, 0);
    }
}

void MainWindow::setupRemoteDock()
{

    remoteDock = new QDockWidget("Remote Play", this);
    remoteDock->setAllowedAreas(Qt::RightDockWidgetArea);
    remoteDock->setFeatures(QDockWidget::DockWidgetClosable |
                            QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable);

    QWidget* dockWidget = new QWidget(this);
    QVBoxLayout* dockLayout = new QVBoxLayout(dockWidget);
    dockLayout->setSpacing(10);
    dockLayout->setContentsMargins(5, 5, 5, 5);

    QHBoxLayout* connectionFields = new QHBoxLayout();
    connectionFields->addWidget(new QLabel("IP/DDNS:", this));
    ipEdit = new QLineEdit(this);
    ipEdit->setPlaceholderText("127.0.0.1");
    ipEdit->setText("127.0.0.1");
    connectionFields->addWidget(ipEdit);

    connectionFields->addWidget(new QLabel("Port:", this));

    portSpin = new QSpinBox(this);
    portSpin->setRange(1, 65535);
    portSpin->setValue(12345);
    connectionFields->addWidget(portSpin);

    /*
    QGroupBox* connectionGroup = new QGroupBox("Connection", this);
    QVBoxLayout* connectionLayout = new QVBoxLayout(connectionGroup);

    connectionStatusLabel = new QLabel("● Disconnected", this);
    connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    contactNameLabel = new QLabel(this);
    */
    QGroupBox* connectionGroup = new QGroupBox("Connection", this);
    QVBoxLayout* connectionLayout = new QVBoxLayout(connectionGroup);

    QHBoxLayout* statusLayout = new QHBoxLayout();
    connectionStatusLabel = new QLabel("● Disconnected", this);
    connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    contactNameLabel = new QLabel(this);
    contactNameLabel->setStyleSheet("color: gray; font-weight: bold; font-size: 14px;");
    contactNameLabel->setText("");

    statusLayout->addWidget(connectionStatusLabel);
    statusLayout->addWidget(contactNameLabel);
    statusLayout->addStretch();  // Push everything to the left

    QHBoxLayout* connectionButtons = new QHBoxLayout();
    listenBtn = new QPushButton("Start Listening", this);
    connectBtn = new QPushButton("Connect to peer", this);
    upnBtn = new QPushButton("UPnP", this);
    connect(upnBtn, &QPushButton::clicked, this, [this]{
        upnpDialog->show();
        upnpDialog->raise();
    });

    connectionButtons->addWidget(listenBtn);
    connectionButtons->addWidget(connectBtn);
    connectionButtons->addWidget(upnBtn);


    connectionLayout->addLayout(statusLayout);
    connectionLayout->addLayout(connectionFields);

    connectionLayout->addLayout(connectionButtons);
    dockLayout->addWidget(connectionGroup);

    QGroupBox* contactsGroup = new QGroupBox("Contacts", this);
    QVBoxLayout* contactsLayout = new QVBoxLayout(contactsGroup);

    contactList = new QListWidget(this);
    connect(contactList, &QListWidget::itemClicked, this, &MainWindow::onContactSelected);

    QHBoxLayout* contactButtons = new QHBoxLayout();
    addContactBtn = new QPushButton("Add", this);
    editContactBtn = new QPushButton("Edit", this);
    deleteContactBtn = new QPushButton("Delete", this);
    deleteContactBtn->setEnabled(false);

    contactButtons->addWidget(addContactBtn);
    contactButtons->addWidget(editContactBtn);
    contactButtons->addWidget(deleteContactBtn);

    contactsLayout->addWidget(contactList);
    contactsLayout->addLayout(contactButtons);
    dockLayout->addWidget(contactsGroup);

    QGroupBox* chatGroup = new QGroupBox("Chat", this);
    QVBoxLayout* chatLayout = new QVBoxLayout(chatGroup);

    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true);
    chatDisplay->setMinimumHeight(250);
    chatDisplay->setPlaceholderText("Chat messages will appear here...");

    QHBoxLayout* chatInputLayout = new QHBoxLayout();
    chatInput = new QLineEdit(this);
    chatInput->setPlaceholderText("Type a message...");
    sendBtn = new QPushButton("Send", this);

    chatInputLayout->addWidget(chatInput);
    chatInputLayout->addWidget(sendBtn);

    chatLayout->addWidget(chatDisplay);
    chatLayout->addLayout(chatInputLayout);
    dockLayout->addWidget(chatGroup);

    QGroupBox* gameControlsGroup = new QGroupBox("Game Controls", this);
    QVBoxLayout* gameControlsLayout = new QVBoxLayout(gameControlsGroup);

    startGameBtn = new QPushButton("Start Game", this);
    startGameBtn->setEnabled(false);
    stopGameBtn = new QPushButton("Stop Game", this);
    stopGameBtn->setEnabled(false);
    resignBtn = new QPushButton("Resign", this);
    resignBtn->setEnabled(false);

    gameControlsLayout->addWidget(startGameBtn);
    gameControlsLayout->addWidget(stopGameBtn);
    gameControlsLayout->addWidget(resignBtn);
    dockLayout->addWidget(gameControlsGroup);

    dockLayout->addStretch();

    remoteDock->setWidget(dockWidget);
    addDockWidget(Qt::RightDockWidgetArea, remoteDock);

    remoteDock->setVisible(false);


    connect(listenBtn, &QPushButton::clicked, this, &MainWindow::onListenClicked);
    connect(connectBtn, &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(startGameBtn, &QPushButton::clicked, this, &MainWindow::onStartRemoteGame);
    connect(stopGameBtn, &QPushButton::clicked, this, &MainWindow::onStopRemoteGame);
    connect(resignBtn, &QPushButton::clicked, this, &MainWindow::onResignGame);
    connect(addContactBtn, &QPushButton::clicked, this, &MainWindow::onAddContact);
    connect(editContactBtn, &QPushButton::clicked, this, &MainWindow::onEditContact);
    connect(deleteContactBtn, &QPushButton::clicked, this, &MainWindow::onDeleteContact);
    connect(sendBtn, &QPushButton::clicked, this, &MainWindow::onSendChatMessage);
    connect(chatInput, &QLineEdit::returnPressed, this, &MainWindow::onSendChatMessage);


}


void MainWindow::onListenClicked()
{


    if (!networkManager) {
        networkManager = new NetworkManager(this);
        chatManager = new ChatManager(networkManager, this);
    }

    if (networkManager && PawnConstants::isServerListening) {

        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Stop Listening",
            "Are you sure you want to stop listening?\n\n"
            "This will:\n"
            "• Terminate the server\n"
            "• Disconnect any connected peer\n"
            "• Stop the current game if in progress",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::No) {
            return;
        }

        networkManager->stopListening();
        connectionStatusLabel->setText("● Disconnected");
        connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        listenBtn->setText("Start Listening");
        //listenBtn->setEnabled(true);
        chatDisplay->append("[System] Stopped listening.");
        PawnConstants::isServerListening = false;
        return;
    }

    int port = portSpin->value();
    networkManager->startListening(port);
    PawnConstants::isServerListening = true;
    connectionStatusLabel->setText("● Waiting for peer...");
    connectionStatusLabel->setStyleSheet("color: orange; font-weight: bold;");
    listenBtn->setText("Stop Lisening");
    //listenBtn->setEnabled(false);
    chatDisplay->append(QString("[System] Listening on port %1. Share your IP with your friend.").arg(port));


    QString externalIP = getExternalIP();
    if (!externalIP.isEmpty()) {
        chatDisplay->append("[System] Your external IP: " + externalIP);
    }
}

void MainWindow::onConnect()
{
    if (networkManager && networkManager->isConnected()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
                    this,
                    "Disconnect",
                    "Are you sure you want to disconnect from peer?",
                    QMessageBox::Yes | QMessageBox::No
                    );

        if (reply == QMessageBox::No) {
            return;
        }

        QJsonObject msg;
        msg["type"] = "peer_disconnect";
        networkManager->sendMessage(msg);

        networkManager->disconnectFromPeer();
        networkManager->stopListening();

        connectionStatusLabel->setText("● Disconnected");
        connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        chatInput->setEnabled(false);
        startGameBtn->setEnabled(false);
        listenBtn->setEnabled(true);
        connectBtn->setText("Connect to peer");
        chatDisplay->append("[System] Disconnected from peer.");
        return;
    }

    if (!networkManager) {
        networkManager = new NetworkManager(this);
        chatManager = new ChatManager(networkManager, this);
    }


    QString ipAddr = ipEdit->text().trimmed();
    int port = portSpin->value();
    networkManager->connectToPeer(ipAddr, port);

    connectionStatusLabel->setText("● Connecting...");
    connectionStatusLabel->setStyleSheet("color: orange; font-weight: bold;");
    listenBtn->setEnabled(false);
    chatDisplay->append("[System] Connecting to " + ipAddr + ":" + QString::number(port));
    //connectBtn->setText("Disconnect");
}



void MainWindow::onStartRemoteGame()
{
    if (!networkManager || !networkManager->isConnected()) {
        QMessageBox::warning(this, "Error", "Not connected to a peer.");
        return;
    }

    if (PawnConstants::remotePlayStarted) {
        chatDisplay->append("[System] Game already in progress.");
        return;
    }

    setupChessBoard();
    PawnConstants::isRemotePlay = true;
    PawnConstants::iAMWhite = true;
    PawnConstants::remotePlayStarted = true;

    board->setPlayerColor(true);
    board->setBoardFlipped(false);

    chatManager->sendGameRequest();
    chatDisplay->append("[System] Game request sent. You are White.");

    startGameBtn->setEnabled(false);
    stopGameBtn->setEnabled(true);
    resignBtn->setEnabled(true);
    gameStatus->setText("Your turn (White) - make a move");
}



void MainWindow::onStopRemoteGame()
{
    if (!networkManager) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "Stop Game",
                "Are you sure you want to stop the game?",
                QMessageBox::Yes | QMessageBox::No
                );

    if (reply == QMessageBox::No) {
        return;
    }

    QJsonObject msg;
    msg["type"] = "game_stop";
    networkManager->sendMessage(msg);

    stopGameLocally("You stopped the game.");
}

void MainWindow::onResignGame()
{
    if (!networkManager) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "Resign",
                "Are you sure you want to resign?",
                QMessageBox::Yes | QMessageBox::No
                );

    if (reply == QMessageBox::Yes) {
        board->setGameOver(true);
        stopGameBtn->setEnabled(false);
        resignBtn->setEnabled(false);
        gameStatus->setText("You resigned.");
        chatDisplay->append("[System] You resigned.");

        QJsonObject msg;
        msg["type"] = "resign";
        networkManager->sendMessage(msg);
    }
}

void MainWindow::onAddContact()
{
    ContactDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Contact contact = dialog.getContact();
        addContact(contact);
        refreshContactList();
    }
}

void MainWindow::onEditContact()
{
    if (!contactList->currentItem()) {
        QMessageBox::warning(this, "Error", "Please select a contact to edit.");
        return;
    }

    QString contactName = contactList->currentItem()->text().split(" - ").first();
    Contact contact = getContact(contactName);

    ContactDialog dialog(this);
    dialog.setContact(contact);
    if (dialog.exec() == QDialog::Accepted) {
        Contact updatedContact = dialog.getContact();
        updateContact(contactName, updatedContact);
        refreshContactList();
    }
}

void MainWindow::onDeleteContact()
{
    if (!contactList->currentItem()) {
        QMessageBox::warning(this, "Error", "Please select a contact to delete.");
        return;
    }

    QString contactName = contactList->currentItem()->text().split(" - ").first();

    QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "Delete Contact",
                QString("Delete contact '%1'?").arg(contactName),
                QMessageBox::Yes | QMessageBox::No
                );

    if (reply == QMessageBox::Yes) {
        deleteContact(contactName);
        refreshContactList();
    }
}

void MainWindow::onSendChatMessage()
{
    QString message = chatInput->text().trimmed();
    if (message.isEmpty()) return;

    chatManager->sendChatMessage(message);
    chatDisplay->append("[You] " + message);
    playSound(CHAT_SEND);
    chatInput->clear();
}


void MainWindow::onConnectionRequestReceived(const QString& sender)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "Connection Request",
                QString("%1 wants to connect. Accept?").arg(sender),
                QMessageBox::Yes | QMessageBox::No
                );

    if (reply == QMessageBox::Yes) {
        chatManager->sendConnectionResponse(true);
    } else {
        chatManager->sendConnectionResponse(false);
    }
}

void MainWindow::onConnectionAccepted()
{
    chatDisplay->append("[System] Connected to peer.");
}

void MainWindow::onConnectionRejected()
{
    connectionStatusLabel->setText("● Disconnected");
    connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    chatInput->setEnabled(false);
    startGameBtn->setEnabled(false);
    chatDisplay->append("[System] Connection rejected.");
    listenBtn->setEnabled(true);
    connectBtn->setText("Connect to peer");
}

void MainWindow::onGameRequestReceived(const QString& sender)
{
    if (PawnConstants::remotePlayStarted) {
        chatManager->sendGameResponse(false);
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "Game Request",
                QString("%1 wants to start a game. Accept?").arg(sender),
                QMessageBox::Yes | QMessageBox::No
                );

    if (reply == QMessageBox::No) {
        chatManager->sendGameResponse(false);
        return;
    }

    setupChessBoard();
    PawnConstants::isRemotePlay = true;
    PawnConstants::iAMWhite = false;
    PawnConstants::remotePlayStarted = true;

    togglePlayerColor();

    chatManager->sendGameResponse(true);

    chatDisplay->append("[System] Game started! You are Black.");
    gameStatus->setText("Waiting for opponent (White) to move...");

    startGameBtn->setEnabled(false);
    stopGameBtn->setEnabled(true);
    resignBtn->setEnabled(true);
}


void MainWindow::onGameAccepted()
{
    chatDisplay->append("[System] Opponent accepted! You are White.");
    gameStatus->setText("Your turn (White) - make a move");
}


void MainWindow::onGameRejected()
{
    chatDisplay->append("[System] Game request rejected.");

    PawnConstants::isRemotePlay = false;
    PawnConstants::iAMWhite = false;
    PawnConstants::remotePlayStarted = false;

    startGameBtn->setEnabled(true);
    stopGameBtn->setEnabled(false);
    resignBtn->setEnabled(false);
    gameStatus->setText("Game rejected");
}


void MainWindow::onGameStarted(const QString& whitePlayer, const QString& blackPlayer)
{
    chatDisplay->append(QString("[System] Game started! %1 is White, %2 is Black.")
                        .arg(whitePlayer).arg(blackPlayer));
    playSound(RGAME_STARTED);

}

void MainWindow::onChatReceived(const QString& sender, const QString& message)
{
    chatDisplay->append("[peer] " + message);
    playSound(CHAT_RECEIVED);
}

void MainWindow::onMoveReceived(const QString& from, const QString& to)
{
    if (board->makeMove(from, to)) {
        board->highlightMove(from, to);
        updateMoveHistory(board->getLastFormattedMove());
        updateCapturedDisplay();

        if (board->isGameOver()) {
            QString winner = board->isWhiteTurn ? "Black" : "White";
            QMessageBox::information(this, "Game Over", QString("%1 wins!").arg(winner));
            return;
        }

        if (PawnConstants::isRemotePlay) {
            if (PawnConstants::iAMWhite) {
                gameStatus->setText("Opponent moved. Your turn (White)");
            } else {
                gameStatus->setText("Opponent moved. Your turn (Black)");
            }
        }

        QJsonObject ack;
        ack["type"] = "ack";
        ack["fen"] = board->getCurrentFen();
        networkManager->sendMessage(ack);
    }
}


void MainWindow::onFENReceived(const QString& fen)
{
    board->setupPosition(fen);
    board->updateGameStatus();
}


void MainWindow::setupNetworkConnections()
{
    if (!chatManager) return;

    connect(chatManager, &ChatManager::connectionRequestReceived,
            this, &MainWindow::onConnectionRequestReceived);
    connect(chatManager, &ChatManager::connectionAccepted,
            this, &MainWindow::onConnectionAccepted);
    connect(chatManager, &ChatManager::connectionRejected,
            this, &MainWindow::onConnectionRejected);
    connect(chatManager, &ChatManager::gameRequestReceived,
            this, &MainWindow::onGameRequestReceived);
    connect(chatManager, &ChatManager::gameAccepted,
            this, &MainWindow::onGameAccepted);
    connect(chatManager, &ChatManager::gameRejected,
            this, &MainWindow::onGameRejected);

    connect(chatManager, &ChatManager::gameStarted,
            this, &MainWindow::onGameStarted);
    connect(chatManager, &ChatManager::chatReceived,
            this, &MainWindow::onChatReceived);
    connect(chatManager, &ChatManager::moveReceived,
            this, &MainWindow::onMoveReceived);
    connect(chatManager, &ChatManager::fenReceived,
            this, &MainWindow::onFENReceived);

    connect(chatManager, &ChatManager::connectedToPeer,
            this, &MainWindow::onConnectedToPeer);
    connect(chatManager, &ChatManager::gameStoppedByPeer,
            this, &MainWindow::onGameStoppedByPeer);
    connect(chatManager, &ChatManager::peerDisconnected,
            this, &MainWindow::onPeerDisconnected);



    if (!networkManager || !chatManager) return;

    connect(networkManager, &NetworkManager::connected,
            chatManager, &ChatManager::onConnected);
    connect(networkManager, &NetworkManager::disconnected,
            chatManager, &ChatManager::onDisconnected);
    connect(networkManager, &NetworkManager::messageReceived,
            chatManager, &ChatManager::onMessageReceived);
    connect(networkManager, &NetworkManager::error,
            chatManager, &ChatManager::onError);
    connect(networkManager, &NetworkManager::connectionRequestReceived,
            chatManager, &ChatManager::onConnectionRequestReceived);
}

QString MainWindow::getExternalIP()
{
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("https://api.ipify.org"));
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QString ip = reply->readAll();
        return ip.trimmed();
    }
    return QString();
}

void MainWindow::onContactSelected(QListWidgetItem* item)
{
    if (!item) return;

    QString fullText = item->text();
    QStringList parts = fullText.split(" - ");

    if (parts.size() >= 2) {
        currentContactName = parts[0];  // Name
        QString address = parts[1];     // "192.168.1.1:12345"

        QStringList addressParts = address.split(":");
        if (addressParts.size() >= 2) {
            ipEdit->setText(addressParts[0]);           // IP
            portSpin->setValue(addressParts[1].toInt()); // Port
        }
    }

    editContactBtn->setEnabled(true);
    deleteContactBtn->setEnabled(true);
}

Contact MainWindow::getContact(const QString& name)
{
    QSettings settings;
    settings.beginGroup("contacts");
    QString value = settings.value(name).toString();
    settings.endGroup();

    Contact contact;
    contact.name = name;
    if (!value.isEmpty()) {
        QStringList parts = value.split(":");
        contact.address = parts[0];
        contact.port = parts.size() > 1 ? parts[1].toUShort() : 12345;
    }
    return contact;
}

void MainWindow::addContact(const Contact& contact)
{
    QSettings settings;
    settings.beginGroup("contacts");
    settings.setValue(contact.name, contact.address + ":" + QString::number(contact.port));
    settings.endGroup();
}

void MainWindow::updateContact(const QString& oldName, const Contact& contact)
{
    QSettings settings;
    settings.beginGroup("contacts");
    settings.remove(oldName);
    settings.setValue(contact.name, contact.address + ":" + QString::number(contact.port));
    settings.endGroup();
}

void MainWindow::deleteContact(const QString& name)
{
    QSettings settings;
    settings.beginGroup("contacts");
    settings.remove(name);
    settings.endGroup();
}

void MainWindow::refreshContactList()
{
    contactList->clear();
    QSettings settings;
    settings.beginGroup("contacts");
    QStringList names = settings.childKeys();
    for (const QString& name : names) {
        QString value = settings.value(name).toString();
        contactList->addItem(name + " - " + value);
    }
    settings.endGroup();
}

void MainWindow::onConnectedToPeer()
{

    connectionStatusLabel->setText("● Connected");
    connectionStatusLabel->setStyleSheet("color: green; font-weight: bold;");
    chatInput->setEnabled(true);
    startGameBtn->setEnabled(true);
    chatDisplay->append("[System] Connected to peer.");
    connectBtn->setText("Disconnect");
}

void MainWindow::stopGameLocally(const QString& reason)
{
    if (board) {
        board->setGameOver(true);
    }

    if (engine) {
        engine->sendCommand("stop");
        engine->stopThinking();
    }

    PawnConstants::isE2EPlay = false;

    stopGameBtn->setEnabled(false);
    resignBtn->setEnabled(false);
    startGameBtn->setEnabled(true);
    gameStatus->setText(reason);
    //connectBtn->setText("Connect to peer");
    PawnConstants::isRemotePlay = false;
    PawnConstants::remotePlayStarted = false;

    QMessageBox::information(this, "Game Stopped", reason);
    chatDisplay->append("[System] " + reason);
}

void MainWindow::onGameStoppedByPeer()
{
    stopGameLocally("Remote peer stopped the game.");
}

void MainWindow::onPeerDisconnected()
{
    connectionStatusLabel->setText("● Disconnected");
    connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    chatInput->setEnabled(false);
    startGameBtn->setEnabled(false);
    listenBtn->setEnabled(true);
    connectBtn->setText("Connect to peer");

    if (PawnConstants::remotePlayStarted) {
        stopGameLocally("Remote peer disconnected.");
    }

    chatDisplay->append("[System] Remote peer disconnected.");
}

void MainWindow::onPieceSetChanged(const QString &setName)
{
    if (setName.isEmpty()) return;

    if (setName == "default") {
        PawnConstants::useDefaultPieces = true;
        PawnConstants::pieceSetDirectory = "";
    } else {
        PawnConstants::useDefaultPieces = false;
        PawnConstants::pieceSetDirectory = "piecesets/" + setName + "/";
    }

    if (board) {
        board->reloadAllPieces();
        board->update();
    }

    loadPieceIcons();
    updateCapturedDisplay();
}

void MainWindow::populatePieceSets() {
    QDir piecesDir("piecesets/");
    /*
    if (piecesDir.exists()) {
        QStringList sets = piecesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        pieceSetCombo->addItems(sets);
    }
    */

    if (piecesDir.exists()) {
        QStringList sets = piecesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& set : sets) {
            if (set != "default") {  // ← SKIP "default"
                pieceSetCombo->addItem(set);
            }
        }
    }

    pieceSetCombo->setCurrentText("default");
}

void MainWindow::onBoardColorChanged(const QString& colorName) {
    if (colorName == "Classic") {
        PawnConstants::LIGHT_SQUARE = QColor(240, 240, 240);
        PawnConstants::DARK_SQUARE = QColor(90, 90, 90);
    } else if (colorName == "Wood") {
        PawnConstants::LIGHT_SQUARE = QColor(222, 184, 135);
        PawnConstants::DARK_SQUARE = QColor(139, 69, 19);
    } else if (colorName == "Green") {
        PawnConstants::LIGHT_SQUARE = QColor(200, 230, 200);
        PawnConstants::DARK_SQUARE = QColor(100, 150, 100);
    } else if (colorName == "Blue") {
        PawnConstants::LIGHT_SQUARE = QColor(200, 220, 255);
        PawnConstants::DARK_SQUARE = QColor(80, 120, 200);
    } else if (colorName == "Gray") {
        PawnConstants::LIGHT_SQUARE = QColor(220, 220, 220);
        PawnConstants::DARK_SQUARE = QColor(120, 120, 120);
    } else if (colorName == "Brown") {
        PawnConstants::LIGHT_SQUARE = QColor(205, 170, 125);
        PawnConstants::DARK_SQUARE = QColor(120, 80, 40);
    }

    if (board) {
        board->redrawSquares();
        board->update();
    }
}

void MainWindow::setupLeftDock()
{
    gameInfoDock = new QDockWidget("Game Info", this);
    gameInfoDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    gameInfoDock->setFeatures(QDockWidget::DockWidgetClosable |
                            QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable);
    QWidget* dockWidget = new QWidget(this);
    QVBoxLayout* dockLayout = new QVBoxLayout(dockWidget);
    dockLayout->setSpacing(5);
    dockLayout->setContentsMargins(5, 5, 5, 5);

    QHBoxLayout* infoLayout = new QHBoxLayout();
    timeLabel = new QLabel("Time: 00:00", this);
    moveLabel = new QLabel("Move: 0/0", this);
    timeLabel->setVisible(false);
    moveLabel->setVisible(false);
    infoLayout->addWidget(timeLabel);
    infoLayout->addWidget(moveLabel);
    infoLayout->addStretch();
    dockLayout->addLayout(infoLayout);


    /*
    QGroupBox* clocksGroup = new QGroupBox("Clocks", this);
    QVBoxLayout* clocksLayout = new QVBoxLayout(clocksGroup);
    whiteClockLabel = new QLabel("White: --:--", this);
    blackClockLabel = new QLabel("Black: --:--", this);
    clocksLayout->addWidget(whiteClockLabel);
    clocksLayout->addWidget(blackClockLabel);
    */


    QGroupBox* clocksGroup = new QGroupBox("Clocks", this);
    QVBoxLayout* clocksLayout = new QVBoxLayout(clocksGroup);

    QHBoxLayout* whiteClockLayout = new QHBoxLayout();
    whiteClockLabel = new QLabel("White: --:--", this);
    whiteClockBar = new QProgressBar(this);
    whiteClockBar->setRange(0, 600);
    whiteClockBar->setValue(600);
    whiteClockBar->setFormat("%v:%m");
    whiteClockBar->setTextVisible(true);
    whiteClockLayout->addWidget(whiteClockLabel);
    whiteClockLayout->addWidget(whiteClockBar);
    clocksLayout->addLayout(whiteClockLayout);

    QHBoxLayout* blackClockLayout = new QHBoxLayout();
    blackClockLabel = new QLabel("Black: --:--", this);
    blackClockBar = new QProgressBar(this);
    blackClockBar->setRange(0, 600);
    blackClockBar->setValue(600);
    blackClockBar->setFormat("%v:%m");
    blackClockBar->setTextVisible(true);
    blackClockLayout->addWidget(blackClockLabel);
    blackClockLayout->addWidget(blackClockBar);
    clocksLayout->addLayout(blackClockLayout);


    QPushButton* consoleToggle = new QPushButton("▼ Console", this);
    consoleToggle->setCheckable(true);
    consoleToggle->setChecked(false);
    dockLayout->addWidget(consoleToggle);

    //QSplitter* splitter = new QSplitter(Qt::Vertical, this);


    consoleEdit = new QTextEdit(this);
    consoleEdit->setReadOnly(true);
    consoleEdit->setMinimumHeight(300);
    consoleEdit->setVisible(false);
    consoleEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: black;"
        "    color: white;"
        "}"
    );
    consoleEdit->ensureCursorVisible();
    dockLayout->addWidget(consoleEdit);
    //splitter->addWidget(consoleEdit);

    QHBoxLayout* commandLayout = new QHBoxLayout();
    QLineEdit* commandEdit = new QLineEdit(this);
    commandEdit->setPlaceholderText("Enter UCI command...");
    QPushButton* commandBtn = new QPushButton("Send", this);

    connect(commandBtn, &QPushButton::clicked, [this, commandEdit]() {
        QString cmd = commandEdit->text().trimmed();
        if (!cmd.isEmpty() && engine) {
            // Show command FIRST
            consoleEdit->append("> " + cmd);
            commandEdit->clear();

            // Then send to engine (response will appear after)
            engine->sendUCICommand(cmd);
        }
    });

    connect(commandEdit, &QLineEdit::returnPressed, [this, commandEdit]() {
        QString cmd = commandEdit->text().trimmed();
        if (!cmd.isEmpty() && engine) {
            consoleEdit->append("> " + cmd);
            commandEdit->clear();
            engine->sendUCICommand(cmd);
        }
    });

    commandLayout->addWidget(commandEdit);
    commandLayout->addWidget(commandBtn);
    dockLayout->addLayout(commandLayout);

    connect(consoleToggle, &QPushButton::toggled, [this, consoleToggle](bool checked) {
        consoleEdit->setVisible(checked);
        consoleToggle->setText(checked ? "▲ Console" : "▼ Console");
    });

    QPushButton* historyToggle = new QPushButton("▼ History", this);
    historyToggle->setCheckable(true);
    historyToggle->setChecked(false);
    dockLayout->addWidget(historyToggle);

    moveTable = new QTableWidget(this);
    moveTable->setColumnCount(2);
    moveTable->setHorizontalHeaderLabels({"White", "Black"});
    moveTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    moveTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);


    moveTable->verticalHeader()->setVisible(false);
    moveTable->setVisible(false);
    moveTable->setMinimumHeight(300);

    connect(moveTable->model(), &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex&, int, int) {
        moveTable->scrollToBottom();
    });

    //splitter->addWidget(moveTable);
    dockLayout->addWidget(moveTable);
    //dockLayout->addWidget(splitter);

    connect(historyToggle, &QPushButton::toggled, [this, historyToggle](bool checked) {
        moveTable->setVisible(checked);
        historyToggle->setText(checked ? "▲ History" : "▼ History");
    });

    QHBoxLayout* pgnLayout = new QHBoxLayout();
    QPushButton* savePgnBtn = new QPushButton("Save PGN", this);
    connect(savePgnBtn, &QPushButton::clicked, this, &MainWindow::savePGN);
    QPushButton* engineLoggingBtn = new QPushButton("Log");
    engineLoggingBtn->setToolTip("Enable/disable engine output logging to console");
    engineLoggingBtn->setCheckable(true);
    engineLoggingBtn->setChecked(false);
    //engineLoggingBtn->setMaximumWidth(20);
    connect(engineLoggingBtn, &QPushButton::toggled, this, [this](bool toggled){
        PawnConstants::isEngineLoggingEnabled = toggled;
        if (toggled) {
               consoleEdit->append(">>> Engine logging enabled");
           } else {
               consoleEdit->append(">>> Engine logging disabled");
           }
    });


    QPushButton* copyPgnBtn = new QPushButton("Copy PGN", this);
    connect(copyPgnBtn, &QPushButton::clicked, this, &MainWindow::copyPGN);
    pgnLayout->addWidget(savePgnBtn);
    pgnLayout->addWidget(engineLoggingBtn);
    pgnLayout->addWidget(copyPgnBtn);
    dockLayout->addLayout(pgnLayout);
    dockLayout->addStretch();

    dockLayout->addWidget(clocksGroup);


    gameInfoDock->setWidget(dockWidget);
    addDockWidget(Qt::LeftDockWidgetArea, gameInfoDock);
    gameInfoDock->setVisible(false);  // Hidden by default


    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(1000);
    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::updateClocks);
}

void MainWindow::onSetBackgroundColor() {
    QColor color = QColorDialog::getColor(Qt::white, this, "Select Background Color");
    if (color.isValid()) {
        view->setBackgroundBrush(QBrush(color));
        QSettings settings;
        settings.setValue("backgroundcolor", color);
    }
}



QString MainWindow::formatTime(int seconds) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
                           .arg(secs, 2, 10, QChar('0'));
}


void MainWindow::updateClockColor(QProgressBar* bar, int timeLeft) {
    int totalTime = bar->maximum();
    if (totalTime <= 0) return;

    int percent = (timeLeft * 100) / totalTime;

    QString color;
    if (percent > 60) {
        color = "#2196F3";  // Green
    } else if (percent > 30) {
        color = "#FFC107";  // Yellow
    } else if (percent > 10) {
        color = "#FF9800";  // Orange
    } else {
        color = "#F44336";  // Red
    }

    bar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(color));
}

void MainWindow::startClocks(int minutes, int increment) {
    m_clockMinutes = minutes;
    m_clockIncrement = increment;
    m_clocksEnabled = true;

    m_whiteTime = minutes * 60;
    m_blackTime = minutes * 60;

    whiteClockBar->setRange(0, m_whiteTime);
    blackClockBar->setRange(0, m_blackTime);
    whiteClockBar->setValue(m_whiteTime);
    blackClockBar->setValue(m_blackTime);

    whiteClockLabel->setText("White: " + formatTime(m_whiteTime));
    blackClockLabel->setText("Black: " + formatTime(m_blackTime));

    updateClockColor(whiteClockBar, m_whiteTime);
    updateClockColor(blackClockBar, m_blackTime);

    m_clockTimer->start();
}

void MainWindow::stopClocks() {
    m_clockTimer->stop();
    m_clocksEnabled = false;
}

void MainWindow::resetClocks() {
    m_clockTimer->stop();
    m_whiteTime = 0;
    m_blackTime = 0;
    whiteClockBar->setValue(0);
    blackClockBar->setValue(0);
    whiteClockLabel->setText("White: --:--");
    blackClockLabel->setText("Black: --:--");
    m_clocksEnabled = false;
}

void MainWindow::updateClocks() {
    if (!m_clocksEnabled || !board) return;
    if (board->isGameOver()) {
        stopClocks();
        return;
    }

    if (board->isWhiteTurn) {
        m_whiteTime--;
        if (m_whiteTime < 0) m_whiteTime = 0;
        whiteClockBar->setValue(m_whiteTime);
        whiteClockLabel->setText("White: " + formatTime(m_whiteTime));
        updateClockColor(whiteClockBar, m_whiteTime);
    } else {
        m_blackTime--;
        if (m_blackTime < 0) m_blackTime = 0;
        blackClockBar->setValue(m_blackTime);
        blackClockLabel->setText("Black: " + formatTime(m_blackTime));
        updateClockColor(blackClockBar, m_blackTime);
    }

    if (m_whiteTime <= 0 || m_blackTime <= 0) {
        stopClocks();
        board->setGameOver(true);
        QString winner = (m_whiteTime <= 0) ? "Black" : "White";
        gameStatus->setText(QString("%1 wins on time!").arg(winner));
        QMessageBox::information(this, "Time Out", QString("%1 wins on time!").arg(winner));
        PawnConstants::isE2EPlay = false;
    }
}



void MainWindow::onStopGame()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Stop Game",
        "Are you sure you want to stop the game?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::No) {
        return;
    }

    if (engine) {
        engine->sendCommand("stop");
        engine->stopThinking();
    }

    PawnConstants::isE2EPlay = false;

    /*
    stopClocks();

    if (board) {
        board->setGameOver(true);
    }

    stopGameButton->setEnabled(false);
    resignButton->setEnabled(false);
    drawButton->setEnabled(false);
    gameStatus->setText("Game stopped");

    chatDisplay->append("[System] Game stopped.");
    */
    //playGameoverSound();
    playSound(SOUNDTYPE::SOUND_CHECKMATE);

}

void MainWindow::onResign()
{
    if (PawnConstants::isE2EPlay) {
        QMessageBox::information(this, "Resign", "Cannot resign in engine vs engine mode.");
        return;
    }

    if (board->isGameOver()) {
        QMessageBox::information(this, "Resign", "Game is already over.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Resign",
        "Are you sure you want to resign?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::No) {
        return;
    }

    bool isWhite = board->playerIsWhite;
    QString winner = isWhite ? "Black" : "White";

    board->setGameOver(true);
    gameStatus->setText(QString("%1 wins by resignation.").arg(winner));
    //playGameoverSound();
    playSound(SOUNDTYPE::SOUND_CHECKMATE);

    if (engine) {
        engine->sendCommand("stop");
        engine->stopThinking();
    }

    PawnConstants::isE2EPlay = false;

    stopGameButton->setEnabled(false);
    resignButton->setEnabled(false);
    drawButton->setEnabled(false);

    if (PawnConstants::isRemotePlay) {
        QJsonObject msg;
        msg["type"] = "resign";
        networkManager->sendMessage(msg);
    }

}

void MainWindow::onDrawOffer()
{
    if (PawnConstants::isE2EPlay) {
        QMessageBox::information(this, "Draw Offer", "Cannot offer draw in engine vs engine mode.");
        return;
    }

    if (PawnConstants::startDialogPlayer1IsMachine || PawnConstants::startDialogPlayer2IsMachine) {
        QMessageBox::information(this, "Draw Offer", "Only available in human to human play");
        return;
    }

    if (board->isGameOver()) {
        QMessageBox::information(this, "Draw Offer", "Game is already over.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Offer Draw",
        "Do you want to offer a draw?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::No) {
        return;
    }

    if (!PawnConstants::isRemotePlay) {
        QMessageBox::StandardButton response = QMessageBox::question(
            this,
            "Draw Offer",
            "Opponent is offering a draw. Accept?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (response == QMessageBox::Yes) {
            board->setGameOver(true);
            gameStatus->setText("Draw by agreement.");
            QMessageBox::information(this, "Draw", "Game is a draw.");

            stopGameButton->setEnabled(false);
            resignButton->setEnabled(false);
            drawButton->setEnabled(false);
        } else {
            gameStatus->setText("Draw offer declined. Continue playing.");
        }
    } else {
        QJsonObject msg;
        msg["type"] = "draw_offer";
        networkManager->sendMessage(msg);
        chatDisplay->append("[System] Draw offer sent to opponent.");
        gameStatus->setText("Draw offer sent. Waiting for response...");

        drawButton->setEnabled(false);
    }
}

void MainWindow::onDrawOfferReceived()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Draw Offer",
        "Opponent offers a draw. Accept?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        board->setGameOver(true);
        gameStatus->setText("Draw by agreement.");
        QMessageBox::information(this, "Draw", "Game is a draw.");

        stopGameButton->setEnabled(false);
        resignButton->setEnabled(false);
        drawButton->setEnabled(false);

        QJsonObject msg;
        msg["type"] = "draw_response";
        msg["accepted"] = true;
        networkManager->sendMessage(msg);
    } else {
        gameStatus->setText("Draw offer declined. Continue playing.");
        chatDisplay->append("[System] You declined the draw offer.");

        QJsonObject msg;
        msg["type"] = "draw_response";
        msg["accepted"] = false;
        networkManager->sendMessage(msg);
    }
}

void MainWindow::onDrawResponseReceived(bool accepted)
{
    if (accepted) {
        board->setGameOver(true);
        gameStatus->setText("Draw by agreement.");
        QMessageBox::information(this, "Draw", "Opponent accepted the draw offer.");
        stopGameButton->setEnabled(false);
        resignButton->setEnabled(false);
        drawButton->setEnabled(false);
        //playGameoverSound();
        playSound(SOUNDTYPE::SOUND_CHECKMATE);

    } else {
        gameStatus->setText("Draw offer declined. Continue playing.");
        chatDisplay->append("[System] Opponent declined the draw offer.");
        drawButton->setEnabled(true);  // Re-enable draw button
    }
}



void MainWindow::onSquareSize() {
    QDialog dialog(this);
    dialog.setWindowTitle("Square Size");
    dialog.setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QLabel* label = new QLabel("Size: " + QString::number(PawnConstants::SQUARE_SIZE) + "px", &dialog);
    layout->addWidget(label);

    QSlider* slider = new QSlider(Qt::Horizontal, &dialog);
    slider->setRange(40, 100);
    slider->setValue(PawnConstants::SQUARE_SIZE);
    slider->setTickInterval(10);
    slider->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(slider);

    QPushButton* applyBtn = new QPushButton("Apply", &dialog);
    QPushButton* cancelBtn = new QPushButton("Cancel", &dialog);
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(applyBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    // Live preview
    connect(slider, &QSlider::valueChanged, [label](int value) {
        label->setText("Size: " + QString::number(value) + "px");
    });

    connect(applyBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        int newSize = slider->value();
        PawnConstants::SQUARE_SIZE = newSize;

        // Redraw board with new size
        board->redrawSquares();

        // Reload and reposition all pieces
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                ChessPiece* piece = board->getPiecesRef()[row][col];
                if (piece) {
                    // Reposition piece to new grid
                    piece->setPos(col * PawnConstants::SQUARE_SIZE, row * PawnConstants::SQUARE_SIZE);
                    // Reload image at new size
                    board->loadPieceImage(piece);
                }
            }
        }

        // Reload captured pieces
        loadPieceIcons();
        updateCapturedDisplay();

        // Update view
        board->update();

        // Save settings
        QSettings settings;
        settings.setValue("squareSize", newSize);
    }
}


// PGN

void MainWindow::savePGN()
{
    QString pgn = generatePGN();

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save PGN",
        PawnConstants::savePGNDirPath + "/game.pgn",
        "PGN Files (*.pgn);;All Files (*)"
    );

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(pgn.toUtf8());
        file.close();
    }
}

void MainWindow::copyPGN()
{
    QString pgn = generatePGN();
    QApplication::clipboard()->setText(pgn);
    QMessageBox::information(this, "Copied!", "PGN copied to clipboard!");
}


QString MainWindow::generatePGN()
{
    QString pgn;

    // Headers
    pgn += "[Event \"Casual Game\"]\n";
    pgn += "[Site \"Pawns\"]\n";
    pgn += "[Date \"" + QDate::currentDate().toString("yyyy.MM.dd") + "\"]\n";
    pgn += "[White \"You\"]\n";
    pgn += "[Black \"Engine\"]\n";

    // Result
    QString result = "*";
    if (board->isGameOver()) {
        if (gameStatus->text().contains("White wins")) result = "1-0";
        else if (gameStatus->text().contains("Black wins")) result = "0-1";
        else if (gameStatus->text().contains("Draw")) result = "1/2-1/2";
    }
    pgn += "[Result \"" + result + "\"]\n";
    pgn += "\n";

    // ===== CONVERT MOVES TO PGN =====
    QStringList lines = moveHistory->toPlainText().split('\n', Qt::SkipEmptyParts);
    QString moveText;
    int moveNumber = 1;

    for (const QString& line : lines) {
        QString pgnMove = convertToPGN(line);

        if (!pgnMove.isEmpty()) {
            if (moveNumber % 2 == 1) {
                moveText += QString::number((moveNumber / 2) + 1) + ". " + pgnMove + " ";
            } else {
                moveText += pgnMove + " ";
            }
            moveNumber++;
        }
    }

    pgn += moveText;
    return pgn;
}

QString MainWindow::convertToPGN(const QString& moveText)
{
    // ===== CHECK FOR SPECIAL MOVE LINES =====
    if (moveText.contains("Castling Performed!")) {
        return "";  // Skip, actual move follows
    }
    if (moveText.contains("EnPassant Capture Performed!")) {
        return "ENPASSANT";  // Marker
    }
    if (moveText.contains("Pawn Promotion Performed!")) {
        return "PROMOTION";  // Marker
    }

    // ===== NORMAL MOVE PARSING =====
    QRegularExpression re("moves from (\\w{2}) to (\\w{2})");
    QRegularExpressionMatch match = re.match(moveText);

    if (match.hasMatch()) {
        QString from = match.captured(1);
        QString to = match.captured(2);

        // ===== CASTLING =====
        if ((from == "e1" && to == "g1") || (from == "e8" && to == "g8")) {
            return "O-O";
        }
        if ((from == "e1" && to == "c1") || (from == "e8" && to == "c8")) {
            return "O-O-O";
        }

        // ===== PROMOTION =====
        if (moveText.contains("Promotion") || moveText.contains("promotion")) {
            QString piece = "";
            if (moveText.contains("Queen")) piece = "Q";
            else if (moveText.contains("Rook")) piece = "R";
            else if (moveText.contains("Bishop")) piece = "B";
            else if (moveText.contains("Knight")) piece = "N";
            return to + "=" + piece;
        }

        // ===== EN PASSANT =====
        if (moveText.contains("EnPassant") || moveText.contains("en passant")) {
            return QString(from[0]) + "x" + to + " e.p.";
        }

        // ===== NORMAL MOVE =====
        QString piece = "";
        if (moveText.contains("King")) piece = "K";
        else if (moveText.contains("Queen")) piece = "Q";
        else if (moveText.contains("Rook")) piece = "R";
        else if (moveText.contains("Bishop")) piece = "B";
        else if (moveText.contains("Knight")) piece = "N";

        bool isCapture = moveText.contains("capturing") || moveText.contains("captures");

        if (isCapture) {
            if (!piece.isEmpty()) {
                return piece + "x" + to;
            } else {
                return QString(from[0]) + "x" + to;
            }
        } else {
            if (!piece.isEmpty()) {
                return piece + to;
            } else {
                return to;
            }
        }
    }
    return moveText;
}

// replaydialog

void MainWindow::onReplayGame()
{


    if(replayDialog) {
        replayDialog->show();
        replayDialog->raise();
    }
}

void MainWindow::saveHistory()
{
    QStringList moves = getMoveHistoryFromTable();

    if (moves.isEmpty()) {
        QMessageBox::information(this, "Save History", "No moves to save.");
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString defaultName = PawnConstants::saveHistoryDirPath + "/history_" + timestamp + ".txt";

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Game History",
        defaultName,
        "History Files (*.txt);;All Files (*)"
    );

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(moves.join("\n").toUtf8());
        file.close();
        QMessageBox::information(this, "Save History", "History saved successfully.");
    }
}

QStringList MainWindow::getMoveHistoryFromTable()
{
    QStringList moves;
    for (int row = 0; row < moveTable->rowCount(); row++) {
        QString white = moveTable->item(row, 0) ? moveTable->item(row, 0)->text() : "";
        QString black = moveTable->item(row, 1) ? moveTable->item(row, 1)->text() : "";

        if (!white.isEmpty()) moves.append(white);
        if (!black.isEmpty()) moves.append(black);
    }
    return moves;
}

void MainWindow::playSound(SOUNDTYPE type)
{
    if(!playSounds) return;

    QString soundFile;

    switch(type) {
        case SOUND_MOVE:
            soundFile = "move_009.wav";
            break;
        case SOUND_CHECK:
            soundFile = "check.wav";
            break;
        case SOUND_CHECKMATE:
            soundFile = "gameover.mp3";
            break;
        case SOUND_CAPTURE:
            soundFile = "capture.wav";
            break;
        case SOUND_PROMOTION:
            soundFile = "specialmove.mp3";
            break;
        case SOUND_CASTLING:
            soundFile = "specialmove.mp3";
            break;
        case SOUND_ENPASSANT:
            soundFile = "specialmove.mp3";
            break;
        case SOUND_GAME_OVER:
            soundFile = "gameover.wav";
            break;
        case SOUND_START:
            soundFile = "start.wav";
            break;
        case CHAT_SEND:
            soundFile = "chatsent.mp3";
            break;
        case CHAT_RECEIVED:
            soundFile = "chatreceived.mp3";
            break;
        default:
            soundFile = "move_009.wav";
            break;
    }

    m_soundPlayer->setSource(QUrl("qrc:/sound/" + soundFile));
    m_soundPlayer->play();
}

StockfishEngine *MainWindow::createNewEngine()
{

    if (engine) {
        engine->stopThinking();
        delete engine;
        engine = nullptr;
    }

    engine = new StockfishEngine(this);
    connect(engine, &StockfishEngine::engineOutput, this, [this](const QString& line){
            consoleEdit->append(line);
            consoleEdit->ensureCursorVisible();
    });
    return engine;
}

void MainWindow::openFolder() {
    // Optional: Check if the folder exists
    QDir dir(PawnConstants::APPDIR);
    if (!dir.exists()) {
        return;
    }

    // Convert local path to URL and open
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(PawnConstants::APPDIR))) {
    }
}

void MainWindow::createPawnsSymlink()
{
#ifdef FLATPAK_BUILD
    QString msg = "";
    QMessageBox msgBox;
    msgBox.setWindowTitle("Flatpak Permission Required");
    msgBox.setText(QString(
                       "%1 is running as a Flatpak and may not have access to your home directory.\n\n"
                       "To create a symlink, you may need to grant home directory access first.\n\n"
                       "Option 1 - Terminal:\n"
                       "  Grant access:\n"
                       "    flatpak override --user --filesystem=home io.github.alamahant.%1\n\n"
                       "  Revoke access later:\n"
                       "    flatpak override --user --nofilesystem=home io.github.alamahant.%1\n\n"
                       "Option 2 - Flatseal:\n"
                       "  Install Flatseal from Flathub and grant 'Home' access to %1.\n\n"
                       "If you have already granted permissions, you can continue."
                       ).arg(QApplication::applicationName()));

    msgBox.setIcon(QMessageBox::Information);

    QPushButton *continueButton = msgBox.addButton("Continue", QMessageBox::AcceptRole);
    QPushButton *cancelButton = msgBox.addButton("Cancel", QMessageBox::RejectRole);
    msgBox.setDefaultButton(cancelButton);

    msgBox.exec();

    if (msgBox.clickedButton() != continueButton) {
        return; // User cancelled
    }

#endif
    // Open dialog to select destination folder
    QString destinationDir = QFileDialog::getExistingDirectory(
                this,
                "Select Destination Folder for Symlink",
                QDir::homePath(),
                QFileDialog::ShowDirsOnly
                );

    if (destinationDir.isEmpty()) {
        return; // User cancelled
    }

    // Create symlink path
    QString symlinkPath = QDir(destinationDir).filePath(QApplication::applicationName());
    // Check if symlink already exists
    if (QFile::exists(symlinkPath) || QFileInfo(symlinkPath).isSymLink()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
                    this,
                    "Symlink Exists",
                    QString("A file or symlink already exists at:\n%1\n\nOverwrite?").arg(symlinkPath),
                    QMessageBox::Yes | QMessageBox::No
                    );

        if (reply != QMessageBox::Yes) {
            return;
        }

        // Remove existing file/symlink
        if (!QFile::remove(symlinkPath)) {
            QMessageBox::warning(this, "Error", "Could not remove existing file/symlink");
            return;
        }
    }

    // Create the symlink
    QString targetPath = PawnConstants::APPDIR;

    if (!QFile::exists(targetPath)) {
        QMessageBox::warning(this, "Error",
                             QString("Target directory does not exist:\n%1").arg(targetPath));
        return;
    }

    if (QFile::link(targetPath, symlinkPath)) {
        QMessageBox::information(
                    this,
                    "Symlink Created",
                    QString("Symlink created successfully!\n\n"
                            "Name: %3\n"
                            "Location: %1\n\n"
                            "Now you can access Pawn data from:\n%2")
                    .arg(destinationDir)
                    .arg(symlinkPath)
                    .arg(QApplication::applicationName())
                    );
    } else {
        QMessageBox::warning(
                    this,
                    "Error",
                    QString("Failed to create symlink.\n\n"
                            "Destination: %1\n"
                            "Target: %2\n\n"
                            "Possible reasons:\n"
                            "• Insufficient permissions\n"
                            "• Invalid destination path\n"
                            "• Filesystem doesn't support symlinks")
                    .arg(symlinkPath)
                    .arg(targetPath)
                    );
    }
}


