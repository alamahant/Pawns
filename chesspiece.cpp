#include "chesspiece.h"
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include "chessboard.h"
#include"Constants.h"


ChessPiece::ChessPiece(PieceType type, PieceColor color, QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
    , type(type)
    , color(color)
    , isDragging(false)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
}

/*
void ChessPiece::mousePressEvent(QGraphicsSceneMouseEvent *event) {

    ChessBoard* board = static_cast<ChessBoard*>(scene());
    if(!board) return;



    if (board->hasAnyPieceBeenDragged) {
        return;
    }

    bool isWhitePiece = (color == ChessPiece::White);
    if (isWhitePiece != board->isWhiteTurn) {
        return;  // Not your turn / opponent's piece
    }

    if (board->playerIsWhite && color == ChessPiece::Black) {
        return;
    }

    if (!board->playerIsWhite && color == ChessPiece::White) {
        return;
    }

    board->setSelectedPiece(this);

    dragStart = pos();
    originalPosition = currentPosition;
    isDragging = true;
    QGraphicsPixmapItem::mousePressEvent(event);
}
*/


void ChessPiece::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    ChessBoard* board = static_cast<ChessBoard*>(scene());
    if(!board) return;

    if (board->hasAnyPieceBeenDragged) {
        return;
    }

    bool isWhitePiece = (color == ChessPiece::White);

    if (isWhitePiece != board->isWhiteTurn) {
        return;  // Not your turn / opponent's piece
    }

    if (PawnConstants::isH2HPlay) {
        board->setSelectedPiece(this);
        dragStart = pos();
        originalPosition = currentPosition;
        isDragging = true;
        QGraphicsPixmapItem::mousePressEvent(event);
        return;
    }

    if (board->playerIsWhite && color == ChessPiece::Black) {
        return;
    }

    if (!board->playerIsWhite && color == ChessPiece::White) {
        return;
    }

    board->setSelectedPiece(this);
    dragStart = pos();
    originalPosition = currentPosition;
    isDragging = true;
    QGraphicsPixmapItem::mousePressEvent(event);
}

void ChessPiece::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (isDragging) {
        QPointF newPos = event->scenePos();
        setPos(newPos - event->buttonDownScenePos(Qt::LeftButton) + dragStart);
        QGraphicsPixmapItem::mouseMoveEvent(event);
    }
}


void ChessPiece::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (!isDragging) return;
    isDragging = false;

    int col = qBound(0, static_cast<int>(round(pos().x() / PawnConstants::SQUARE_SIZE)), 7);
    int row = qBound(0, static_cast<int>(round(pos().y() / PawnConstants::SQUARE_SIZE)), 7);

    QPointF newPos(col * PawnConstants::SQUARE_SIZE, row * PawnConstants::SQUARE_SIZE);
    setPos(newPos);

    QString newPosition = QString(QChar('a' + col)) + QString::number(8 - row);

    if (newPosition != currentPosition) {
        currentPosition = newPosition;

        ChessBoard* board = static_cast<ChessBoard*>(scene());
        if (board) {
            board->hasAnyPieceBeenDragged = true;   // Only set on actual move
        }
    }

    QGraphicsPixmapItem::mouseReleaseEvent(event);
}

char ChessPiece::getFenChar() const {
    char piece;
    switch(type) {
    case King:   piece = 'k'; break;
    case Queen:  piece = 'q'; break;
    case Rook:   piece = 'r'; break;
    case Bishop: piece = 'b'; break;
    case Knight: piece = 'n'; break;
    case Pawn:   piece = 'p'; break;
    default:     piece = '?'; break;
    }
    return color == White ? toupper(piece) : piece;
}

ChessPiece::PieceType ChessPiece::getType() const {
    return type;
}

ChessPiece::PieceColor ChessPiece::getColor() const {
    return color;
}

QString ChessPiece::getPosition() const {
    return currentPosition;
}

void ChessPiece::setInitialPosition(const QString& pos) {
    currentPosition = pos;
    originalPosition = pos;
    dragStartPos = this->pos();
    ChessBoard* board = static_cast<ChessBoard*>(scene());
    if (!board) return;
    board->hasAnyPieceBeenDragged = false;   //  Reset
}

void ChessPiece::returnToOriginalPosition() {
    setPos(dragStartPos);
    currentPosition = originalPosition;
    ChessBoard* board = static_cast<ChessBoard*>(scene());
    if (!board) return;
    board->hasAnyPieceBeenDragged = false;   //  Reset

}
