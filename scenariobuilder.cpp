#include "scenariobuilder.h"
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include<QDebug>

ScenarioBuilder::ScenarioBuilder(QWidget *parent)
    : QDialog(parent)
    , selectedPiece("")
    , selectedPosition("")
{
    setupUI();
    setWindowTitle("Position Setup");
    setModal(false);
    setMinimumSize(700, 700);
    resize(700,700);


}

ScenarioBuilder::~ScenarioBuilder()
{
}

void ScenarioBuilder::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QHBoxLayout* contentLayout = new QHBoxLayout();

    QGroupBox* piecesGroup = new QGroupBox("Pieces", this);
    QVBoxLayout* piecesLayout = new QVBoxLayout(piecesGroup);

    piecesList = new QListWidget(this);
    piecesList->addItem("♔ White King");
    piecesList->addItem("♕ White Queen");
    piecesList->addItem("♖ White Rook");
    piecesList->addItem("♗ White Bishop");
    piecesList->addItem("♘ White Knight");
    piecesList->addItem("♙ White Pawn");
    piecesList->addItem("♚ Black King");
    piecesList->addItem("♛ Black Queen");
    piecesList->addItem("♜ Black Rook");
    piecesList->addItem("♝ Black Bishop");
    piecesList->addItem("♞ Black Knight");
    piecesList->addItem("♟ Black Pawn");
    piecesList->addItem("— Remove —");

    connect(piecesList, &QListWidget::itemClicked, this, &ScenarioBuilder::onPieceSelected);

    piecesLayout->addWidget(piecesList);
    piecesGroup->setLayout(piecesLayout);
    contentLayout->addWidget(piecesGroup);

    QGroupBox* settingsGroup = new QGroupBox("Settings", this);
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsGroup);

    QGroupBox* turnGroup = new QGroupBox("Turn", this);
    QVBoxLayout* turnLayout = new QVBoxLayout(turnGroup);
    whiteTurn = new QRadioButton("White to move", this);
    blackTurn = new QRadioButton("Black to move", this);
    whiteTurn->setChecked(true);

    connect(whiteTurn, &QRadioButton::toggled, this, &ScenarioBuilder::onTurnChanged);
    connect(blackTurn, &QRadioButton::toggled, this, &ScenarioBuilder::onTurnChanged);

    turnLayout->addWidget(whiteTurn);
    turnLayout->addWidget(blackTurn);
    turnGroup->setLayout(turnLayout);
    settingsLayout->addWidget(turnGroup);

    QGroupBox* castlingGroup = new QGroupBox("Castling Rights", this);
    QHBoxLayout* castlingLayout = new QHBoxLayout(castlingGroup);
    castleK = new QCheckBox("K", this);
    castleQ = new QCheckBox("Q", this);
    castlek = new QCheckBox("k", this);
    castleq = new QCheckBox("q", this);
    castleK->setChecked(true);
    castleQ->setChecked(true);
    castlek->setChecked(true);
    castleq->setChecked(true);
    castlingLayout->addWidget(castleK);
    castlingLayout->addWidget(castleQ);
    castlingLayout->addWidget(castlek);
    castlingLayout->addWidget(castleq);
    castlingGroup->setLayout(castlingLayout);
    settingsLayout->addWidget(castlingGroup);

    QGroupBox* enPassantGroup = new QGroupBox("En Passant Target", this);
    QVBoxLayout* enPassantLayout = new QVBoxLayout(enPassantGroup);
    enPassantCombo = new QComboBox(this);
    enPassantCombo->setToolTip("Select the en passant target square if available");
    enPassantCombo->addItem("None", "");
    QStringList files = {"a","b","c","d","e","f","g","h"};
    for (int rank = 1; rank <= 8; rank++) {
        for (const QString& file : files) {
            QString square = file + QString::number(rank);
            enPassantCombo->addItem(square, square);
        }
    }
    enPassantLayout->addWidget(enPassantCombo);
    enPassantGroup->setLayout(enPassantLayout);
    settingsLayout->addWidget(enPassantGroup);

    QGroupBox* moveGroup = new QGroupBox("Move Counters", this);
    QHBoxLayout* moveLayout = new QHBoxLayout(moveGroup);
    halfmoveSpin = new QSpinBox(this);
    halfmoveSpin->setRange(0, 999);
    halfmoveSpin->setValue(0);
    fullmoveSpin = new QSpinBox(this);
    fullmoveSpin->setRange(1, 999);
    fullmoveSpin->setValue(1);
    moveLayout->addWidget(new QLabel("Halfmove:", this));
    moveLayout->addWidget(halfmoveSpin);
    moveLayout->addWidget(new QLabel("Fullmove:", this));
    moveLayout->addWidget(fullmoveSpin);
    moveGroup->setLayout(moveLayout);
    settingsLayout->addWidget(moveGroup);

    QGroupBox* actionGroup = new QGroupBox("Actions", this);
    QVBoxLayout* actionLayout = new QVBoxLayout(actionGroup);
    QPushButton* placeBtn = new QPushButton("Place", this);
    QPushButton* removeBtn = new QPushButton("Remove", this);
    QPushButton* clearBtn = new QPushButton("Clear Board", this);
    QPushButton* startBtn = new QPushButton("Start Position", this);
    startBtn->setVisible(false);
    connect(placeBtn, &QPushButton::clicked, this, &ScenarioBuilder::onPlaceClicked);
    connect(removeBtn, &QPushButton::clicked, this, &ScenarioBuilder::onRemoveClicked);
    connect(clearBtn, &QPushButton::clicked, this, &ScenarioBuilder::onClearBoardClicked);
    connect(startBtn, &QPushButton::clicked, this, &ScenarioBuilder::onStartPositionClicked);

    actionLayout->addWidget(placeBtn);
    actionLayout->addWidget(removeBtn);
    actionLayout->addWidget(clearBtn);
    actionLayout->addWidget(startBtn);
    actionGroup->setLayout(actionLayout);
    settingsLayout->addWidget(actionGroup);

    QHBoxLayout* fenLayout = new QHBoxLayout();
    QPushButton* copyBtn = new QPushButton("Copy FEN", this);
    QPushButton* pasteBtn = new QPushButton("Paste FEN", this);
    pasteBtn->setToolTip("Paste a FEN string from clipboard to load a scenario");
    connect(copyBtn, &QPushButton::clicked, this, &ScenarioBuilder::onCopyFENClicked);
    connect(pasteBtn, &QPushButton::clicked, this, &ScenarioBuilder::onPasteFENClicked);
    fenLayout->addWidget(copyBtn);
    fenLayout->addWidget(pasteBtn);
    settingsLayout->addLayout(fenLayout);

    QPushButton* setBtn = new QPushButton("Set Position", this);
    setBtn->setVisible(false);
    QPushButton* closeBtn = new QPushButton("Close", this);
    closeBtn->setToolTip("Need to close the Scenario Builder to apply the position to the board");
    QPushButton* saveBtn = new QPushButton("Save", this);
    saveBtn->setToolTip("Save the scenario to load and replay later");
    connect(setBtn, &QPushButton::clicked, this, &ScenarioBuilder::onSetPositionClicked);
    connect(closeBtn, &QPushButton::clicked, this, &ScenarioBuilder::onCloseClicked);
    connect(saveBtn, &QPushButton::clicked, this, &ScenarioBuilder::saveRequested);

    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(setBtn);
    bottomLayout->addWidget(saveBtn);
    bottomLayout->addWidget(closeBtn);
    settingsLayout->addLayout(bottomLayout);

    settingsGroup->setLayout(settingsLayout);
    contentLayout->addWidget(settingsGroup);

    QGroupBox* positionsGroup = new QGroupBox("Positions", this);
    QVBoxLayout* positionsLayout = new QVBoxLayout(positionsGroup);

    positionsList = new QListWidget(this);
    for (int file = 0; file < 8; file++) {              // ← File first (a to h)
        for (int rank = 1; rank <= 8; rank++) {         // ← Rank second (1 to 8)
            QString square = QString(QChar('a' + file)) + QString::number(rank);
            positionsList->addItem(square);
        }
    }

    connect(positionsList, &QListWidget::itemClicked, this, &ScenarioBuilder::onPositionSelected);

    positionsLayout->addWidget(positionsList);
    positionsGroup->setLayout(positionsLayout);
    contentLayout->addWidget(positionsGroup);

    mainLayout->addLayout(contentLayout);

    statusLabel = new QLabel("Select a piece, select a position, click Place", this);
    statusLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; }");
    mainLayout->addWidget(statusLabel);
}

