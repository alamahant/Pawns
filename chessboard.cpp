#include "chessboard.h"
#include <QGraphicsRectItem>
#include <QDebug>
#include<QDir>
#include<QTimer>
#include<QGraphicsView>
#include<QMessageBox>
#include<QApplication>
#include<QClipboard>
#include "Constants.h"



ChessBoard::ChessBoard(QObject *parent)
    : QGraphicsScene{parent}
    , stockfishEngine(nullptr)
    , isWhiteTurn(true)
{
    pieces.resize(8, QVector<ChessPiece*>(8, nullptr));
    createSquares();
    placePieces();
}


void ChessBoard::createSquares() {


    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            QGraphicsRectItem* square = new QGraphicsRectItem(
                        col * PawnConstants::SQUARE_SIZE,
                        row * PawnConstants::SQUARE_SIZE,
                        PawnConstants::SQUARE_SIZE,
                        PawnConstants::SQUARE_SIZE
                        );
            square->setBrush((row + col) % 2 ? PawnConstants::DARK_SQUARE : PawnConstants::LIGHT_SQUARE);
            square->setZValue(0);  //squares at bottom
            addItem(square);
        }
    }

    redrawMarkings();
}


void ChessBoard::placePieces() {
    pieces[0][0] = new ChessPiece(ChessPiece::Rook, ChessPiece::Black);
    pieces[0][1] = new ChessPiece(ChessPiece::Knight, ChessPiece::Black);
    pieces[0][2] = new ChessPiece(ChessPiece::Bishop, ChessPiece::Black);
    pieces[0][3] = new ChessPiece(ChessPiece::Queen, ChessPiece::Black);
    pieces[0][4] = new ChessPiece(ChessPiece::King, ChessPiece::Black);
    pieces[0][5] = new ChessPiece(ChessPiece::Bishop, ChessPiece::Black);
    pieces[0][6] = new ChessPiece(ChessPiece::Knight, ChessPiece::Black);
    pieces[0][7] = new ChessPiece(ChessPiece::Rook, ChessPiece::Black);

    for(int col = 0; col < 8; col++) {
        pieces[1][col] = new ChessPiece(ChessPiece::Pawn, ChessPiece::Black);
    }

    pieces[7][0] = new ChessPiece(ChessPiece::Rook, ChessPiece::White);
    pieces[7][1] = new ChessPiece(ChessPiece::Knight, ChessPiece::White);
    pieces[7][2] = new ChessPiece(ChessPiece::Bishop, ChessPiece::White);
    pieces[7][3] = new ChessPiece(ChessPiece::Queen, ChessPiece::White);
    pieces[7][4] = new ChessPiece(ChessPiece::King, ChessPiece::White);
    pieces[7][5] = new ChessPiece(ChessPiece::Bishop, ChessPiece::White);
    pieces[7][6] = new ChessPiece(ChessPiece::Knight, ChessPiece::White);
    pieces[7][7] = new ChessPiece(ChessPiece::Rook, ChessPiece::White);

    for(int col = 0; col < 8; col++) {
        pieces[6][col] = new ChessPiece(ChessPiece::Pawn, ChessPiece::White);

    }

    for(int row = 0; row < 8; row++) {
        for(int col = 0; col < 8; col++) {
            if(pieces[row][col]) {
                addItem(pieces[row][col]);
                pieces[row][col]->setPos(col * PawnConstants::SQUARE_SIZE, row * PawnConstants::SQUARE_SIZE);
                loadPieceImage(pieces[row][col]);
                QString position = QString(QChar('a' + col)) + QString::number(8 - row);
                pieces[row][col]->setInitialPosition(position);
            }
        }
    }
}


void ChessBoard::loadPieceImage(ChessPiece* piece) {
    QString type;
    switch (piece->getType()) {
    case ChessPiece::King:   type = "k"; break;
    case ChessPiece::Queen:  type = "q"; break;
    case ChessPiece::Rook:   type = "r"; break;
    case ChessPiece::Bishop: type = "b"; break;
    case ChessPiece::Knight: type = "n"; break;
    case ChessPiece::Pawn:   type = "p"; break;
    }

    QString colorLetter = (piece->getColor() == ChessPiece::White) ? "w" : "b";

    QString filename;
    if (PawnConstants::useDefaultPieces) {
        filename = ":/piecesets/default/" + colorLetter + type + ".png";
    } else {
        filename = PawnConstants::pieceSetDirectory + colorLetter + type + ".png";
    }

    QPixmap pixmap(filename);
    if (!pixmap.isNull()) {
        piece->setPixmap(pixmap.scaled(PawnConstants::SQUARE_SIZE, PawnConstants::SQUARE_SIZE,
                                       Qt::KeepAspectRatio, Qt::SmoothTransformation));
        piece->setZValue(1);  // pieces on top
    }
}

QVector<QVector<ChessPiece *> > ChessBoard::getPieces() const
{
    return pieces;
}


void ChessBoard::setStockfishEngine(StockfishEngine* engine) {
    stockfishEngine = engine;
    stockfishEngine->setBoard(this);
    generateFen();
}

