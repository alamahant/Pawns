// chessengine.h
#ifndef CHESSENGINE_H
#define CHESSENGINE_H

#include <QObject>
#include <QString>

class ChessBoard;

class ChessEngine : public QObject {
    Q_OBJECT

public:
    explicit ChessEngine(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ChessEngine() = default;

    // ===== EXACTLY MATCH STOCKFISH =====
    virtual void setBoard(ChessBoard* board) = 0;
    virtual void setPosition(const QString& fen) = 0;
    virtual bool validateMove(const QString& from, const QString& to) = 0;
    virtual QString getBestMove(int thinkTime = 1000) = 0;
    virtual void setDifficulty(int level) = 0;
    virtual bool isPositionCheck(const QString& kingPos) = 0;
    virtual QString waitForResponse(const QString& expected) = 0;
    virtual void sendCommand(const QString& cmd) = 0;
    virtual void sendDirectCommand(const QString& cmd) = 0;
    virtual void updatePosition(const QString& from, const QString& to) = 0;
    virtual void stopThinking() = 0;

signals:
    void engineReady();
    void moveValidated(bool isValid);
    void bestMoveFound(const QString& move);
    void engineOutput(const QString& line);
};

#endif // CHESSENGINE_H
