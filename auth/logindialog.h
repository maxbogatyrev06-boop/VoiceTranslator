#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "network/networkmanager.h"

namespace Ui { class LoginDialog; }

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(NetworkManager *network, QWidget *parent = nullptr);
    ~LoginDialog();

signals:
    void loginSucceeded(int userId, const QString &username,
                        const QString &nativeLanguage, const QString &country);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLoginSuccess(int userId, const QString &username,
                        const QString &nativeLanguage, const QString &country);
    void onRequestError(const QString &error);

private:
    Ui::LoginDialog *ui;
    NetworkManager *m_network;
};

#endif // LOGINDIALOG_H