QString ChessBoard::getCurrentFen() const {
    return currentFen;
}

void ChessBoard::generateFen() {
    QString fen;
    int emptyCount = 0;

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            ChessPiece* piece = pieces[row][col];
            if (piece) {
                if (emptyCount > 0) {
                    fen += QString::number(emptyCount);
                    emptyCount = 0;
                }
                fen += piece->getFenChar();
            } else {
                emptyCount++;
            }
        }
        if (emptyCount > 0) {
            fen += QString::number(emptyCount);
            emptyCount = 0;
        }
        if (row < 7) fen += '/';
    }

    fen += isWhiteTurn ? " w " : " b ";

    QString castlingRights;
    if (canCastleKingside(ChessPiece::White)) castlingRights += 'K';
    if (canCastleQueenside(ChessPiece::White)) castlingRights += 'Q';
    if (canCastleKingside(ChessPiece::Black)) castlingRights += 'k';
    if (canCastleQueenside(ChessPiece::Black)) castlingRights += 'q';
    fen += castlingRights.isEmpty() ? "-" : castlingRights;


    fen += " " + (enPassantTarget.isEmpty() ? "-" : enPassantTarget) + " 0 1";
    currentFen = fen;
    if (stockfishEngine) {
        stockfishEngine->setPosition(currentFen);
    }

}

bool ChessBoard::makeMove(const QString& from, const QString& to) {
    if (!stockfishEngine) {
        return false;
    }

    lastMoveWasCapture = false;
    lastCaptureSquare.clear();
    lastCapturedType = ChessPiece::Pawn;

    int fromCol = from[0].toLatin1() - 'a';
    int fromRow = '8' - from[1].toLatin1();
    int toCol = to[0].toLatin1() - 'a';
    int toRow = '8' - to[1].toLatin1();
    ChessPiece* movingPiece = pieces[fromRow][fromCol];

    if (isPawnDoubleAdvance(from, to)) {
        int enPassantRow = (fromRow + toRow) / 2;
        enPassantTarget = QString(from[0]) + QString::number(8 - enPassantRow);
    } else if (!isEnPassantCapture(from, to)) {
        enPassantTarget.clear();
    }

    if (isEnPassantCapture(from, to)) {
        emit gameStatusChanged("EnPassant Capture Performed!");

        lastMoveWasCapture = true;
        lastCapturedType = ChessPiece::Pawn;
        lastCaptureSquare = to;

        int actualToRow = (pieces[fromRow][fromCol]->getColor() == ChessPiece::White) ? toRow + 1 : toRow - 1;

        int capturedRow = fromRow;
        int capturedCol = toCol;
        ChessPiece* capturedPawn = pieces[capturedRow][capturedCol];
        this->removeItem(capturedPawn);
        delete capturedPawn;
        pieces[capturedRow][capturedCol] = nullptr;

        pieces[actualToRow][toCol] = pieces[fromRow][fromCol];
        pieces[fromRow][fromCol] = nullptr;
        pieces[actualToRow][toCol]->setPos(toCol * PawnConstants::SQUARE_SIZE,
                                           actualToRow * PawnConstants::SQUARE_SIZE);

        isWhiteTurn = !isWhiteTurn;
        generateFen();
        stockfishEngine->sendCommand("position fen " + currentFen);
        return true;
    }

    QString promotionTo = to;
    if (isPawnPromotion(from, to)) {
        promotionTo += "q";
    }

    if (stockfishEngine->validateMove(from, promotionTo)) {
        int toCol = to[0].toLatin1() - 'a';
        int toRow = '8' - to[1].toLatin1();

        if (isKingsideCastling(from, to) || isQueensideCastling(from, to)) {
            emit gameStatusChanged("Castling Performed!");
            performCastling(from, to);
        }

        if (pieces[toRow][toCol]) {
            ChessPiece* capturedPiece = pieces[toRow][toCol];
            addCapturedPiece(capturedPiece);
            lastMoveWasCapture = true;
            lastCapturedType = capturedPiece->getType();
            lastCaptureSquare = to;

            this->removeItem(capturedPiece);
            delete capturedPiece;
        }

        if (isPawnPromotion(from, to)) {
            emit gameStatusChanged("Pawn Promotion Performed!");
            ChessPiece* oldPiece = pieces[fromRow][fromCol];
            ChessPiece::PieceColor color = oldPiece->getColor();

            pieces[toRow][toCol] = pieces[fromRow][fromCol];
            pieces[fromRow][fromCol] = nullptr;
            pieces[toRow][toCol]->setPos(toCol * PawnConstants::SQUARE_SIZE, toRow * PawnConstants::SQUARE_SIZE);

            showPromotionDialog(color, to);
        } else {
            pieces[toRow][toCol] = pieces[fromRow][fromCol];
        }
        pieces[fromRow][fromCol] = nullptr;

        pieces[toRow][toCol]->setPos(toCol * PawnConstants::SQUARE_SIZE,
                                     toRow * PawnConstants::SQUARE_SIZE);
        pieces[toRow][toCol]->setInitialPosition(to);

        QString formattedMove = formatMove(from, to, pieces[toRow][toCol]);
        moveHistory.append(formattedMove);
        lastFormattedMove = formattedMove;

        isWhiteTurn = !isWhiteTurn;
        generateFen();
        updateGameStatus();

        emit firstMoveMade();
        return true;
    }

    movingPiece->returnToOriginalPosition();
    return false;
}


