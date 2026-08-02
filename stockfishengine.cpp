#include "stockfishengine.h"
#include"chessboard.h"
#include"Constants.h"
#include <QDebug>

StockfishEngine::StockfishEngine(QObject* parent)
    : QObject(parent)
    , engineProcess(new QProcess(this))
    , m_board(nullptr)
    , isEngineBusy(false)
{
    engineProcess->setProcessChannelMode(QProcess::MergedChannels);
    engineProcess->start("stockfish");

    if (!engineProcess->waitForStarted()) {
        return;
    }

    initializeEngine();
}

StockfishEngine::~StockfishEngine() {
    if (engineProcess->state() == QProcess::Running) {
        sendCommand("quit");
        engineProcess->waitForFinished();
    }
}

void StockfishEngine::initializeEngine() {
    sendCommand("uci");
    waitForResponse("uciok");
    sendCommand("setoption name MultiPV value 1");
    sendCommand("isready");
    waitForResponse("readyok");
    emit engineReady();
}



void StockfishEngine::sendCommand(const QString& cmd) {
    if (!engineProcess || engineProcess->state() != QProcess::Running) {
        return;
    }
    engineProcess->write((cmd + "\n").toUtf8());
    engineProcess->waitForBytesWritten();
}

QString StockfishEngine::waitForResponse(const QString& expected) {
    QString response;
    int attempts = 0;
    const int MAX_ATTEMPTS = 600; // 50 seconds max

    while (!response.contains(expected) && attempts < MAX_ATTEMPTS) {
        if (engineProcess->waitForReadyRead(100)) {
            QString newData = QString::fromUtf8(engineProcess->readAllStandardOutput());
            response += newData;
        }
        attempts++;
    }
    if (PawnConstants::isEngineLoggingEnabled) {
        emit engineOutput(response);
    }
    return response;
}

void StockfishEngine::setBoard(ChessBoard* board) {
    m_board = board;
}

void StockfishEngine::setPosition(const QString& fen) {
    currentFen = fen;
    if (currentFen != lastFen) {
        sendCommand("position fen " + currentFen);
        lastFen = currentFen;
    }
}



bool StockfishEngine::validateMove(const QString& from, const QString& to) {
    QString move = toUCI(from, to);

    sendCommand("position fen " + currentFen);
    sendCommand("go perft 1");
    QString response = waitForResponse("Nodes");

    bool isValid = response.contains(move + ": 1");


    return isValid;
}


QString StockfishEngine::getBestMove(int thinkTime) {
    if (isEngineBusy) return QString();

    isEngineBusy = true;
    sendCommand(QString("go movetime %1").arg(thinkTime));
    QString response = waitForResponse("bestmove");
    isEngineBusy = false;

    if (!response.contains("bestmove")) {
        return QString();
    }

    QStringList lines = response.split("\n");
    QString bestmoveLine;
    for (const QString& line : lines) {
        if (line.trimmed().startsWith("bestmove")) {
            bestmoveLine = line.trimmed();
            break;
        }
    }

    if (bestmoveLine.isEmpty()) {
        return QString();
    }

    QStringList parts = bestmoveLine.split(" ");
    if (parts.size() < 2) {
        return QString();
    }

    QString bestMove = parts[1];
    if (bestMove == "(none)") {
        return "none";
    }

    emit bestMoveFound(bestMove);
    return bestMove;
}


void StockfishEngine::setDifficulty(int level) {
    sendCommand(QString("setoption name Skill Level value %1").arg(level));
    sendCommand("d");  // Add this line to verify the setting

}

QString StockfishEngine::toUCI(const QString& from, const QString& to) {
    return from + to;
}

void StockfishEngine::fromUCI(const QString& uci, QString& from, QString& to) {
    from = uci.mid(0, 2);
    to = uci.mid(2, 2);
}


bool StockfishEngine::isPositionCheck(const QString& kingPos) {
    sendCommand("position fen " + currentFen);
    sendCommand("d");
    QString response = waitForResponse("Checkers:");

    bool isCheck = response.contains("Checkers:") &&
            !response.contains("Checkers: (none)") &&
            !response.contains("Checkers: \n");


    return isCheck;
}

void StockfishEngine::updatePosition(const QString& from, const QString& to) {
    QString moveString = from + to;
    QString command = "position fen " + currentFen + " moves " + moveString;
    sendCommand(command);

    sendCommand("d");
    waitForResponse("Checkers:");
}

void StockfishEngine::stopThinking() {
    if (engineProcess && engineProcess->state() == QProcess::Running) {
        sendCommand("stop");
        isEngineBusy = false;
    }
}

void StockfishEngine::sendUCICommand(const QString& cmd) {
    sendCommand(cmd);

    if (engineProcess->waitForReadyRead(100)) {
        QString response = QString::fromUtf8(engineProcess->readAllStandardOutput());
        if (!response.isEmpty() && PawnConstants::isEngineLoggingEnabled) {
            emit engineOutput(response);
        }
    }
}

