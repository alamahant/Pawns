#ifndef CHESSPIECE_H
#define CHESSPIECE_H

#include <QGraphicsPixmapItem>

class ChessPiece: public QGraphicsPixmapItem {

public:
    enum PieceType { King, Queen, Rook, Bishop, Knight, Pawn };
    enum PieceColor { White, Black };

    ChessPiece(PieceType type, PieceColor color, QGraphicsItem *parent = nullptr);

    PieceType getType() const;
    PieceColor getColor() const;
    QString getPosition() const;
    char getFenChar() const;
    PieceType type;
    PieceColor color;
    QString currentPosition;

    void setInitialPosition(const QString& pos);

    void returnToOriginalPosition();

    QString getOriginalPosition() const { return originalPosition; }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:

    bool isDragging;
    QPointF dragStart;
    QString originalPosition;
    QPointF dragStartPos;
    //
    QPointF originalPos;

};

#endif // CHESSPIECE_H