/*
void ChessBoard::applyEngineMove(const QString& move) {

    if (move.length() < 4) {
        qWarning() << "❌ Invalid move:" << move;
        return;
    }

    lastMoveWasCapture = false;
    lastCaptureSquare.clear();
    lastCapturedType = ChessPiece::Pawn;

    QString from = move.mid(0, 2);
    QString to = move.mid(2, 2);



    int fromCol = from[0].toLatin1() - 'a';
    int fromRow = '8' - from[1].toLatin1();
    int toCol = to[0].toLatin1() - 'a';
    int toRow = '8' - to[1].toLatin1();

    ChessPiece* movingPiece = pieces[fromRow][fromCol];

    if (isPawnDoubleAdvance(from, to)) {
        int enPassantRow = (fromRow + toRow) / 2;
        enPassantTarget = QString(from[0]) + QString::number(8 - enPassantRow);
    } else if (!isEnPassantCapture(from, to)) {
        enPassantTarget.clear();
    }


    if (isEnPassantCapture(from, to)) {
        emit gameStatusChanged("(engine) EnPassant Capture Performed!");

        lastMoveWasCapture = true;
        lastCapturedType = ChessPiece::Pawn;
        lastCaptureSquare = to;

        int actualToRow = (pieces[fromRow][fromCol]->getColor() == ChessPiece::White) ? toRow + 1 : toRow - 1;

        int capturedRow = fromRow;
        int capturedCol = toCol;
        ChessPiece* capturedPawn = pieces[capturedRow][capturedCol];
        this->removeItem(capturedPawn);
        delete capturedPawn;
        pieces[capturedRow][capturedCol] = nullptr;

        pieces[actualToRow][toCol] = pieces[fromRow][fromCol];
        pieces[fromRow][fromCol] = nullptr;
        pieces[actualToRow][toCol]->setPos(toCol * PawnConstants::SQUARE_SIZE,
                                           actualToRow * PawnConstants::SQUARE_SIZE);


        isWhiteTurn = !isWhiteTurn;
        generateFen();
        stockfishEngine->sendCommand("position fen " + currentFen);
        stockfishEngine->sendCommand("go movetime 1000");
        return;
    }

    if (isKingsideCastling(from, to) || isQueensideCastling(from, to)) {
        emit gameStatusChanged("(engine) Castling Performed!");
        performCastling(from, to);
    }

    if (pieces[toRow][toCol]) {
        ChessPiece* capturedPiece = pieces[toRow][toCol];
        addCapturedPiece(capturedPiece);
        lastMoveWasCapture = true;
        lastCapturedType = capturedPiece->getType();
        lastCaptureSquare = to;

        this->removeItem(capturedPiece);
        delete capturedPiece;
    }
    if (move.length() > 4) {
        emit gameStatusChanged("(engine) Pawn Promotion Performed!");
        ChessPiece::PieceType newType;
        switch(move[4].toLatin1()) {
        case 'q': newType = ChessPiece::Queen; break;
        case 'r': newType = ChessPiece::Rook; break;
        case 'b': newType = ChessPiece::Bishop; break;
        case 'n': newType = ChessPiece::Knight; break;
        default: newType = ChessPiece::Queen; break;
        }

        ChessPiece::PieceColor promoColor = movingPiece->getColor();
        if (movingPiece) {
            removeItem(movingPiece);
            delete movingPiece;
            movingPiece = nullptr;
        }
        pieces[toRow][toCol] = new ChessPiece(newType, promoColor);  // ← USE promoColor
        addItem(pieces[toRow][toCol]);
        loadPieceImage(pieces[toRow][toCol]);
        pieces[toRow][toCol]->setPos(toCol * PawnConstants::SQUARE_SIZE, toRow * PawnConstants::SQUARE_SIZE);

        pieces[toRow][toCol]->setInitialPosition(to);
    } else {
        pieces[toRow][toCol] = pieces[fromRow][fromCol];
    }

    pieces[fromRow][fromCol] = nullptr;
    pieces[toRow][toCol]->setPos(toCol * PawnConstants::SQUARE_SIZE,
                                 toRow * PawnConstants::SQUARE_SIZE);
    pieces[toRow][toCol]->setInitialPosition(to);

    lastFormattedMove = formatMove(from, to, pieces[toRow][toCol]);

    isWhiteTurn = !isWhiteTurn;
    generateFen();
    updateGameStatus();
}
*/

