#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "communication/communicationdialog.h"
#include "profile/profiledialog.h"
#include "nearby/nearbydialog.h"
#include <QDebug>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QFont>

// Иконка-заглушка с буквой
static QPixmap makeLetterIcon(const QString &letter)
{
    QPixmap pix(40, 40);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBrush(QColor("#2a82da"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 40, 40);

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(16);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(0, 0, 40, 40), Qt::AlignCenter, letter);

    painter.end();
    return pix;
}

MainWindow::MainWindow(NetworkManager *network,
                       int userId,
                       const QString &username,
                       const QString &nativeLanguage,
                       const QString &country,
                       QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_network(network),
    m_userId(userId),
    m_username(username),
    m_nativeLanguage(nativeLanguage),
    m_country(country)
{
    ui->setupUi(this);

    setWindowTitle("VoiceTranslator — " + username);
    ui->myNameLabel->setText(username);

    ui->profileButton->setText(username.left(1).toUpper());
    ui->profileButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2a82da;"
        "  color: white;"
        "  border-radius: 22px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  border: none;"
        "}"
        "QPushButton:hover { background-color: #1e6bb8; }"
        );

    QTimer::singleShot(200, this, [this]() { loadMyAvatar(); });

    // HTTP-сигналы
    connect(m_network, &NetworkManager::onlineUsersReceived,
            this, &MainWindow::onUsersReceived);
    connect(m_network, &NetworkManager::countriesReceived,
            this, &MainWindow::onCountriesReceived);
    connect(m_network, &NetworkManager::requestError,
            this, &MainWindow::onRequestError);

    // UI
    connect(ui->searchEdit, &QLineEdit::textChanged,
            this, &MainWindow::onSearchTextChanged);
    connect(ui->countryFilterCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::onCountryFilterChanged);
    connect(ui->profileButton, &QPushButton::clicked,
            this, &MainWindow::onProfileClicked);
    connect(ui->usersList, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onUserDoubleClicked);

    connect(ui->randomButton, &QPushButton::clicked,
            this, &MainWindow::onRandomClicked);

    connect(ui->nearbyButton, &QPushButton::clicked, this, [this]() {
        NearbyDialog dlg(m_nativeLanguage, this);
        dlg.exec();
    });

    // WebSocket
    m_ws = new WebSocketClient(this);
    m_ws->connectToServer("ws://localhost:5000/ws", m_userId);

    connect(m_ws, &WebSocketClient::connected,
            this, &MainWindow::onWsConnected);
    connect(m_ws, &WebSocketClient::errorOccurred,
            this, &MainWindow::onWsError);
    connect(m_ws, &WebSocketClient::incomingInvite,
            this, &MainWindow::onIncomingInvite);
    connect(m_ws, &WebSocketClient::callStarted,
            this, &MainWindow::onCallStarted);
    connect(m_ws, &WebSocketClient::inviteRejected,
            this, &MainWindow::onInviteRejected);
    connect(m_ws, &WebSocketClient::inviteFailed,
            this, &MainWindow::onInviteFailed);
    connect(m_ws, &WebSocketClient::partnerDisconnected,
            this, &MainWindow::onPartnerDisconnected);
    connect(m_ws, &WebSocketClient::waitingForPartner,
            this, &MainWindow::onWaitingForPartner);
    connect(m_ws, &WebSocketClient::searchCancelled,
            this, &MainWindow::onSearchCancelled);

    // HTTP-запросы
    m_network->fetchCountries();
    m_network->fetchOnlineUsers();

    // Heartbeat
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout,
            this, &MainWindow::onHeartbeatTimer);
    m_heartbeatTimer->start(30 * 1000);
    m_network->sendHeartbeat(m_userId);

    // Refresh
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout,
            this, &MainWindow::onRefreshTimer);
    m_refreshTimer->start(15 * 1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Круглая обрезка
QPixmap MainWindow::makeCircularPixmap(const QPixmap &source, int diameter)
{
    QPixmap scaled = source.scaled(diameter, diameter,
                                   Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation);

    QPixmap result(diameter, diameter);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addEllipse(0, 0, diameter, diameter);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, scaled);
    painter.end();

    return result;
}

