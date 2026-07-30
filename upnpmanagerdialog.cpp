#include "upnpmanagerdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkInterface>

extern "C" {
#include <miniupnpc.h>
#include <upnpcommands.h>
#include <upnperrors.h>
}
#include"Constants.h"


UPnPManagerDialog::UPnPManagerDialog(QWidget *parent)
    : QDialog(parent)
    , m_routerFound(false)
{
    setWindowTitle("UPnP Port Manager");
    setMinimumSize(600, 400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* controlGroup = new QGroupBox("Port Mapping");
    controlLayout = new QHBoxLayout(controlGroup);

    controlLayout->addWidget(new QLabel("Port:"));
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(12345);
    controlLayout->addWidget(m_portSpin);

    controlLayout->addWidget(new QLabel("Protocol:"));
    m_protocolCombo = new QComboBox();
    m_protocolCombo->addItem("TCP");
    m_protocolCombo->addItem("UDP");
    int idx = m_protocolCombo->findText("TCP");
    if (idx >= 0) m_protocolCombo->setCurrentIndex(idx);
    controlLayout->addWidget(m_protocolCombo);

    m_openBtn = new QPushButton("Open Port");
    connect(m_openBtn, &QPushButton::clicked, this, &UPnPManagerDialog::onOpenPort);
    controlLayout->addWidget(m_openBtn);

    controlLayout->addStretch();
    setupCornerWidget();
    mainLayout->addWidget(controlGroup);

    QGroupBox* listGroup = new QGroupBox("Active Mappings");
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);
    m_mappingList = new QListWidget();
    listLayout->addWidget(m_mappingList);

    QHBoxLayout* listBtnLayout = new QHBoxLayout();
    m_closeSelectedBtn = new QPushButton("Close Selected");
    connect(m_closeSelectedBtn, &QPushButton::clicked, this, &UPnPManagerDialog::onCloseSelected);
    listBtnLayout->addWidget(m_closeSelectedBtn);

    m_closeAllBtn = new QPushButton("Close All (Pawns)");
    connect(m_closeAllBtn, &QPushButton::clicked, this, &UPnPManagerDialog::onCloseAll);
    listBtnLayout->addWidget(m_closeAllBtn);

    m_refreshBtn = new QPushButton("Refresh List");
    connect(m_refreshBtn, &QPushButton::clicked, this, &UPnPManagerDialog::onRefresh);
    listBtnLayout->addWidget(m_refreshBtn);

    listBtnLayout->addStretch();
    listLayout->addLayout(listBtnLayout);
    mainLayout->addWidget(listGroup);

    QHBoxLayout* bottomLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Ready");
    bottomLayout->addWidget(m_statusLabel, 1);

    m_testBtn = new QPushButton("Test Router");
    connect(m_testBtn, &QPushButton::clicked, this, &UPnPManagerDialog::onTestRouter);
    bottomLayout->addWidget(m_testBtn);

    QPushButton* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);

    mainLayout->addLayout(bottomLayout);

}



bool UPnPManagerDialog::discoverRouter(struct UPNPUrls& urls, struct IGDdatas& data, char* lanaddr)
{
    int error = 0;
    struct UPNPDev* devlist = upnpDiscover(2000, NULL, NULL, 0, 0, 2, &error);
    if (!devlist) {
        m_lastError = "No UPnP devices found";
        return false;
    }

    int rc = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, 64, NULL, 0);
    //int rc = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, 64);
    freeUPNPDevlist(devlist);

    if (rc != 1) {
        m_lastError = "No valid Internet Gateway Device found";
        return false;
    }

    m_routerFound = true;
    return true;
}

bool UPnPManagerDialog::openPort(int port, const QString& protocol)
{
    struct UPNPUrls urls;
    struct IGDdatas data;
    char lanaddr[64];

    if (!discoverRouter(urls, data, lanaddr)) {
        return false;
    }

    char portStr[8];
    snprintf(portStr, sizeof(portStr), "%d", port);

    int result = UPNP_AddPortMapping(
        urls.controlURL,
        data.first.servicetype,
        portStr,
        portStr,
        lanaddr,
        "Pawns App",
        protocol.toUtf8().constData(),
        NULL,
        "0"
    );

    FreeUPNPUrls(&urls);

    if (result != UPNPCOMMAND_SUCCESS) {
        m_lastError = QString("UPnP Error: %1").arg(result);
        return false;
    }
    addMappingToList(port, protocol);

    return true;
}