bool ChessBoard::applyEngineMove(const QString& move) {
    if (move.length() < 4) {
        qWarning() << "❌ Invalid move:" << move;
        return false;
    }

    lastMoveWasCapture = false;
    lastCaptureSquare.clear();
    lastCapturedType = ChessPiece::Pawn;

    QString from = move.mid(0, 2);
    QString to = move.mid(2, 2);

    int fromCol = from[0].toLatin1() - 'a';
    int fromRow = '8' - from[1].toLatin1();
    int toCol = to[0].toLatin1() - 'a';
    int toRow = '8' - to[1].toLatin1();

    ChessPiece* movingPiece = pieces[fromRow][fromCol];

    // ===== CHECK IF PIECE EXISTS =====
    if (!movingPiece) {
        qWarning() << "❌ No piece at from square:" << from;
        return false;
    }

    if (isPawnDoubleAdvance(from, to)) {
        int enPassantRow = (fromRow + toRow) / 2;
        enPassantTarget = QString(from[0]) + QString::number(8 - enPassantRow);
    } else if (!isEnPassantCapture(from, to)) {
        enPassantTarget.clear();
    }

    if (isEnPassantCapture(from, to)) {
        emit gameStatusChanged("(engine) EnPassant Capture Performed!");

        lastMoveWasCapture = true;
        lastCapturedType = ChessPiece::Pawn;
        lastCaptureSquare = to;

        int actualToRow = (pieces[fromRow][fromCol]->getColor() == ChessPiece::White) ? toRow + 1 : toRow - 1;

        int capturedRow = fromRow;
        int capturedCol = toCol;
        ChessPiece* capturedPawn = pieces[capturedRow][capturedCol];
        this->removeItem(capturedPawn);
        delete capturedPawn;
        pieces[capturedRow][capturedCol] = nullptr;

        pieces[actualToRow][toCol] = pieces[fromRow][fromCol];
        pieces[fromRow][fromCol] = nullptr;
        pieces[actualToRow][toCol]->setPos(toCol * PawnConstants::SQUARE_SIZE,
                                           actualToRow * PawnConstants::SQUARE_SIZE);

        isWhiteTurn = !isWhiteTurn;
        generateFen();
        stockfishEngine->sendCommand("position fen " + currentFen);
        stockfishEngine->sendCommand("go movetime 1000");
        return true;
    }

    if (isKingsideCastling(from, to) || isQueensideCastling(from, to)) {
        emit gameStatusChanged("(engine) Castling Performed!");
        performCastling(from, to);
    }

    if (pieces[toRow][toCol]) {
        ChessPiece* capturedPiece = pieces[toRow][toCol];
        addCapturedPiece(capturedPiece);
        lastMoveWasCapture = true;
        lastCapturedType = capturedPiece->getType();
        lastCaptureSquare = to;

        this->removeItem(capturedPiece);
        delete capturedPiece;
    }

    if (move.length() > 4) {
        emit gameStatusChanged("(engine) Pawn Promotion Performed!");
        ChessPiece::PieceType newType;
        switch(move[4].toLatin1()) {
        case 'q': newType = ChessPiece::Queen; break;
        case 'r': newType = ChessPiece::Rook; break;
        case 'b': newType = ChessPiece::Bishop; break;
        case 'n': newType = ChessPiece::Knight; break;
        default: newType = ChessPiece::Queen; break;
        }

        ChessPiece::PieceColor promoColor = movingPiece->getColor();
        if (movingPiece) {
            removeItem(movingPiece);
            delete movingPiece;
            movingPiece = nullptr;
        }
        pieces[toRow][toCol] = new ChessPiece(newType, promoColor);
        addItem(pieces[toRow][toCol]);
        loadPieceImage(pieces[toRow][toCol]);
        pieces[toRow][toCol]->setPos(toCol * PawnConstants::SQUARE_SIZE, toRow * PawnConstants::SQUARE_SIZE);
        pieces[toRow][toCol]->setInitialPosition(to);
    } else {
        pieces[toRow][toCol] = pieces[fromRow][fromCol];
    }

    pieces[fromRow][fromCol] = nullptr;
    pieces[toRow][toCol]->setPos(toCol * PawnConstants::SQUARE_SIZE,
                                 toRow * PawnConstants::SQUARE_SIZE);
    pieces[toRow][toCol]->setInitialPosition(to);

    lastFormattedMove = formatMove(from, to, pieces[toRow][toCol]);

    isWhiteTurn = !isWhiteTurn;
    generateFen();
    updateGameStatus();

    return true;
}

void ChessBoard::updateBoardState() {
    ChessPiece* piece = getSelectedPiece();
    QString from = piece->getOriginalPosition();
    QString to = piece->getPosition();

    int fromCol = from[0].toLatin1() - 'a';
    int fromRow = '8' - from[1].toLatin1();
    int toCol = to[0].toLatin1() - 'a';
    int toRow = '8' - to[1].toLatin1();

    pieces[toRow][toCol] = pieces[fromRow][fromCol];
    pieces[fromRow][fromCol] = nullptr;
    piece->setInitialPosition(to);

}