// Своя аватарка
void MainWindow::loadMyAvatar()
{
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
                QPixmap circular = makeCircularPixmap(pix, 44);
                ui->profileButton->setText("");
                ui->profileButton->setIcon(QIcon(circular));
                ui->profileButton->setIconSize(QSize(44, 44));
                ui->profileButton->setStyleSheet(
                    "QPushButton {"
                    "  background-color: transparent;"
                    "  border: none;"
                    "  border-radius: 22px;"
                    "  padding: 0px;"
                    "}"
                    "QPushButton:hover { border: 2px solid #4a9eea; }"
                    );
            }
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

// Аватарка пользователя
void MainWindow::loadAvatarForUser(int userId)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QString urlStr = m_network->serverUrl()
                     + QString("/api/avatar/%1").arg(userId);

    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(urlStr)));

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, manager, userId]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray data = reply->readAll();
                    QPixmap pix;
                    if (pix.loadFromData(data)) {
                        QPixmap circular = makeCircularPixmap(pix, 40);
                        m_avatarCache[userId] = circular;

                        for (int i = 0; i < ui->usersList->count(); i++) {
                            QListWidgetItem *it = ui->usersList->item(i);
                            if (it->data(Qt::UserRole).toInt() == userId) {
                                it->setIcon(QIcon(circular));
                                break;
                            }
                        }
                    }
                }
                reply->deleteLater();
                manager->deleteLater();
            });
}

// HTTP-слоты
void MainWindow::onUsersReceived(const QJsonArray &users)
{
    m_allUsers = users;
    refreshUsersList();
}

void MainWindow::onCountriesReceived(const QStringList &countries)
{
    ui->countryFilterCombo->blockSignals(true);
    ui->countryFilterCombo->clear();
    ui->countryFilterCombo->addItem("Все страны");
    ui->countryFilterCombo->addItems(countries);
    ui->countryFilterCombo->blockSignals(false);
}

void MainWindow::onRequestError(const QString &error)
{
    updateStatus("Ошибка: " + error);
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text)
    refreshUsersList();
}

void MainWindow::onCountryFilterChanged(int index)
{
    Q_UNUSED(index)
    refreshUsersList();
}

void MainWindow::onHeartbeatTimer()
{
    m_network->sendHeartbeat(m_userId);
}

void MainWindow::onRefreshTimer()
{
    m_network->fetchOnlineUsers();
}

// Список
void MainWindow::refreshUsersList()
{
    ui->usersList->clear();

    QString searchText = ui->searchEdit->text().trimmed().toLower();
    QString countryFilter = ui->countryFilterCombo->currentText();

    int count = 0;
    for (const QJsonValue &v : m_allUsers) {
        QJsonObject user = v.toObject();
        int userId = user["id"].toInt();
        QString username = user["username"].toString();
        QString country = user["country"].toString();

        if (userId == m_userId) continue;
        if (countryFilter != "Все страны" && country != countryFilter) continue;
        if (!searchText.isEmpty() &&
            !username.toLower().contains(searchText)) continue;

        QString itemText = QString("  %1  —  %2").arg(username, country);
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, userId);
        item->setSizeHint(QSize(0, 56));

        if (m_avatarCache.contains(userId)) {
            item->setIcon(QIcon(m_avatarCache[userId]));
        } else {
            item->setIcon(makeLetterIcon(username.left(1).toUpper()));
            loadAvatarForUser(userId);
        }

        ui->usersList->addItem(item);
        count++;
    }

    ui->onlineLabel->setText(QString("Онлайн: %1").arg(count));
}

void MainWindow::updateStatus(const QString &text)
{
    ui->statusLabel->setText(text);
    qDebug() << "[MainWindow]" << text;
}

