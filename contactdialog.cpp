#include "contactdialog.h"

ContactDialog::ContactDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Contact");
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QFormLayout* form = new QFormLayout();
    nameEdit = new QLineEdit(this);
    addressEdit = new QLineEdit(this);
    portSpin = new QSpinBox(this);
    portSpin->setRange(1, 65535);
    portSpin->setValue(12345);
    
    form->addRow("Name:", nameEdit);
    form->addRow("IP/DDNS:", addressEdit);
    form->addRow("Port:", portSpin);
    mainLayout->addLayout(form);
    
    QHBoxLayout* buttons = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("OK", this);
    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addWidget(okBtn);
    buttons->addWidget(cancelBtn);
    mainLayout->addLayout(buttons);
}

void ContactDialog::setContact(const Contact& contact)
{
    nameEdit->setText(contact.name);
    addressEdit->setText(contact.address);
    portSpin->setValue(contact.port);
}

Contact ContactDialog::getContact() const
{
    Contact contact;
    contact.name = nameEdit->text().trimmed();
    contact.address = addressEdit->text().trimmed();
    contact.port = portSpin->value();
    return contact;
}