QString ChessBoard::formatMove(const QString& from, const QString& to, ChessPiece* piece) {
    static const QMap<ChessPiece::PieceType, QString> pieceNames = {
        {ChessPiece::King, "King"},
        {ChessPiece::Queen, "Queen"},
        {ChessPiece::Rook, "Rook"},
        {ChessPiece::Bishop, "Bishop"},
        {ChessPiece::Knight, "Knight"},
        {ChessPiece::Pawn, "Pawn"}
    };

    QString color = piece->getColor() == ChessPiece::White ? "White" : "Black";
    QString pieceName = pieceNames[piece->getType()];

    if (lastMoveWasCapture && !lastCaptureSquare.isEmpty()) {
        QString capturedName;
        if (pieceNames.contains(lastCapturedType)) {
            capturedName = pieceNames[lastCapturedType];
        } else {
            capturedName = "Unknown";
        }
        return QString("%1 %2 moves from %3 to %4 captures %5 %6")
                .arg(color)
                .arg(pieceName)
                .arg(from)
                .arg(to)
                .arg((color == "Black") ? "White" : "Black")
                .arg(capturedName);
    } else {
        return QString("%1 %2 moves from %3 to %4")
                .arg(color)
                .arg(pieceName)
                .arg(from)
                .arg(to);
    }
}

QString ChessBoard::findKingPosition(ChessPiece::PieceColor color) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (pieces[row][col] &&
                    pieces[row][col]->getType() == ChessPiece::King &&
                    pieces[row][col]->getColor() == color) {
                return QString(QChar('a' + col)) + QString::number(8 - row);
            }
        }
    }
    return QString();
}
bool ChessBoard::isKingInCheck(ChessPiece::PieceColor color) {
    QString kingPos = findKingPosition(color);
    return stockfishEngine->isPositionCheck(kingPos);
}



bool ChessBoard::isCheckmate(ChessPiece::PieceColor color) {
    if (!isKingInCheck(color)) {
        return false;
    }

    stockfishEngine->sendCommand("position fen " + getCurrentFen());
    stockfishEngine->sendCommand("go perft 1");
    QString response = stockfishEngine->waitForResponse("Nodes");

    return response.contains("Nodes searched: 0");
}



void ChessBoard::updateGameStatus() {
    ChessPiece::PieceColor currentColor = isWhiteTurn ? ChessPiece::White : ChessPiece::Black;

    if (isKingInCheck(currentColor)) {
        bool checkmate = isCheckmate(currentColor);

        if (checkmate) {
            QString winner = currentColor == ChessPiece::White ? "Black" : "White";
            emit gameStatusChanged("Checkmate! " + winner + " wins!");
            if (stockfishEngine) {
                stockfishEngine->sendCommand("stop");
                stockfishEngine->stopThinking();
            }
            gameOver = true;
        } else {
            emit gameStatusChanged("Check!");
        }
    } else {
        emit gameStatusChanged("");
    }
}



bool ChessBoard::hasLegalMoves(ChessPiece::PieceColor color) {
    if (!stockfishEngine) {
        return false;
    }

    stockfishEngine->sendCommand("go perft 1");
    QString response = stockfishEngine->waitForResponse("Nodes");

    return response.contains("Nodes") && !response.contains("Nodes 0");
}



void ChessBoard::showPromotionDialog(ChessPiece::PieceColor color, const QString& square) {
    QDialog dialog;
    QVBoxLayout layout(&dialog);
    QButtonGroup group;
    QStringList pieces = {"Queen", "Rook", "Bishop", "Knight"};

    for (const QString& piece : pieces) {
        QRadioButton* button = new QRadioButton(piece);
        group.addButton(button);
        layout.addWidget(button);
    }

    QPushButton* okButton = new QPushButton("OK");
    layout.addWidget(okButton);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted) {
        QRadioButton* selected = qobject_cast<QRadioButton*>(group.checkedButton());
        promotePawn(square, selected->text());
    }
}

void ChessBoard::promotePawn(const QString& square, const QString& pieceType) {
    int col = square[0].toLatin1() - 'a';
    int row = '8' - square[1].toLatin1();

    ChessPiece::PieceType newType;
    if (pieceType == "Queen") newType = ChessPiece::Queen;
    else if (pieceType == "Rook") newType = ChessPiece::Rook;
    else if (pieceType == "Bishop") newType = ChessPiece::Bishop;
    else if (pieceType == "Knight") newType = ChessPiece::Knight;
    else newType = ChessPiece::Queen;

    ChessPiece* oldPiece = pieces[row][col];
    ChessPiece::PieceColor color = oldPiece->getColor();

    delete oldPiece;
    pieces[row][col] = new ChessPiece(newType, color);
    addItem(pieces[row][col]);
    loadPieceImage(pieces[row][col]);
    pieces[row][col]->setPos(col * PawnConstants::SQUARE_SIZE, row * PawnConstants::SQUARE_SIZE);

    //setBoardFlipped(isFlipped);  // ← This will rotate everything

    generateFen();
}


