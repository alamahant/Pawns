#ifndef UPNPMANAGERDIALOG_H
#define UPNPMANAGERDIALOG_H

#include <QDialog>
#include <QList>
#include <QString>
#include <qboxlayout.h>

struct UPnPMapping {
    int externalPort;
    QString internalIp;
    int internalPort;
    QString protocol;
    QString description;
    bool enabled;
};

class QSpinBox;
class QComboBox;
class QListWidget;
class QPushButton;
class QLabel;

class UPnPManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UPnPManagerDialog(QWidget *parent = nullptr);

private slots:
    void onOpenPort();
    void onCloseSelected();
    void onCloseAll();
    void onRefresh();
    void onTestRouter();

private:
    // --- UI ---
    QSpinBox* m_portSpin;
    QComboBox* m_protocolCombo;
    QListWidget* m_mappingList;
    QLabel* m_statusLabel;
    QPushButton* m_openBtn;
    QPushButton* m_closeSelectedBtn;
    QPushButton* m_closeAllBtn;
    QPushButton* m_refreshBtn;
    QPushButton* m_testBtn;

    // --- UPnP Logic ---
    bool discoverRouter(struct UPNPUrls& urls, struct IGDdatas& data, char* lanaddr);
    bool closePort(int port, const QString& protocol);
    void refreshMappings();
    QString getLocalIp();
    void setStatus(const QString& msg, bool isError = false);

    // --- State ---
    bool m_routerFound;
    QString m_lastError;
    QList<UPnPMapping> m_mappings;
    void addMappingToList(int port, const QString& protocol);
    void setupCornerWidget();
    QHBoxLayout* controlLayout;
public:
    bool closeAllPorts();
    bool openPort(int port, const QString& protocol);


    QList<UPnPMapping> mappings() const;
};

#endif // UPNPMANAGERDIALOG_H
