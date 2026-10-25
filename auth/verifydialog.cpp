#include "verifydialog.h"
#include "ui_verifydialog.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QMessageBox>

VerifyDialog::VerifyDialog(NetworkManager *network, QWidget *parent)
    : QDialog(parent),
    ui(new Ui::VerifyDialog),
    m_network(network),
    m_userId(-1)
{
    ui->setupUi(this);

    // Пока ждём ответа — поля заблокированы
    ui->infoLabel->setText("Отправка кода...");
    ui->codeEdit->setEnabled(false);
    ui->verifyButton->setEnabled(false);
    ui->statusLabel->setText("Подождите...");

    // Только 4 цифры в поле
    QRegularExpression rx("[0-9]{0,4}");
    ui->codeEdit->setValidator(new QRegularExpressionValidator(rx, this));

    // Подписки на сигналы
    connect(ui->verifyButton, &QPushButton::clicked,
            this, &VerifyDialog::onVerifyClicked);

    connect(m_network, &NetworkManager::registerSuccess,
            this, &VerifyDialog::onRegisterSuccess);
    connect(m_network, &NetworkManager::verifySuccess,
            this, &VerifyDialog::onVerifySuccess);
    connect(m_network, &NetworkManager::requestError,
            this, &VerifyDialog::onRequestError);
}

VerifyDialog::~VerifyDialog()
{
    delete ui;
}

// Пришёл ответ на регистрацию — активируем поля
void VerifyDialog::onRegisterSuccess(int userId, const QString &debugCode)
{
    m_userId = userId;

    ui->infoLabel->setText(
        QString("Введите код из письма.\n(Отладка: код = %1)").arg(debugCode));
    ui->statusLabel->clear();

    ui->codeEdit->setEnabled(true);
    ui->verifyButton->setEnabled(true);
    ui->codeEdit->setFocus();
}

// Пользователь нажал «Подтвердить»
void VerifyDialog::onVerifyClicked()
{
    if (m_userId < 0) {
        ui->statusLabel->setText("Ожидание ответа сервера...");
        return;
    }

    QString code = ui->codeEdit->text().trimmed();
    if (code.length() != 4) {
        ui->statusLabel->setText("Код должен содержать 4 цифры");
        return;
    }

    ui->statusLabel->setText("Проверка...");
    ui->verifyButton->setEnabled(false);

    m_network->verifyCode(m_userId, code);
}

// Код подтверждён — закрываем диалог
void VerifyDialog::onVerifySuccess(int userId, const QString &username)
{
    Q_UNUSED(userId)
    Q_UNUSED(username)
    accept();
}

// Ошибка
void VerifyDialog::onRequestError(const QString &error)
{
    if (m_userId < 0) {
        // Ошибка пришла ДО подтверждения → проблема с регистрацией
        QMessageBox::critical(this, "Ошибка регистрации", error);
        reject();   // закрываем окно — RegisterDialog покажет ошибку
    } else {
        // Ошибка подтверждения — показываем в диалоге
        ui->verifyButton->setEnabled(true);
        ui->statusLabel->setText(error);
    }
}