bool ChessBoard::isPawnPromotion(const QString& from, const QString& to) {
    int fromRow = '8' - from[1].toLatin1();
    int fromCol = from[0].toLatin1() - 'a';

    ChessPiece* piece = pieces[fromRow][fromCol];
    if (!piece || piece->getType() != ChessPiece::Pawn) {
        return false;
    }

    int toRow = '8' - to[1].toLatin1();


    return (piece->getColor() == ChessPiece::White && toRow == 0) ||
            (piece->getColor() == ChessPiece::Black && toRow == 7);
}




void ChessBoard::performCastling(const QString& from, const QString& to) {
    int fromRow = '8' - from[1].toLatin1();
    int rookFromCol = (to[0] == 'g') ? 7 : 0;
    int rookToCol = (to[0] == 'g') ? 5 : 3;


    pieces[fromRow][rookToCol] = pieces[fromRow][rookFromCol];
    pieces[fromRow][rookFromCol] = nullptr;

    pieces[fromRow][rookToCol]->setPos(rookToCol * PawnConstants::SQUARE_SIZE,
                                       fromRow * PawnConstants::SQUARE_SIZE);
    QString rookTo = QString(QChar('a' + rookToCol)) + from[1];
    pieces[fromRow][rookToCol]->setInitialPosition(rookTo);

}



bool ChessBoard::isEnPassantCapture(const QString& from, const QString& to) {

    int fromCol = from[0].toLatin1() - 'a';
    int fromRow = '8' - from[1].toLatin1();
    ChessPiece* piece = pieces[fromRow][fromCol];

    if (!piece || piece->getType() != ChessPiece::Pawn) {
        return false;
    }


    return (to == enPassantTarget);
}


/*
bool ChessBoard::isPawnDoubleAdvance(const QString& from, const QString& to) {
    ChessPiece* piece = pieces[from[1].digitValue() - 1][from[0].toLatin1() - 'a'];
    return piece &&
           piece->getType() == ChessPiece::Pawn &&
           abs(from[1].digitValue() - to[1].digitValue()) == 2;
}
*/

bool ChessBoard::isPawnDoubleAdvance(const QString& from, const QString& to) {
    int fromRow = '8' - from[1].toLatin1();  // "e2" → 7-2 = 6? Wait...
    int fromCol = from[0].toLatin1() - 'a';
    ChessPiece* piece = pieces[fromRow][fromCol];

    return piece &&
            piece->getType() == ChessPiece::Pawn &&
            abs(from[1].digitValue() - to[1].digitValue()) == 2;
}

bool ChessBoard::canCastleKingside(ChessPiece::PieceColor color) {
    int row = (color == ChessPiece::White) ? 7 : 0;

    ChessPiece* king = pieces[row][4];
    ChessPiece* rook = pieces[row][7];

    return king && king->getType() == ChessPiece::King &&
            rook && rook->getType() == ChessPiece::Rook;
}

bool ChessBoard::canCastleQueenside(ChessPiece::PieceColor color) {
    int row = (color == ChessPiece::White) ? 7 : 0;

    ChessPiece* king = pieces[row][4];
    ChessPiece* rook = pieces[row][0];

    return king && king->getType() == ChessPiece::King &&
            rook && rook->getType() == ChessPiece::Rook;
}

void ChessBoard::setupPosition(const QString& fen) {
    for(int row = 0; row < 8; row++) {
        for(int col = 0; col < 8; col++) {
            if(pieces[row][col]) {
                delete pieces[row][col];
                pieces[row][col] = nullptr;
            }
        }
    }

    QStringList fenParts = fen.split(' ');


    QString position = fenParts[0];

    int row = 0;
    int col = 0;
    for(QChar c : position) {
        if(c == '/') {
            row++;
            col = 0;
        } else if(c.isDigit()) {
            col += c.digitValue();
        } else {
            pieces[row][col] = new ChessPiece(getPieceType(c),
                                              c.isUpper() ? ChessPiece::White : ChessPiece::Black);
            addItem(pieces[row][col]);
            loadPieceImage(pieces[row][col]);
            pieces[row][col]->setPos(col * PawnConstants::SQUARE_SIZE,
                                     row * PawnConstants::SQUARE_SIZE);
            QString pos = QString(QChar('a' + col)) + QString::number(8 - row);
            pieces[row][col]->setInitialPosition(pos);
            col++;
        }
    }

    isWhiteTurn = (fenParts[1] == "w");

    if (fenParts.size() > 3) {
        enPassantTarget = fenParts[3] != "-" ? fenParts[3] : QString();
    }

    currentFen = fen;
    if(stockfishEngine) {
        stockfishEngine->setPosition(fen);
        stockfishEngine->sendCommand("isready");
        stockfishEngine->sendCommand("go perft 1");  // Add this line to check legal moves
    }
}




ChessPiece::PieceType ChessBoard::getPieceType(QChar fenChar) {
    switch(fenChar.toLower().toLatin1()) {
    case 'p': return ChessPiece::Pawn;
    case 'n': return ChessPiece::Knight;
    case 'b': return ChessPiece::Bishop;
    case 'r': return ChessPiece::Rook;
    case 'q': return ChessPiece::Queen;
    case 'k': return ChessPiece::King;
    default: return ChessPiece::Pawn;
    }
}


