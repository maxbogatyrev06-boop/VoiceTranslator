#ifndef VERIFYDIALOG_H
#define VERIFYDIALOG_H

#include <QDialog>
#include "network/networkmanager.h"

namespace Ui { class VerifyDialog; }

class VerifyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VerifyDialog(NetworkManager *network, QWidget *parent = nullptr);
    ~VerifyDialog();

private slots:
    void onVerifyClicked();
    void onRegisterSuccess(int userId, const QString &debugCode);
    void onVerifySuccess(int userId, const QString &username);
    void onRequestError(const QString &error);

private:
    Ui::VerifyDialog *ui;
    NetworkManager *m_network;
    int m_userId;   // -1 = ещё не получили от сервера
};

#endif // VERIFYDIALOG_H