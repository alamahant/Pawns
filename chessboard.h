#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include <QGraphicsScene>
#include <QColor>
#include "chesspiece.h"
#include"stockfishengine.h"
#include<QRegularExpression>
#include<QDialog>
#include<QButtonGroup>
#include<QRadioButton>
#include<QVBoxLayout>
#include<QPushButton>
#include<QGraphicsItemGroup>
#include<QGraphicsRectItem>
#include<QObject>
class StockfishEngine;

class ChessBoard : public QGraphicsScene {
    Q_OBJECT

public:
    explicit ChessBoard(QObject *parent = nullptr);
    void setStockfishEngine(StockfishEngine* engine);
    QString getCurrentFen() const;
    bool makeMove(const QString& from, const QString& to);
    //void applyEngineMove(const QString& move);
    bool applyEngineMove(const QString& move);

    ChessPiece* getSelectedPiece() const { return selectedPiece; }
    void setSelectedPiece(ChessPiece* piece) { selectedPiece = piece; }
    bool isWhiteTurn;
    QString getLastFormattedMove() const { return lastFormattedMove; }
    //castling

    bool isKingsideCastling(const QString& from, const QString& to);
    bool isQueensideCastling(const QString& from, const QString& to);
    void performCastling(const QString& from, const QString& to);
    bool canCastleKingside(ChessPiece::PieceColor color);
    bool canCastleQueenside(ChessPiece::PieceColor color);
    //en-passant
    QString enPassantTarget;  // Store the target square
    bool isEnPassantCapture(const QString& from, const QString& to);
    bool isPawnDoubleAdvance(const QString& from, const QString& to);

    StockfishEngine* getEngine() {return stockfishEngine;}
    //testing scenarios
    void setupPosition(const QString& fen);
    ChessPiece::PieceType getPieceType(QChar fenChar);
    StockfishEngine* getStockfishEngine() const { return stockfishEngine; }
    void setPlayerColor(bool isWhite) {
        playerIsWhite = isWhite;
    }
    bool playerIsWhite=true;
    void updateTurnStatus();  // Add this new method




public slots:
    void showPromotionDialog(ChessPiece::PieceColor color, const QString& square);


signals:
    void gameStatusChanged(const QString& status);
    void firstMoveMade();
private:
    void createSquares();
    void placePieces();

    QVector<QVector<ChessPiece*>> pieces;
    StockfishEngine* stockfishEngine;

    QString currentFen;
    QStringList moveHistory;
    void updateBoardState();
    ChessPiece* selectedPiece = nullptr;
    QString formatMove(const QString& from, const QString& to, ChessPiece* piece);
    QString lastFormattedMove;
    //check/mate

    bool isKingInCheck(ChessPiece::PieceColor color);
    bool isCheckmate(ChessPiece::PieceColor color);
    bool hasLegalMoves(ChessPiece::PieceColor color);
    QString findKingPosition(ChessPiece::PieceColor color);
    //pawn promo
    bool isPawnPromotion(const QString& from, const QString& to);
    void showPromotionDialog(ChessPiece::PieceColor color);
    void promotePawn(const QString& square, const QString& newPieceType);
public:
    void loadPieceImage(ChessPiece* piece);
    void generateFen();

    void highlightMove(const QString& from, const QString& to);
    void clearHighlights();
    QVector<QVector<ChessPiece *> > getPieces() const;
    void updateGameStatus();

    bool hasAnyPieceBeenDragged = false;
private:
    QGraphicsRectItem* sourceHighlight = nullptr;
    QGraphicsRectItem* destinationHighlight = nullptr;
    QTimer* highlightTimer = nullptr;
    bool lastMoveWasCapture = false;
    ChessPiece::PieceType lastCapturedType = ChessPiece::Pawn;
    QString lastCaptureSquare;
public:
    void setBoardFlipped(bool flipped);
    bool isFlipped = false;
    void dumpFen() const;
    QVector<QVector<ChessPiece*>>& getPiecesRef();
    const QVector<QVector<ChessPiece*>>& getPiecesRef() const;

    void placePieceOnBoard(ChessPiece* piece, int row, int col);
    void removePieceFromBoard(int row, int col);
    void clearBoard();

    // capture display
public:
    QStringList getCapturedWhiteList() const { return capturedWhiteList; }
    QStringList getCapturedBlackList() const { return capturedBlackList; }
    void addCapturedPiece(ChessPiece* piece);

private:
    QStringList capturedWhiteList;
    QStringList capturedBlackList;

public:
    bool isGameOver() const { return gameOver; }
    void setGameOver(bool over) { gameOver = over; }
    void reloadAllPieces();
    void redrawSquares();
    void redrawMarkings();
    void clearMarkings();

private:
    bool gameOver = false;
    QList<QGraphicsTextItem*> m_markings;

};

#endif // CHESSBOARD_H
