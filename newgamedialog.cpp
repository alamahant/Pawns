#include "newgamedialog.h"
#include"Constants.h"
#include<QRandomGenerator>

NewGameDialog::NewGameDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("New Game");
    setModal(true);
    setMinimumWidth(400);
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    p1Group = new QGroupBox("Player 1 (White)");
    QFormLayout* p1Layout = new QFormLayout(p1Group);

    m_p1TypeCombo = new QComboBox();
    m_p1TypeCombo->addItems({"Human", "Machine"});

    connect(m_p1TypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
               this, &NewGameDialog::onPlayer1TypeChanged);


    p1Layout->addRow("Type:", m_p1TypeCombo);

    m_p1ColorCombo = new QComboBox();
    m_p1ColorCombo->addItems({"White", "Black", "Random"});

    connect(m_p1ColorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &NewGameDialog::onColorChanged);



    p1Layout->addRow("Color:", m_p1ColorCombo);

    mainLayout->addWidget(p1Group);

    QGroupBox* levelGroup = new QGroupBox("Difficulty Level");
    QHBoxLayout* levelLayout = new QHBoxLayout(levelGroup);

    m_levelCombo = new QComboBox();
    for (int i = 0; i <= 20; i++) {
        m_levelCombo->addItem(QString::number(i));
    }
    connect(m_levelCombo, &QComboBox::currentIndexChanged, this, &NewGameDialog::onDifficultyChanged);

    m_levelCombo->setCurrentIndex(0);
    levelLayout->addWidget(new QLabel("Level:"));
    levelLayout->addWidget(m_levelCombo);
    levelLayout->addStretch();

    mainLayout->addWidget(levelGroup);

    p2Group = new QGroupBox("Player 2 (Black)");
    QFormLayout* p2Layout = new QFormLayout(p2Group);

    m_p2TypeCombo = new QComboBox();
    m_p2TypeCombo->addItems({"Human", "Machine"});
    connect(m_p2TypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NewGameDialog::onPlayer2TypeChanged);
    p2Layout->addRow("Type:", m_p2TypeCombo);

    QVBoxLayout* checkLayout = new QVBoxLayout();
    m_remotePlayCheck = new QCheckBox("Remote Play (P2P)");
    m_remotePlayCheck->setVisible(false);
    connect(m_remotePlayCheck, &QCheckBox::toggled, this, [this](bool toggled){
        emit remoteDockVisible(toggled);
    });
    m_ficsCheck = new QCheckBox("FICS (coming soon)");
    m_ficsCheck->setEnabled(false);
    checkLayout->addWidget(m_remotePlayCheck);
    checkLayout->addWidget(m_ficsCheck);
    p2Layout->addRow("Mode:", checkLayout);

    mainLayout->addWidget(p2Group);

    QGroupBox* timeGroup = new QGroupBox("Time Control");
    QFormLayout* timeLayout = new QFormLayout(timeGroup);

    m_clocksCheck = new QCheckBox("Use Clock");
    m_clocksCheck->setChecked(false);
    connect(m_clocksCheck, &QCheckBox::toggled, this , [this](bool toggled){
        onClocksEnabled(toggled);
    });
    timeLayout->addRow(m_clocksCheck);

    QHBoxLayout* timeControlLayout = new QHBoxLayout();
    m_minutesCombo = new QComboBox();
    QStringList minutes = {"1", "2", "3", "5", "10", "15", "20", "30", "45", "60"};
    m_minutesCombo->addItems(minutes);
    m_minutesCombo->setCurrentText("10");
    timeControlLayout->addWidget(new QLabel("Minutes:"));
    timeControlLayout->addWidget(m_minutesCombo);

    m_incrementCombo = new QComboBox();
    QStringList increments = {"0", "1", "2", "3", "5", "10", "15", "20", "30"};
    m_incrementCombo->addItems(increments);
    timeControlLayout->addWidget(new QLabel("Increment:"));
    timeControlLayout->addWidget(m_incrementCombo);
    timeLayout->addRow(timeControlLayout);

    m_ratedCheck = new QCheckBox("Rated");
    m_ratedCheck->setChecked(true);
    m_ratedCheck->setVisible(false);
    timeLayout->addRow(m_ratedCheck);

    mainLayout->addWidget(timeGroup);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* startBtn = new QPushButton("Start Game");
    startBtn->setDefault(true);
    QPushButton* cancelBtn = new QPushButton("Cancel");
    connect(startBtn, &QPushButton::clicked, this, &NewGameDialog::onStartClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(startBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);
}