void ChessBoard::updateTurnStatus() {
    QString turnText;
    if (playerIsWhite) {
        turnText = isWhiteTurn ? "White to move" : "Black (Engine) to move";
    } else {
        turnText = isWhiteTurn ? "White (Engine) to move" : "Black to move";
    }
    emit gameStatusChanged(turnText);
}


void ChessBoard::highlightMove(const QString& from, const QString& to) {
    clearHighlights();

    int fromCol = from[0].toLatin1() - 'a';
    int fromRow = '8' - from[1].toLatin1();
    int toCol = to[0].toLatin1() - 'a';
    int toRow = '8' - to[1].toLatin1();

    sourceHighlight = new QGraphicsRectItem(
                fromCol * PawnConstants::SQUARE_SIZE,
                fromRow * PawnConstants::SQUARE_SIZE,
                PawnConstants::SQUARE_SIZE,
                PawnConstants::SQUARE_SIZE
                );
    sourceHighlight->setBrush(QBrush(QColor(0, 255, 0, 100))); // Green with transparency
    sourceHighlight->setPen(QPen(Qt::NoPen));
    addItem(sourceHighlight);

    destinationHighlight = new QGraphicsRectItem(
                toCol * PawnConstants::SQUARE_SIZE,
                toRow * PawnConstants::SQUARE_SIZE,
                PawnConstants::SQUARE_SIZE,
                PawnConstants::SQUARE_SIZE
                );
    destinationHighlight->setBrush(QBrush(QColor(0, 255, 0, 100))); // Green with transparency

    destinationHighlight->setPen(QPen(Qt::NoPen));
    addItem(destinationHighlight);

    if (!highlightTimer) {
        highlightTimer = new QTimer(this);
        highlightTimer->setSingleShot(true);
        connect(highlightTimer, &QTimer::timeout, this, &ChessBoard::clearHighlights);
    }

    highlightTimer->start(3000); // 3.5 seconds
}

void ChessBoard::clearHighlights() {
    if (sourceHighlight) {
        removeItem(sourceHighlight);
        delete sourceHighlight;
        sourceHighlight = nullptr;
    }

    if (destinationHighlight) {
        removeItem(destinationHighlight);
        delete destinationHighlight;
        destinationHighlight = nullptr;
    }

    if (highlightTimer) {
        highlightTimer->stop();
    }
}


void ChessBoard::setBoardFlipped(bool flipped)
{
    //if (isFlipped == flipped) return;

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (pieces[row][col]) {
                pieces[row][col]->setTransformOriginPoint(
                            pieces[row][col]->boundingRect().center()
                            );
                pieces[row][col]->setRotation(flipped ? 180.0 : 0.0);
            }
        }
    }
    QList<QGraphicsItem*> items = this->items();
    for (QGraphicsItem* item : items) {
        QGraphicsTextItem* textItem = dynamic_cast<QGraphicsTextItem*>(item);
        if (textItem) {
            textItem->setTransformOriginPoint(textItem->boundingRect().center());
            textItem->setRotation(flipped ? 180.0 : 0.0);
        }
    }

    QGraphicsView* view = qobject_cast<QGraphicsView*>(views().first());
    if (view) {
        if (flipped) {
            view->setTransform(QTransform::fromScale(-1, -1));
            view->centerOn(width() / 2, height() / 2);
        } else {
            view->setTransform(QTransform());
            view->centerOn(0, 0);
        }
    }
    //isFlipped = flipped;
}

void ChessBoard::dumpFen() const {
    QString details = QString(
                "FEN: %1\n"
                "Turn: %2\n"
                "Player color: %3\n"
                "Board flipped: %4"
                ).arg(currentFen)
            .arg(isWhiteTurn ? "White" : "Black")
            .arg(playerIsWhite ? "White" : "Black")
            .arg(!PawnConstants::startDialogPlayer1IsWhite ? "Yes" : "No");

    QMessageBox msgBox;
    msgBox.setWindowTitle("FEN Dump");
    msgBox.setText(details);
    msgBox.setMinimumWidth(400);

    QPushButton* copyButton = msgBox.addButton("Copy FEN", QMessageBox::ActionRole);
    QPushButton* closeButton = msgBox.addButton("Close", QMessageBox::RejectRole);

    msgBox.exec();

    if (msgBox.clickedButton() == copyButton) {
        QApplication::clipboard()->setText(currentFen);

        QMessageBox::information(nullptr, "Copied!",
                                 "FEN copied to clipboard!",
                                 QMessageBox::Ok);
    }
}



bool ChessBoard::isKingsideCastling(const QString& from, const QString& to) {
    int fromRow = '8' - from[1].toLatin1();
    int fromCol = from[0].toLatin1() - 'a';
    ChessPiece* piece = pieces[fromRow][fromCol];


    return (piece && piece->getType() == ChessPiece::King &&
            from == "e1" && to == "g1") ||  // White kingside
            (piece && piece->getType() == ChessPiece::King &&
             from == "e8" && to == "g8");    // Black kingside - ADD piece check!
}