bool UPnPManagerDialog::closePort(int port, const QString& protocol)
{
    struct UPNPUrls urls;
    struct IGDdatas data;
    char lanaddr[64];

    if (!discoverRouter(urls, data, lanaddr)) {
        return false;
    }

    char portStr[8];
    snprintf(portStr, sizeof(portStr), "%d", port);

    int result = UPNP_DeletePortMapping(
        urls.controlURL,
        data.first.servicetype,
        portStr,
        protocol.toUtf8().constData(),
        NULL
    );

    FreeUPNPUrls(&urls);

    if (result != UPNPCOMMAND_SUCCESS) {
        m_lastError = QString("UPnP Error: %1").arg(result);
        return false;
    }

    for (int i = 0; i < m_mappingList->count(); i++) {
        QListWidgetItem* item = m_mappingList->item(i);
        if (item->data(Qt::UserRole).toInt() == port &&
            item->data(Qt::UserRole + 1).toString() == protocol) {
            delete m_mappingList->takeItem(i);
            break;
        }
    }

    for (int i = 0; i < m_mappings.size(); i++) {
        if (m_mappings[i].externalPort == port &&
            m_mappings[i].protocol == protocol) {
            m_mappings.removeAt(i);
            break;
        }
    }
   \
    return true;
}

bool UPnPManagerDialog::closeAllPorts()
{
    bool allClosed = true;
    for (const UPnPMapping& m : m_mappings) {
        if (m.description == "Pawns App") {
            if (!closePort(m.externalPort, m.protocol)) {
                allClosed = false;
            }
        }
    }
    return allClosed;
}


QString UPnPManagerDialog::getLocalIp()
{
    for (const QHostAddress& addr : QNetworkInterface::allAddresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
            !addr.isLoopback() &&
            addr != QHostAddress::LocalHost) {
            return addr.toString();
        }
    }
    return "127.0.0.1";
}

void UPnPManagerDialog::setStatus(const QString& msg, bool isError)
{
    m_statusLabel->setText(msg);
    if (isError) {
        m_statusLabel->setStyleSheet("color: red;");
    } else {
        m_statusLabel->setStyleSheet("");
    }
}

QList<UPnPMapping> UPnPManagerDialog::mappings() const
{
    return m_mappings;
}



void UPnPManagerDialog::refreshMappings()
{
    return;

    m_mappings.clear();
    m_mappingList->clear();

    struct UPNPUrls urls;
    struct IGDdatas data;
    char lanaddr[64];

    if (!discoverRouter(urls, data, lanaddr)) {
        setStatus("Router not found. UPnP may be disabled.", true);
        return;
    }

    unsigned int count = 0;
    int result = UPNP_GetPortMappingNumberOfEntries(urls.controlURL, data.first.servicetype, &count);

    for (unsigned int i = 0; i < count; i++) {
        char extPort[8], intPort[8], intClient[64], protocol[8], desc[128], enabled[8], lease[8];
        char remoteHost[64];

        int r = UPNP_GetGenericPortMappingEntry(
            urls.controlURL,
            data.first.servicetype,
            QString::number(i).toUtf8().constData(),
            extPort,
            intClient,
            intPort,
            protocol,
            desc,
            enabled,
            lease,
            remoteHost
        );


        if (r == UPNPCOMMAND_SUCCESS) {
            UPnPMapping m;
            m.externalPort = QString(extPort).toInt();
            m.internalIp = QString(intClient);
            m.internalPort = QString(intPort).toInt();
            m.protocol = QString(protocol);
            m.description = QString(desc);
            m.enabled = (QString(enabled) == "1");

            m_mappings.append(m);

            QString display = QString("%1 → %2:%3 (%4) [%5]")
                .arg(m.externalPort)
                .arg(m.internalIp)
                .arg(m.internalPort)
                .arg(m.protocol)
                .arg(m.description.isEmpty() ? "No desc" : m.description);
            m_mappingList->addItem(display);
        }

    }

    FreeUPNPUrls(&urls);
    setStatus(QString("Found %1 mappings").arg(m_mappings.size()));
}

void UPnPManagerDialog::onCloseSelected()
{
    int row = m_mappingList->currentRow();
    if (row < 0 || row >= m_mappings.size()) {
        setStatus("Please select a mapping to close", true);
        return;
    }

    UPnPMapping m = m_mappings.at(row);
    if (closePort(m.externalPort, m.protocol)) {
        setStatus(QString("Closed port %1 (%2)").arg(m.externalPort).arg(m.protocol));
        refreshMappings();
    } else {
        setStatus(QString("Failed to close port: %1").arg(m_lastError), true);
    }
}

void UPnPManagerDialog::onCloseAll()
{
    if (m_mappings.isEmpty()) {
        setStatus("No mappings to close");
        return;
    }

    if (closeAllPorts()) {
        setStatus("Closed all Pawns App mappings");
        refreshMappings();
    } else {
        setStatus("Failed to close all ports", true);
    }
}

void UPnPManagerDialog::onRefresh()
{
    refreshMappings();
}