// WebSocket-слоты
void MainWindow::onWsConnected()
{
    qDebug() << "[Main] WebSocket подключён";
}

void MainWindow::onWsError(const QString &error)
{
    updateStatus("WebSocket: " + error);
}

void MainWindow::onIncomingInvite(int fromId,
                                  const QString &fromUsername,
                                  const QString &fromCountry,
                                  const QString &fromLanguage)
{
    Q_UNUSED(fromCountry)
    Q_UNUSED(fromLanguage)

    auto reply = QMessageBox::question(
        this, "Входящий звонок",
        QString("%1 хочет с вами поговорить.\n\nПринять?").arg(fromUsername),
        QMessageBox::Yes | QMessageBox::No);

    bool accepted = (reply == QMessageBox::Yes);
    m_ws->respondToInvite(fromId, m_userId, accepted);
    updateStatus(accepted ? "Принято" : "Отклонено");
}

void MainWindow::onCallStarted(int partnerId,
                               const QString &partnerUsername,
                               const QString &partnerCountry,
                               const QString &partnerLanguage)
{
    // Сброс флага поиска
    if (m_searching) {
        m_searching = false;
        ui->randomButton->setText("🎲  Найти случайного собеседника");
        ui->randomButton->setStyleSheet("");
    }

    updateStatus("Разговор начат с " + partnerUsername);

    CommunicationDialog dlg(m_userId, partnerId, partnerUsername,
                            partnerCountry,
                            m_nativeLanguage, partnerLanguage,
                            m_ws, this);
    dlg.exec();

    updateStatus("Разговор завершён");
}

void MainWindow::onInviteRejected(int byUserId)
{
    Q_UNUSED(byUserId)
    updateStatus("Пользователь отклонил приглашение");
    QMessageBox::information(this, "Отклонено",
                             "Пользователь отказался от разговора.");
}

void MainWindow::onInviteFailed(const QString &reason)
{
    updateStatus("Приглашение не удалось: " + reason);
    QMessageBox::warning(this, "Ошибка", reason);
}

void MainWindow::onPartnerDisconnected(int userId)
{
    Q_UNUSED(userId)
    updateStatus("Собеседник отключился");
    QMessageBox::information(this, "Отключение",
                             "Собеседник завершил разговор.");
}

void MainWindow::onUserDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;

    int userId = item->data(Qt::UserRole).toInt();
    QString display = item->text();
    QString username = display.section("—", 0, 0).remove("👤").trimmed();

    updateStatus("Приглашаю " + username + "...");
    m_ws->inviteUser(m_userId, userId);
}

// Профиль
void MainWindow::onProfileClicked()
{
    ProfileDialog dlg(m_network, m_userId, m_username,
                      m_country, m_nativeLanguage, "", this);

    if (dlg.exec() == QDialog::Accepted && dlg.wasUpdated()) {
        loadMyAvatar();
        updateStatus("Профиль обновлён");
    }
}

// Случайный собеседник
void MainWindow::onRandomClicked()
{
    if (!m_searching) {
        m_searching = true;
        ui->randomButton->setText("✕  Отменить поиск");
        ui->randomButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #e53935;"
            "  color: white;"
            "  border-radius: 6px;"
            "  border: none;"
            "}"
            "QPushButton:hover { background-color: #c62828; }"
            );
        updateStatus("Поиск собеседника...");
        m_ws->findRandom(m_userId);
    } else {
        m_searching = false;
        ui->randomButton->setText("🎲  Найти случайного собеседника");
        ui->randomButton->setStyleSheet("");
        updateStatus("Поиск отменён");
        m_ws->cancelSearch(m_userId);
    }
}

void MainWindow::onWaitingForPartner()
{
    updateStatus("⏳ Ожидание собеседника...");
}

void MainWindow::onSearchCancelled()
{
    m_searching = false;
    ui->randomButton->setText("🎲  Найти случайного собеседника");
    ui->randomButton->setStyleSheet("");
    updateStatus("Поиск отменён");
}