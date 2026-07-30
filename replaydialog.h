// replaydialog.h
#ifndef REPLAYDIALOG_H
#define REPLAYDIALOG_H

#include <QDialog>
#include <QGraphicsView>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include "chessboard.h"
#include"sounds.h"

class ReplayDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReplayDialog(QWidget* parent = nullptr);
    ~ReplayDialog();
    QStringList currentHistory;

signals:
    void loadCurrentHistoryRequested();
    void playSound(SOUNDTYPE soundtype);

public slots:
    void loadMoves(const QStringList& moves);
    void resetBoard();
    void onLoadHistory();

private slots:
    void onFirst();
    void onPrevious();
    void onPlay();
    void onStop();
    void onNext();
    void onLast();
    void onTimerTimeout();
    void updateBoard(int index);
    void onLoadCurrentHistory();
private:
    void setupUI();
    void applyReplayMove(const QString& from, const QString& to);
    void setMoveInfo();

    ChessBoard* m_board;
    QGraphicsView* m_view;
    QStringList m_moves;
    int m_currentIndex;
    int m_totalMoves;
    QTimer* m_timer;
    bool m_isPlaying;

    QLabel* m_moveLabel;
    QPushButton* m_playBtn;
    QPushButton* m_stopBtn;

};

#endif // REPLAYDIALOG_H
