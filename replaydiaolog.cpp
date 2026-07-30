// replaydialog.cpp
#include "replaydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QFile>
#include "Constants.h"

ReplayDialog::ReplayDialog(QWidget* parent)
    : QDialog(parent)
    , m_currentIndex(0)
    , m_totalMoves(0)
    , m_timer(new QTimer(this))
    , m_isPlaying(false)
    , m_board(nullptr)
{
    setupUI();
    connect(m_timer, &QTimer::timeout, this, &ReplayDialog::onTimerTimeout);
}

ReplayDialog::~ReplayDialog()
{
    m_timer->stop();
    if (m_board) {
        delete m_board;
        m_board = nullptr;
    }
}

void ReplayDialog::setupUI()
{
    setWindowTitle("Game Replay");
    setModal(false);
    resize(700, 500);
    setMinimumSize(500, 700);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ===== BOARD =====
    m_board = new ChessBoard(this);
    // No engine! Just visual board

    m_view = new QGraphicsView(this);
    m_view->setScene(m_board);
    m_view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->centerOn(0, 0);
    mainLayout->addWidget(m_view);

    // ===== INFO =====
    m_moveLabel = new QLabel("No moves loaded", this);
    m_moveLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_moveLabel);

    // ===== MEDIA CONTROLS =====
    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(5);
    controlLayout->setAlignment(Qt::AlignCenter);

    QString btnStyle = "QPushButton { font-size: 18px; padding: 5px 12px; }";

    QPushButton* firstBtn = new QPushButton("◄◄", this);
    QPushButton* prevBtn = new QPushButton("◄", this);
    m_playBtn = new QPushButton("▶", this);
    m_stopBtn = new QPushButton("■", this);
    m_stopBtn->setEnabled(false);
    QPushButton* nextBtn = new QPushButton("►", this);
    QPushButton* lastBtn = new QPushButton("►►", this);
    QPushButton* loadBtn = new QPushButton("📂 Load", this);
    QPushButton* currentBtn = new QPushButton("📋 Current", this);
    QPushButton* closeBtn = new QPushButton("✕", this);

    QPushButton* resetBtn = new QPushButton("⟳", this);
    resetBtn->setToolTip("Reset board to start");


    firstBtn->setStyleSheet(btnStyle);
    prevBtn->setStyleSheet(btnStyle);
    m_playBtn->setStyleSheet(btnStyle);
    m_stopBtn->setStyleSheet(btnStyle);
    nextBtn->setStyleSheet(btnStyle);
    lastBtn->setStyleSheet(btnStyle);


    /*
    controlLayout->addWidget(firstBtn);
    controlLayout->addWidget(prevBtn);
    controlLayout->addWidget(m_playBtn);
    controlLayout->addWidget(m_stopBtn);
    controlLayout->addWidget(nextBtn);
    controlLayout->addWidget(lastBtn);
    */



    controlLayout->addWidget(firstBtn);
    controlLayout->addWidget(prevBtn);
    controlLayout->addWidget(m_playBtn);
    controlLayout->addWidget(m_stopBtn);
    controlLayout->addWidget(nextBtn);
    controlLayout->addWidget(lastBtn);
    controlLayout->addWidget(loadBtn);
    controlLayout->addWidget(currentBtn);
    controlLayout->addWidget(closeBtn);
    controlLayout->addSpacing(5);
    controlLayout->addWidget(resetBtn);

    mainLayout->addLayout(controlLayout);

    // ===== CONNECTIONS =====
    connect(firstBtn, &QPushButton::clicked, this, &ReplayDialog::onFirst);
    connect(prevBtn, &QPushButton::clicked, this, &ReplayDialog::onPrevious);
    connect(m_playBtn, &QPushButton::clicked, this, &ReplayDialog::onPlay);
    connect(m_stopBtn, &QPushButton::clicked, this, &ReplayDialog::onStop);
    connect(nextBtn, &QPushButton::clicked, this, &ReplayDialog::onNext);
    connect(lastBtn, &QPushButton::clicked, this, &ReplayDialog::onLast);
    connect(loadBtn, &QPushButton::clicked, this, &ReplayDialog::onLoadHistory);
    connect(currentBtn, &QPushButton::clicked, this, &ReplayDialog::onLoadCurrentHistory);
    connect(closeBtn, &QPushButton::clicked, this, &ReplayDialog::close);
    connect(resetBtn, &QPushButton::clicked, this, &ReplayDialog::resetBoard);

}

