#include "mainwindow.h"
#include "auth/logindialog.h"
#include "network/networkmanager.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    NetworkManager network;

    // Экран входа
    LoginDialog login(&network);
    if (login.exec() != QDialog::Accepted) {
        return 0;
    }

    // Получаем данные пользователя из LoginDialog
    int userId = login.property("userId").toInt();
    QString username = login.property("username").toString();
    QString nativeLang = login.property("nativeLanguage").toString();
    QString country = login.property("country").toString();

    MainWindow w(&network, userId, username, nativeLang, country);
    w.show();

    return a.exec();
}