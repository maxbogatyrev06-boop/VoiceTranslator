#include "logindialog.h"
#include "ui_logindialog.h"
#include <QMessageBox>
#include "auth/registerdialog.h"

LoginDialog::LoginDialog(NetworkManager *network, QWidget *parent)
    : QDialog(parent),
    ui(new Ui::LoginDialog),
    m_network(network)
{
    ui->setupUi(this);

    // Клики по кнопкам
    connect(ui->loginButton, &QPushButton::clicked,
            this, &LoginDialog::onLoginClicked);
    connect(ui->registerButton, &QPushButton::clicked,
            this, &LoginDialog::onRegisterClicked);

    // Ответы сервера
    connect(m_network, &NetworkManager::loginSuccess,
            this, &LoginDialog::onLoginSuccess);
    connect(m_network, &NetworkManager::requestError,
            this, &LoginDialog::onRequestError);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::onLoginClicked()
{
    QString loginField = ui->loginEdit->text().trimmed();
    QString password = ui->passwordEdit->text();

    if (loginField.isEmpty() || password.isEmpty()) {
        ui->statusLabel->setText("Введите логин и пароль");
        return;
    }

    ui->statusLabel->setText("Вход...");
    ui->loginButton->setEnabled(false);

    m_network->login(loginField, password);
}

void LoginDialog::onRegisterClicked()
{
    RegisterDialog dlg(m_network, this);

    connect(&dlg, &RegisterDialog::registrationCompleted,
            this, [this]() {
                ui->statusLabel->setText("Регистрация успешна. Войдите в аккаунт.");
            });

    dlg.exec();
}

void LoginDialog::onLoginSuccess(int userId, const QString &username,
                                 const QString &nativeLanguage,
                                 const QString &country)
{
    ui->loginButton->setEnabled(true);

    // Сохраняем данные для передачи в MainWindow
    setProperty("userId", userId);
    setProperty("username", username);
    setProperty("nativeLanguage", nativeLanguage);
    setProperty("country", country);

    emit loginSucceeded(userId, username, nativeLanguage, country);
    accept();
}

void LoginDialog::onRequestError(const QString &error)
{
    ui->loginButton->setEnabled(true);
    ui->statusLabel->setText(error);
}