void NewGameDialog::onPlayer1TypeChanged(int index) {

    PawnConstants::startDialogPlayer1IsMachine = (index == Machine);
    if (index == Machine) {
        m_p2TypeCombo->setEnabled(false);
        m_p2TypeCombo->setCurrentIndex(Machine);  // Force machie
    } else {
        m_p2TypeCombo->setEnabled(true);
    }
}

void NewGameDialog::onPlayer2TypeChanged(int index) {
    PawnConstants::startDialogPlayer2IsMachine = (index == Machine);
}

NewGameDialog::PlayerType NewGameDialog::player1Type() const {
    return static_cast<PlayerType>(m_p1TypeCombo->currentIndex());
}

NewGameDialog::PlayerType NewGameDialog::player2Type() const {
    return static_cast<PlayerType>(m_p2TypeCombo->currentIndex());
}

int NewGameDialog::player1Level() const {
    return m_p1LevelCombo->currentIndex();
}

NewGameDialog::ColorChoice NewGameDialog::player1Color() const {
    return static_cast<ColorChoice>(m_p1ColorCombo->currentIndex());
}

bool NewGameDialog::remotePlayEnabled() const {
    return m_remotePlayCheck->isChecked();
}

bool NewGameDialog::ficsEnabled() const {
    return m_ficsCheck->isChecked();
}

bool NewGameDialog::clocksEnabled() const {
    return m_clocksCheck->isChecked();
}

int NewGameDialog::minutes() const {
    return m_minutesCombo->currentText().toInt();
}

int NewGameDialog::increment() const {
    return m_incrementCombo->currentText().toInt();
}

bool NewGameDialog::rated() const {
    return m_ratedCheck->isChecked();
}

void NewGameDialog::onDifficultyChanged(int index) {
    emit difficultyChanged(index);
}

void NewGameDialog::resetDialog()
{
    m_p1TypeCombo->setCurrentIndex(0);  // Human
    m_p1ColorCombo->setCurrentIndex(0);  // White
    m_p1LevelCombo->setCurrentIndex(0);
    m_p1LevelCombo->setEnabled(false);

    m_p2TypeCombo->setCurrentIndex(0);  // Human

    m_remotePlayCheck->setChecked(false);
    m_ficsCheck->setChecked(false);
    m_clocksCheck->setChecked(false);  // Default: clocks ON
    m_ratedCheck->setChecked(false);   // Default: rated ON

    m_minutesCombo->setCurrentText("10");
    m_incrementCombo->setCurrentText("0");

    m_levelCombo->setCurrentIndex(0);
}

void NewGameDialog::reject()
{
    QDialog::reject();
}

void NewGameDialog::closeEvent(QCloseEvent* event)
{
    resetDialog();
    QDialog::closeEvent(event);
}

void NewGameDialog::onColorChanged(int index)
{

    if (index == 0) { // White
        p1Group->setTitle("Player 1 (White)");
        p2Group->setTitle("Player 2 (Black)");

        PawnConstants::startDialogPlayer1IsWhite = true;
    } else if (index == 1) { // Black
        p1Group->setTitle("Player 1 (Black)");
        p2Group->setTitle("Player 2 (White)");
        PawnConstants::startDialogPlayer1IsWhite = false;
    } else { // Random
        bool isWhite = QRandomGenerator::global()->generate() % 2 == 0;
        p1Group->setTitle(isWhite ? "Player 1 (White)" : "Player 1 (Black)");
        p2Group->setTitle(isWhite ? "Player 2 (Black)" : "Player 2 (White)");
        PawnConstants::startDialogPlayer1IsWhite = isWhite;
    }
}

QComboBox *NewGameDialog::p1ColorCombo() const
{
    return m_p1ColorCombo;
}

void NewGameDialog::onStartClicked() {
    if (m_remotePlayCheck->isChecked()) {

        reject();  // Cancel the dialog
    } else {
        accept();  // Normal start
    }
}

void NewGameDialog::onClocksEnabled(bool enabled)
{
    PawnConstants::startDialogClocksEnabled = enabled;
}


