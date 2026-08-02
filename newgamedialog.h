// newgamedialog.h
#ifndef NEWGAMEDIALOG_H
#define NEWGAMEDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class NewGameDialog : public QDialog {
    Q_OBJECT
public:
    enum PlayerType { Human, Machine };
    enum ColorChoice { White, Black, Random };
    
    explicit NewGameDialog(QWidget* parent = nullptr);
    
    // Getters - use these to read settings
    PlayerType player1Type() const;
    PlayerType player2Type() const;
    int player1Level() const;
    ColorChoice player1Color() const;
    bool remotePlayEnabled() const;
    bool ficsEnabled() const;
    bool clocksEnabled() const;
    int minutes() const;
    int increment() const;
    bool rated() const;
    void resetDialog();
    QComboBox *p1ColorCombo() const;

    QCheckBox *remotePlayCheck() const;

protected:
    void reject() override;
    void closeEvent(QCloseEvent* event) override;
signals:
    void difficultyChanged(int level);
    void remoteDockVisible(bool toggled);
    void leftdockvisible(bool enabled);

private slots:
    void onPlayer1TypeChanged(int index);
    void onPlayer2TypeChanged(int index);

    void onDifficultyChanged(int index);
    void onStartClicked();
    void onClocksEnabled(bool enabled);

public slots:

    void onColorChanged(int index);

private:
    QComboBox* m_p1TypeCombo;
    QComboBox* m_p1LevelCombo;
    QComboBox* m_p1ColorCombo;
    QComboBox* m_p2TypeCombo;
    QCheckBox* m_remotePlayCheck;
    QCheckBox* m_ficsCheck;
    QCheckBox* m_clocksCheck;
    QComboBox* m_minutesCombo;
    QComboBox* m_incrementCombo;
    QCheckBox* m_ratedCheck;
    QComboBox* m_levelCombo;
    QGroupBox* p1Group;
    QGroupBox* p2Group;
};

#endif
