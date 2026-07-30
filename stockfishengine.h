#ifndef STOCKFISHENGINE_H
#define STOCKFISHENGINE_H

#include <QObject>
#include<QProcess>

class ChessBoard;  // Forward declaration

class StockfishEngine : public QObject {
    Q_OBJECT

public:
    explicit StockfishEngine(QObject* parent = nullptr);
    ~StockfishEngine();

    void setBoard(ChessBoard* board);
    void setPosition(const QString& fen);
    bool validateMove(const QString& from, const QString& to);
    QString getBestMove(int thinkTime = 1000);
    void setDifficulty(int level);

    static QString toUCI(const QString& from, const QString& to);
    static void fromUCI(const QString& uci, QString& from, QString& to);
    bool isPositionCheck(const QString& kingPos);
    QString waitForResponse(const QString& expected);
    void sendCommand(const QString& cmd);
    void updatePosition(const QString& from, const QString& to);

signals:
    void engineReady();
    void moveValidated(bool isValid);
    void bestMoveFound(const QString& move);
    void engineOutput(const QString& line);

private:
    QProcess* engineProcess;
    ChessBoard* m_board;
    QString currentFen;
    QString lastFen;

    void initializeEngine();
    bool isEngineBusy;
public:
    void stopThinking();
    void sendUCICommand(const QString& cmd);
};

#endif // STOCKFISHENGINE_H
