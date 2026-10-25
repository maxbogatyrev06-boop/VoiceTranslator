#include "registerdialog.h"
#include "ui_registerdialog.h"
#include <QMessageBox>
#include <QDate>
#include "auth/verifydialog.h"

RegisterDialog::RegisterDialog(NetworkManager *network, QWidget *parent)
    : QDialog(parent),
    ui(new Ui::RegisterDialog),
    m_network(network),
    m_pendingUserId(-1)
{
    ui->setupUi(this);

    // Дата по умолчанию — 18 лет назад, максимум — сегодня
    ui->birthdateEdit->setDate(QDate::currentDate().addYears(-18));
    ui->birthdateEdit->setMaximumDate(QDate::currentDate());

    // Заполняем страны
    ui->countryCombo->addItems({
        "Россия", "США", "Германия", "Франция", "Испания",
        "Италия", "Китай", "Япония", "Великобритания", "Канада"
    });

    // Заполняем языки
    ui->languageCombo->addItems({
        "Русский", "English", "Deutsch", "Español", "Français",
        "Italiano", "Português", "中文", "日本語", "한국어",
        "العربية", "Türkçe", "Polski", "Nederlands"
    });

    connect(ui->registerButton, &QPushButton::clicked,
            this, &RegisterDialog::onRegisterClicked);
    connect(ui->backButton, &QPushButton::clicked,
            this, &RegisterDialog::onBackClicked);
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

QString RegisterDialog::languageCodeFromCombo() const
{
    static const QStringList codes = {
        "ru", "en", "de", "es", "fr", "it", "pt",
        "zh", "ja", "ko", "ar", "tr", "pl", "nl"
    };
    int idx = ui->languageCombo->currentIndex();
    if (idx >= 0 && idx < codes.size()) {
        return codes[idx];
    }
    return "en";
}

bool RegisterDialog::validateForm(QString &errorMessage)
{
    QString email = ui->emailEdit->text().trimmed();
    QString phone = ui->phoneEdit->text().trimmed();
    QString username = ui->usernameEdit->text().trimmed();
    QString password = ui->passwordEdit->text();
    QString passwordConfirm = ui->passwordConfirmEdit->text();

    if (email.isEmpty() && phone.isEmpty()) {
        errorMessage = "Укажите email или телефон";
        return false;
    }
    if (!email.isEmpty() && !email.contains('@')) {
        errorMessage = "Некорректный email";
        return false;
    }
    if (username.length() < 3) {
        errorMessage = "Никнейм не короче 3 символов";
        return false;
    }
    if (password.length() < 5) {
        errorMessage = "Пароль не короче 5 символов";
        return false;
    }
    if (password != passwordConfirm) {
        errorMessage = "Пароли не совпадают";
        return false;
    }
    if (ui->countryCombo->currentText().trimmed().isEmpty()) {
        errorMessage = "Укажите страну";
        return false;
    }
    return true;
}

void RegisterDialog::onRegisterClicked()
{
    QString errorMessage;
    if (!validateForm(errorMessage)) {
        ui->statusLabel->setText(errorMessage);
        return;
    }

    // Отправляем запрос на сервер
    m_network->registerUser(
        ui->emailEdit->text().trimmed().toLower(),
        ui->phoneEdit->text().trimmed(),
        ui->usernameEdit->text().trimmed(),
        ui->passwordEdit->text(),
        ui->birthdateEdit->date().toString("yyyy-MM-dd"),
        ui->countryCombo->currentText().trimmed(),
        languageCodeFromCombo()
        );

    // Сразу открываем окно подтверждения — не ждём ответа
    VerifyDialog dlg(m_network, this);

    if (dlg.exec() == QDialog::Accepted) {
        QMessageBox::information(this, "Готово",
                                 "Аккаунт подтверждён! Теперь войдите с вашими данными.");
        emit registrationCompleted();
        accept();
    } else {
        ui->statusLabel->setText("Регистрация отменена");
    }
}

void RegisterDialog::onBackClicked()
{
    reject();
}

void RegisterDialog::onRequestError(const QString &error)
{
    ui->registerButton->setEnabled(true);
    ui->statusLabel->setText(error);
}