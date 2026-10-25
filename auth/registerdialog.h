#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include "network/networkmanager.h"

namespace Ui { class RegisterDialog; }

class RegisterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RegisterDialog(NetworkManager *network, QWidget *parent = nullptr);
    ~RegisterDialog();

signals:
    void registrationCompleted();

private slots:
    void onRegisterClicked();
    void onBackClicked();
    void onRequestError(const QString &error);   // ← оставь эту строку

private:
    Ui::RegisterDialog *ui;
    NetworkManager *m_network;
    int m_pendingUserId;

    bool validateForm(QString &errorMessage);
    QString languageCodeFromCombo() const;
};

#endif // REGISTERDIALOG_H