void ReplayDialog::loadMoves(const QStringList& moves)
{
    resetBoard();
    m_moves = moves;
    m_totalMoves = moves.size();
    m_currentIndex = 0;
    m_isPlaying = false;
    m_timer->stop();
    m_playBtn->setText("▶");
    m_stopBtn->setEnabled(false);

    // Reset board to starting position
    m_board->setupPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    setMoveInfo();
    updateBoard(-1);
}

void ReplayDialog::setMoveInfo()
{
    if (m_totalMoves == 0) {
        m_moveLabel->setText("No moves loaded");
        return;
    }
    m_moveLabel->setText(QString("Move: %1 / %2").arg(m_currentIndex + 1).arg(m_totalMoves + 1));
}

void ReplayDialog::updateBoard(int index)
{

    if (m_moves.isEmpty()) return;

    // Reset to starting position
    m_board->setupPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // Apply moves up to index
    for (int i = 0; i <= index && i < m_moves.size(); i++) {
        QString move = m_moves[i];
        QString from = move.mid(0, 2);
        QString to = move.mid(2, 2);
        applyReplayMove(from, to);
    }

    m_currentIndex = index;
    setMoveInfo();
}

void ReplayDialog::applyReplayMove(const QString& from, const QString& to)
{
    int fromCol = from[0].toLatin1() - 'a';
    int fromRow = '8' - from[1].toLatin1();
    int toCol = to[0].toLatin1() - 'a';
    int toRow = '8' - to[1].toLatin1();

    auto& pieces = m_board->getPiecesRef();

    ChessPiece* piece = pieces[fromRow][fromCol];
    if (!piece) return;

    // Capture
    if (pieces[toRow][toCol]) {
        ChessPiece* captured = pieces[toRow][toCol];
        m_board->removeItem(captured);
        delete captured;
        pieces[toRow][toCol] = nullptr;
    }

    // Move
    pieces[toRow][toCol] = piece;
    pieces[fromRow][fromCol] = nullptr;
    piece->setPos(toCol * PawnConstants::SQUARE_SIZE, toRow * PawnConstants::SQUARE_SIZE);
    emit playSound(SOUNDTYPE::SOUND_MOVE);
}



void ReplayDialog::onPlay()
{
    if (m_isPlaying) {
        // Pause
        m_isPlaying = false;
        m_timer->stop();
        m_playBtn->setText("▶");
    } else {
        // Play
        if (m_moves.isEmpty()) {
            QMessageBox::information(this, "Replay", "Load a history file first.");
            return;
        }

        if (m_currentIndex >= m_totalMoves) {
            updateBoard(0);
        }

        m_isPlaying = true;
        m_playBtn->setText("⏸");
        m_timer->start(1500);
    }
}

void ReplayDialog::onStop()
{
    m_isPlaying = false;
    m_timer->stop();
    m_playBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
}

void ReplayDialog::onPrevious()
{
    if (m_isPlaying) return;
    if (m_currentIndex > 0) {
        updateBoard(m_currentIndex - 1);
    }
}

void ReplayDialog::onNext()
{
    if (m_isPlaying) return;
    if (m_currentIndex < m_totalMoves) {
        updateBoard(m_currentIndex + 1);
    }
}

void ReplayDialog::onFirst()
{
    if (m_isPlaying) return;
    updateBoard(0);
}

void ReplayDialog::onLast()
{
    if (m_isPlaying) return;
    updateBoard(m_totalMoves);
}

void ReplayDialog::onTimerTimeout()
{
    if (m_currentIndex < m_totalMoves) {
        updateBoard(m_currentIndex + 1);
    } else {
        onStop();
        emit playSound(SOUNDTYPE::SOUND_CHECKMATE);
    }
}

void ReplayDialog::onLoadHistory()
{

    resetBoard();
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Load Game History",
        PawnConstants::saveHistoryDirPath,
        "History Files (*.txt);;All Files (*)"
    );

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file.");
        return;
    }

    QString content = file.readAll();
    file.close();


    QStringList lines = content.split('\n', Qt::SkipEmptyParts);
    QStringList moves;

    for (const QString& line : lines) {
        // Remove move numbers like "1." "2." etc.
        QString cleanLine = line;
        QRegularExpression re("^\\d+\\.\\s*");
        cleanLine.remove(re);

        if (!cleanLine.isEmpty()) {
            moves.append(cleanLine);
        }
    }

    if (moves.isEmpty()) {
        QMessageBox::information(this, "Replay", "No valid moves found.");
        return;
    }

    loadMoves(moves);
}




void ReplayDialog::onLoadCurrentHistory()
{
    emit loadCurrentHistoryRequested();
}

void ReplayDialog::resetBoard()
{
    m_isPlaying = false;
    m_timer->stop();
    m_playBtn->setText("▶");
    m_currentIndex = 0;
    m_board->setupPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    setMoveInfo();
}
