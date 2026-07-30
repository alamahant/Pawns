#ifndef CONTACTDIALOG_H
#define CONTACTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>

struct Contact {
    QString name;
    QString address;
    quint16 port;
};

class ContactDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ContactDialog(QWidget* parent = nullptr);
    
    void setContact(const Contact& contact);
    Contact getContact() const;

private:
    QLineEdit* nameEdit;
    QLineEdit* addressEdit;
    QSpinBox* portSpin;
};

#endif