void ScenarioBuilder::onPieceSelected(QListWidgetItem* item)
{
    selectedPiece = item->text();
    updateStatusLabel();
}

void ScenarioBuilder::onPositionSelected(QListWidgetItem* item)
{
    selectedPosition = item->text();
    updateStatusLabel();
}

void ScenarioBuilder::onPlaceClicked()
{
    if (selectedPiece.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a piece first.");
        return;
    }
    if (selectedPosition.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a position first.");
        return;
    }
    if (selectedPiece == "— Remove —") {
        emit removePieceRequested(selectedPosition);
        return;
    }

    // ===== CHECK IF SQUARE IS OCCUPIED =====
    if (board) {
        int col = selectedPosition[0].toLatin1() - 'a';
        int row = 8 - selectedPosition[1].digitValue();

        if (board->getPiecesRef()[row][col]) {
            QMessageBox::warning(this, "Square Occupied",
                                 "This square already has a piece.\n"
                                 "Remove it first before placing a new piece.");
            return;
        }
    }

    emit placePieceRequested(selectedPiece, selectedPosition);
}

void ScenarioBuilder::onRemoveClicked()
{
    if (selectedPosition.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a position first.");
        return;
    }
    emit removePieceRequested(selectedPosition);
}

void ScenarioBuilder::onClearBoardClicked()
{
    emit clearBoardRequested();
}