bool ChessBoard::isQueensideCastling(const QString& from, const QString& to) {
    int fromRow = '8' - from[1].toLatin1();
    int fromCol = from[0].toLatin1() - 'a';
    ChessPiece* piece = pieces[fromRow][fromCol];


    return (piece && piece->getType() == ChessPiece::King &&
            from == "e1" && to == "c1") ||  // White queenside
            (piece && piece->getType() == ChessPiece::King &&
             from == "e8" && to == "c8");    // Black queenside - ADD piece check!
}


QVector<QVector<ChessPiece*>>& ChessBoard::getPiecesRef()
{
    return pieces;
}

const QVector<QVector<ChessPiece*>>& ChessBoard::getPiecesRef() const
{
    return pieces;
}

void ChessBoard::placePieceOnBoard(ChessPiece* piece, int row, int col)
{
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return;
    pieces[row][col] = piece;
}

void ChessBoard::removePieceFromBoard(int row, int col)
{
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return;
    if (pieces[row][col]) {
        removeItem(pieces[row][col]);
        delete pieces[row][col];
        pieces[row][col] = nullptr;
    }
}

void ChessBoard::clearBoard()
{
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (pieces[row][col]) {
                removeItem(pieces[row][col]);
                delete pieces[row][col];
                pieces[row][col] = nullptr;
            }
        }
    }

    isWhiteTurn = true;
    enPassantTarget.clear();
    currentFen = "";

    generateFen();
}


void ChessBoard::addCapturedPiece(ChessPiece* piece)
{
    if (!piece) return;

    QString key;
    if (piece->getColor() == ChessPiece::White) {
        key = "w";
    } else {
        key = "b";
    }

    switch(piece->getType()) {
    case ChessPiece::King:   key += "k"; break;
    case ChessPiece::Queen:  key += "q"; break;
    case ChessPiece::Rook:   key += "r"; break;
    case ChessPiece::Bishop: key += "b"; break;
    case ChessPiece::Knight: key += "n"; break;
    case ChessPiece::Pawn:   key += "p"; break;
    default: key += "?";
    }

    if (piece->getColor() == ChessPiece::White) {
        capturedWhiteList.append(key);
    } else {
        capturedBlackList.append(key);
    }
}

void ChessBoard::reloadAllPieces() {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (pieces[row][col]) {
                loadPieceImage(pieces[row][col]);
            }
        }
    }
}

void ChessBoard::redrawSquares() {
    for (auto item : items()) {
        if (dynamic_cast<QGraphicsRectItem*>(item)) {
            removeItem(item);
            delete item;
        }
    }
    createSquares();
}


void ChessBoard::clearMarkings() {
    for (auto item : m_markings) {
        removeItem(item);
        delete item;
    }
    m_markings.clear();
}

void ChessBoard::redrawMarkings() {
    clearMarkings();

    if (!PawnConstants::showBoardMarkings) return;

    for (int row = 0; row < 8; row++) {
        QGraphicsTextItem* rankLabel = new QGraphicsTextItem(QString::number(8 - row));
        rankLabel->setDefaultTextColor(Qt::black);
        rankLabel->setPos(-20, row * PawnConstants::SQUARE_SIZE + PawnConstants::SQUARE_SIZE/3);
        addItem(rankLabel);
        m_markings.append(rankLabel);
    }

    for (int row = 0; row < 8; row++) {
        QGraphicsTextItem* rankLabel = new QGraphicsTextItem(QString::number(8 - row));
        rankLabel->setDefaultTextColor(Qt::black);
        rankLabel->setPos(8 * PawnConstants::SQUARE_SIZE + 5, row * PawnConstants::SQUARE_SIZE + PawnConstants::SQUARE_SIZE/3);
        addItem(rankLabel);
        m_markings.append(rankLabel);
    }

    for (int col = 0; col < 8; col++) {
        QGraphicsTextItem* fileLabel = new QGraphicsTextItem(QString(QChar('A' + col)));
        fileLabel->setDefaultTextColor(Qt::black);
        fileLabel->setPos(col * PawnConstants::SQUARE_SIZE + PawnConstants::SQUARE_SIZE/3,
                          8 * PawnConstants::SQUARE_SIZE + 5);
        addItem(fileLabel);
        m_markings.append(fileLabel);
    }

    for (int col = 0; col < 8; col++) {
        QGraphicsTextItem* fileLabel = new QGraphicsTextItem(QString(QChar('A' + col)));
        fileLabel->setDefaultTextColor(Qt::black);
        fileLabel->setPos(col * PawnConstants::SQUARE_SIZE + PawnConstants::SQUARE_SIZE/3,
                          -30);
        addItem(fileLabel);
        m_markings.append(fileLabel);
    }

    if (!PawnConstants::startDialogPlayer1IsWhite) {
        for (auto item : m_markings) {
            item->setTransformOriginPoint(item->boundingRect().center());
            item->setRotation(180.0);
        }
    }
}
