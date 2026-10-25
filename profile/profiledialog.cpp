#include "profiledialog.h"
#include "ui_profiledialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>
#include <QPixmap>
#include <QApplication>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>

ProfileDialog::ProfileDialog(NetworkManager *network,
                             int userId,
                             const QString &username,
                             const QString &country,
                             const QString &nativeLanguage,
                             const QString &birthdate,
                             QWidget *parent)
    : QDialog(parent),
    ui(new Ui::ProfileDialog),
    m_network(network),
    m_userId(userId),
    m_wasUpdated(false)
{
    ui->setupUi(this);

    ui->usernameEdit->setText(username);

    ui->countryCombo->setEditable(true);
    ui->countryCombo->addItems({
        "Россия", "США", "Германия", "Франция", "Испания",
        "Италия", "Китай", "Япония", "Великобритания", "Канада"
    });
    ui->countryCombo->setCurrentText(country);

    ui->languageCombo->addItems({
        "Русский", "English", "Deutsch", "Español", "Français",
        "Italiano", "Português", "中文", "日本語", "한국어",
        "العربية", "Türkçe", "Polski", "Nederlands"
    });

    static const QStringList codes = {
        "ru", "en", "de", "es", "fr", "it", "pt",
        "zh", "ja", "ko", "ar", "tr", "pl", "nl"
    };
    int idx = codes.indexOf(nativeLanguage);
    if (idx >= 0) ui->languageCombo->setCurrentIndex(idx);

    if (!birthdate.isEmpty()) {
        QDate d = QDate::fromString(birthdate, "yyyy-MM-dd");
        if (d.isValid()) ui->birthdateEdit->setDate(d);
    }

    // Начальная буква
    ui->avatarButton->setText(username.left(1).toUpper());

    // Кнопки
    connect(ui->avatarButton, &QPushButton::clicked,
            this, &ProfileDialog::onAvatarClicked);
    connect(ui->saveButton, &QPushButton::clicked,
            this, &ProfileDialog::onSaveClicked);
    connect(ui->logoutButton, &QPushButton::clicked,
            this, &ProfileDialog::onLogoutClicked);

    // Загружаем аватарку — с задержкой, чтобы окно уже отрисовалось
    QTimer::singleShot(200, this, [this]() { loadAvatar(); });
}

ProfileDialog::~ProfileDialog()
{
    delete ui;
}

QString ProfileDialog::languageCodeFromCombo() const
{
    static const QStringList codes = {
        "ru", "en", "de", "es", "fr", "it", "pt",
        "zh", "ja", "ko", "ar", "tr", "pl", "nl"
    };
    int i = ui->languageCombo->currentIndex();
    return (i >= 0 && i < codes.size()) ? codes[i] : "en";
}

// Загрузка аватарки — упрощённая версия
void ProfileDialog::loadAvatar()
{
    qDebug() << "[Profile] loadAvatar() начал, user_id =" << m_userId;

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QString urlStr = m_network->serverUrl()
                     + QString("/api/avatar/%1?t=%2")
                           .arg(m_userId)
                           .arg(QDateTime::currentMSecsSinceEpoch());

    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(urlStr)));

    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QPixmap pix;
            if (pix.loadFromData(data)) {
                // Обрезаем в круг 120x120
                QPixmap scaled = pix.scaled(120, 120,
                                            Qt::KeepAspectRatioByExpanding,
                                            Qt::SmoothTransformation);

                QPixmap circular(120, 120);
                circular.fill(Qt::transparent);

                QPainter painter(&circular);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(0, 0, 120, 120);
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, scaled);
                painter.end();

                ui->avatarButton->setText("");
                ui->avatarButton->setIcon(QIcon(circular));
                ui->avatarButton->setIconSize(QSize(120, 120));
                ui->avatarButton->setStyleSheet(
                    "QPushButton {"
                    "  background-color: transparent;"
                    "  border: 3px solid #4a9eea;"
                    "  border-radius: 60px;"
                    "  padding: 0px;"
                    "}"
                    );
                qDebug() << "[Profile] Аватарка установлена (круглая)";
            }
        } else {
            qDebug() << "[Profile] Аватарки нет:" << reply->errorString();
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

// Клик по аватарке — выбор файла и загрузка
void ProfileDialog::onAvatarClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    "Выберите аватарку", "",
                                                    "Изображения (*.png *.jpg *.jpeg *.gif *.bmp)");

    if (filePath.isEmpty()) return;

    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
        delete file;
        return;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart idPart;
    idPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                     QVariant("form-data; name=\"user_id\""));
    idPart.setBody(QString::number(m_userId).toUtf8());
    multiPart->append(idPart);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/*"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString("form-data; name=\"avatar\"; filename=\"%1\"")
                                    .arg(QFileInfo(filePath).fileName())));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    QUrl url(m_network->serverUrl() + "/api/upload_avatar");
    QNetworkRequest request(url);
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QNetworkReply *reply = manager->post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, manager]() {
                QByteArray data = reply->readAll();
                qDebug() << "[Profile] Ответ сервера:" << data;

                if (reply->error() == QNetworkReply::NoError) {
                    m_wasUpdated = true;
                    QMessageBox::information(this, "Готово", "Аватарка обновлена!");
                    QTimer::singleShot(500, this, [this]() { loadAvatar(); });
                } else {
                    QMessageBox::warning(this, "Ошибка",
                                         "Не удалось загрузить: " + reply->errorString());
                }
                reply->deleteLater();
                manager->deleteLater();
            });
}

// Сохранить профиль
void ProfileDialog::onSaveClicked()
{
    QString username = ui->usernameEdit->text().trimmed();
    if (username.length() < 3) {
        QMessageBox::warning(this, "Ошибка", "Никнейм не короче 3 символов");
        return;
    }

    QJsonObject body;
    body["user_id"] = m_userId;
    body["username"] = username;
    body["country"] = ui->countryCombo->currentText().trimmed();
    body["native_language"] = languageCodeFromCombo();
    body["birthdate"] = ui->birthdateEdit->date().toString("yyyy-MM-dd");

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(m_network->serverUrl() + "/api/update_profile"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, manager, username]() {
                QByteArray data = reply->readAll();
                if (reply->error() == QNetworkReply::NoError) {
                    m_wasUpdated = true;
                    emit profileUpdated(username,
                                        ui->countryCombo->currentText(),
                                        languageCodeFromCombo());
                    QMessageBox::information(this, "Готово", "Профиль сохранён!");
                    accept();
                } else {
                    QJsonDocument doc = QJsonDocument::fromJson(data);
                    QString err = doc.object()["error"].toString();
                    if (err.isEmpty()) err = reply->errorString();
                    QMessageBox::warning(this, "Ошибка", "Не удалось: " + err);
                }
                reply->deleteLater();
                manager->deleteLater();
            });
}

void ProfileDialog::onLogoutClicked()
{
    if (QMessageBox::question(this, "Выход",
                              "Выйти из аккаунта?") == QMessageBox::Yes) {
        qApp->quit();
    }
}