void ScenarioBuilder::onStartPositionClicked()
{
    emit startPositionRequested();
}

void ScenarioBuilder::onSetPositionClicked()
{
    QString fen = generateFEN();
    emit setPositionRequested(fen);
    QMessageBox::information(this, "Success", "Position set! You can now close this dialog.");
}

void ScenarioBuilder::onCopyFENClicked()
{
    emit fenCopyRequested();
    return;
    QString fen = generateFEN();
    QApplication::clipboard()->setText(fen);
    QMessageBox::information(this, "Copied!", "FEN copied to clipboard!");
}

/*
void ScenarioBuilder::onPasteFENClicked()
{
    QString fen = QApplication::clipboard()->text();
    if (fen.isEmpty()) {
        QMessageBox::warning(this, "Error", "Clipboard is empty.");
        return;
    }

    QStringList parts = fen.split(' ');
    if (parts.size() > 1) {
        if (parts[1] == "w") whiteTurn->setChecked(true);
        else blackTurn->setChecked(true);
    }

    emit setPositionRequested(fen);
    QMessageBox::information(this, "Success", "FEN loaded from clipboard!");
}
*/

void ScenarioBuilder::onPasteFENClicked()
{
    QString fen = QApplication::clipboard()->text();
    if (fen.isEmpty()) {
        QMessageBox::warning(this, "Error", "Clipboard is empty.");
        return;
    }

    QStringList parts = fen.split(' ');

    // Turn
    if (parts.size() > 1) {
        if (parts[1] == "w") whiteTurn->setChecked(true);
        else blackTurn->setChecked(true);
    }

    // Castling rights
    if (parts.size() > 2) {
        QString castling = parts[2];
        castleK->setChecked(castling.contains('K'));
        castleQ->setChecked(castling.contains('Q'));
        castlek->setChecked(castling.contains('k'));
        castleq->setChecked(castling.contains('q'));
    }

    // En passant
    if (parts.size() > 3) {
        QString ep = parts[3];
        if (ep != "-") {
            int index = enPassantCombo->findData(ep);
            if (index >= 0) enPassantCombo->setCurrentIndex(index);
        } else {
            enPassantCombo->setCurrentIndex(0);
        }
    }

    // Move counters
    if (parts.size() > 4) {
        halfmoveSpin->setValue(parts[4].toInt());
    }
    if (parts.size() > 5) {
        fullmoveSpin->setValue(parts[5].toInt());
    }

    emit setPositionRequested(fen);
    QMessageBox::information(this, "Success", "FEN loaded from clipboard!");
}

void ScenarioBuilder::onCloseClicked()
{
    /*
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Save Scenario",
        "Do you want to save this scenario before closing?",
        QMessageBox::Save | QMessageBox::Cancel
    );

    if (reply == QMessageBox::Cancel) {
        return;  // Don't close, stay open
    } else if (reply == QMessageBox::Save) {
        emit closeRequested();  // Save
        close();
    }
    */
    emit closeRequested();  // Save
    close();

}

void ScenarioBuilder::updateStatusLabel()
{
    QString status = "Selected: ";
    if (!selectedPiece.isEmpty()) status += selectedPiece;
    else status += "[No piece]";

    status += " → ";

    if (!selectedPosition.isEmpty()) status += selectedPosition;
    else status += "[No position]";

    status += " | Click Place to add to board";

    statusLabel->setText(status);
}

QString ScenarioBuilder::generateFEN()
{

    QString castling = "";
    if (castleK->isChecked()) castling += "K";
    if (castleQ->isChecked()) castling += "Q";
    if (castlek->isChecked()) castling += "k";
    if (castleq->isChecked()) castling += "q";

    QString enPassant = enPassantCombo->currentData().toString();
    if (enPassant.isEmpty()) enPassant = "-";

    return QString(" ") +
            (whiteTurn->isChecked() ? "w" : "b") + " " +
            (castling.isEmpty() ? "-" : castling) + " " +
            enPassant + " " +
            QString::number(halfmoveSpin->value()) + " " +
            QString::number(fullmoveSpin->value());
}

void ScenarioBuilder::parsePieceString(const QString& pieceStr, QString& color, QString& type)
{
    QStringList parts = pieceStr.split(' ');
    if (parts.size() >= 2) {
        color = parts[0];
        type = parts[1];
    }
}

QRadioButton *ScenarioBuilder::getWhiteTurn() const
{
    return whiteTurn;
}

void ScenarioBuilder::setBoard(ChessBoard *newBoard)
{
    board = newBoard;
}

void ScenarioBuilder::onTurnChanged(bool checked)
{

    emit turnChanged(whiteTurn->isChecked());
}
