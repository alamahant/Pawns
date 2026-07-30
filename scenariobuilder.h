#ifndef SCENARIOBUILDER_H
#define SCENARIOBUILDER_H

#include <QDialog>
#include <QListWidget>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QListWidgetItem>
#include"chessboard.h"

class ScenarioBuilder : public QDialog
{
    Q_OBJECT

public:
    explicit ScenarioBuilder(QWidget *parent = nullptr);
    ~ScenarioBuilder();

    void setBoard(ChessBoard *newBoard);

    QRadioButton *getWhiteTurn() const;

signals:
    // Signals that MainWindow will connect to
    void placePieceRequested(const QString& piece, const QString& position);
    void removePieceRequested(const QString& position);
    void clearBoardRequested();
    void startPositionRequested();
    void setPositionRequested(const QString& fen);
    void closeRequested();
    void turnChanged(bool isWhiteTurn);
    void fenCopyRequested();
    void saveRequested();
private slots:

    void onPlaceClicked();
    void onRemoveClicked();
    void onClearBoardClicked();
    void onStartPositionClicked();
    void onSetPositionClicked();
    void onCopyFENClicked();
    void onPasteFENClicked();
    void onCloseClicked();
    void onPieceSelected(QListWidgetItem* item);
    void onPositionSelected(QListWidgetItem* item);
    void onTurnChanged(bool checked);

private:
    void setupUI();
    void updateStatusLabel();
    QString generateFEN();
    void parsePieceString(const QString& pieceStr, QString& color, QString& type);

    // UI Components
    QListWidget* piecesList;
    QListWidget* positionsList;
    QLabel* statusLabel;

    // Settings
    QRadioButton* whiteTurn;
    QRadioButton* blackTurn;
    QCheckBox* castleK;
    QCheckBox* castleQ;
    QCheckBox* castlek;
    QCheckBox* castleq;
    QComboBox* enPassantCombo;
    QSpinBox* halfmoveSpin;
    QSpinBox* fullmoveSpin;

    QString selectedPiece;
    QString selectedPosition;
    ChessBoard* board = nullptr;
};

#endif // SCENARIOBUILDER_H