void UPnPManagerDialog::onTestRouter()
{
    struct UPNPUrls urls;
    struct IGDdatas data;
    char lanaddr[64];

    if (discoverRouter(urls, data, lanaddr)) {
        char externalIp[16];
        int result = UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIp);

        QString msg = QString("UPnP router found\nLocal IP: %1\nControl URL: %2")
            .arg(lanaddr)
            .arg(urls.controlURL);

        if (result == UPNPCOMMAND_SUCCESS) {
            msg += QString("\nExternal IP: %1").arg(externalIp);
        } else {
            msg += "\nExternal IP: Could not retrieve";
        }

        setStatus("UPnP router found");
        QMessageBox::information(this, "UPnP Test", msg);
        FreeUPNPUrls(&urls);
    } else {
        setStatus("❌ No UPnP router found", true);
        QMessageBox::warning(this, "UPnP Test", "No UPnP router found.\n\nMake sure UPnP is enabled on your router.");
    }
}


void UPnPManagerDialog::onOpenPort()
{
    int port = m_portSpin->value();
    QString protocol = m_protocolCombo->currentText();


    if (openPort(port, protocol)) {
        setStatus(QString("Port %1 (%2) opened successfully").arg(port).arg(protocol));
        refreshMappings();
    } else {
        setStatus(QString("Failed to open port: %1").arg(m_lastError), true);
    }
}

void UPnPManagerDialog::addMappingToList(int port, const QString& protocol)
{
    QString display = QString("%1 (%2) [Pawns App]")
        .arg(port)
        .arg(protocol);

    QListWidgetItem* item = new QListWidgetItem(display);
    item->setData(Qt::UserRole, port);
    item->setData(Qt::UserRole + 1, protocol);
    m_mappingList->addItem(item);

    UPnPMapping m;
    m.externalPort = port;
    m.internalPort = port;
    m.protocol = protocol;
    m.description = "Pawns App";
    m.internalIp = getLocalIp();
    m.enabled = true;
    m_mappings.append(m);
}


void UPnPManagerDialog::setupCornerWidget()
{
    QPushButton *infoButton = new QPushButton("ⓘ", this);
    infoButton->setFixedSize(16, 16);

    infoButton->setToolTip(
        "<html><body style='white-space: nowrap;'>"
        "<b>🌐 UPnP Port Manager</b><br>"
        "━━━━━━━━━━━━━━━━━━━━━━━━━━━━<br><br>"

        "<b>ℹ️ What This Does:</b><br>"
        "• Opens ports on your router using UPnP<br>"
        "• No need to manually configure port forwarding<br>"
        "• Works with most home routers<br><br>"

        "<b>📌 How to Use:</b><br>"
        "• Enter the port number you want to open<br>"
        "• Select TCP or UDP protocol<br>"
        "• Click 'Open Port' to add the mapping<br>"
        "• The port will appear in the list below<br>"
        "• Select a port and click 'Close Selected' to remove it<br>"
        "• Click 'Close All (Pawns)' to remove all ports opened by this app<br><br>"

        "<b>⚠️ Important Notes:</b><br>"
        "• Your router MUST have UPnP enabled<br>"
        "• Some routers do not support listing mappings<br>"
        "• This dialog tracks ports locally<br>"
        "• Ports opened here are visible in your router's UPnP table<br><br>"

        "<b>🔧 Test Router:</b><br>"
        "• Click 'Test Router' to verify UPnP is working<br>"
        "• Shows local IP and control URL if successful<br><br>"

        "<b>⏱️ Closing Ports:</b><br>"
           "• Ports can be closed manually via the buttons above<br>"
           "• Ports will also close automatically when the application exits<br>"
           "• They will also expire automatically based on router settings<br>"
           "</body></html>"
    );

    infoButton->setObjectName("infoButton");

    if (PawnConstants::isDarkTheme) {
        infoButton->setStyleSheet(
            "QPushButton#infoButton { "
            "background-color: rgba(13, 71, 161, 80); "
            "border-radius: 16px; "
            "font-size: 18px; "
            "font-weight: bold; "
            "color: rgba(255, 255, 255, 80); "
            "border: none; "
            "padding: 0px; "
            "} "
            "QPushButton#infoButton:hover { "
            "background-color: rgba(21, 101, 192, 255); "
            "color: rgba(255, 255, 255, 255); "
            "}"
        );
    } else {
        infoButton->setStyleSheet(
            "QPushButton#infoButton { "
            "background-color: rgba(70, 90, 110, 60); "
            "border-radius: 16px; "
            "font-size: 18px; "
            "font-weight: bold; "
            "color: rgba(40, 50, 70, 120); "
            "border: none; "
            "padding: 0px; "
            "} "
            "QPushButton#infoButton:hover { "
            "background-color: rgba(21, 101, 192, 200); "
            "color: rgba(255, 255, 255, 255); "
            "}"
        );
    }

    controlLayout->addWidget(infoButton);
}
