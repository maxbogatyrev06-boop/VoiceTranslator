#include "communicationdialog.h"
#include "ui_communicationdialog.h"
#include <QMessageBox>
#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QTimer>

CommunicationDialog::CommunicationDialog(int myUserId,
                                         int partnerId,
                                         const QString &partnerUsername,
                                         const QString &partnerCountry,
                                         const QString &myNativeLang,
                                         const QString &partnerNativeLang,
                                         WebSocketClient *ws,
                                         QWidget *parent)
    : QDialog(parent),
    ui(new Ui::CommunicationDialog),
    m_ws(ws),
    m_myUserId(myUserId),
    m_partnerId(partnerId),
    m_partnerUsername(partnerUsername),
    m_myNativeLang(myNativeLang),
    m_partnerNativeLang(partnerNativeLang),
    m_speakingOwnTranslation(false)
{
    ui->setupUi(this);

    ui->partnerNameLabel->setText(partnerUsername);
    ui->partnerInfoLabel->setText(
        QString("%1  ·  %2").arg(partnerCountry, partnerNativeLang));
    ui->partnerAvatarLabel->setText(partnerUsername.left(1).toUpper());

    QTimer::singleShot(200, this, [this]() { loadPartnerAvatar(); });

    m_recorder   = new AudioRecorder(this);
    m_stt        = new STTManager(this);
    m_translator = new Translator(this);
    m_tts        = new TTSManager(this);

    connect(ui->micButton, &QPushButton::toggled,
            this, &CommunicationDialog::onMicToggled);
    connect(ui->endButton, &QPushButton::clicked,
            this, &CommunicationDialog::onEndClicked);

    connect(m_recorder, &AudioRecorder::recordingFinished,
            this, &CommunicationDialog::onRecordingFinished);
    connect(m_recorder, &AudioRecorder::recordingError,
            this, &CommunicationDialog::onRecordingError);

    connect(m_stt, &STTManager::transcriptionFinished,
            this, &CommunicationDialog::onTranscriptionFinished);
    connect(m_stt, &STTManager::transcriptionError,
            this, &CommunicationDialog::onTranscriptionError);

    connect(m_translator, &Translator::translationFinished,
            this, &CommunicationDialog::onTranslationFinished);
    connect(m_translator, &Translator::translationError,
            this, &CommunicationDialog::onTranslationError);

    connect(m_tts, &TTSManager::speakingFinished,
            this, &CommunicationDialog::onSpeakingFinished);
    connect(m_tts, &TTSManager::speakingError,
            this, &CommunicationDialog::onSpeakingError);

    if (m_ws) {
        connect(m_ws, &WebSocketClient::incomingTranslation,
                this, &CommunicationDialog::onIncomingTranslation);
        connect(m_ws, &WebSocketClient::partnerDisconnected,
                this, &CommunicationDialog::onPartnerDisconnected);
    }

    updateStatus("Нажмите 🎤 чтобы начать говорить");
}

CommunicationDialog::~CommunicationDialog()
{
    delete ui;
}

// Обрезка в круг
static QPixmap makeCircularPixmapLocal(const QPixmap &source, int diameter)
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

// Загрузка аватарки собеседника
void CommunicationDialog::loadPartnerAvatar()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QString urlStr = QString("http://localhost:5000/api/avatar/%1?t=%2")
                         .arg(m_partnerId)
                         .arg(QDateTime::currentMSecsSinceEpoch());

    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(urlStr)));

    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QPixmap pix;
            if (pix.loadFromData(data)) {
                QPixmap circular = makeCircularPixmapLocal(pix, 160);
                ui->partnerAvatarLabel->setText("");
                ui->partnerAvatarLabel->setPixmap(circular);
                ui->partnerAvatarLabel->setStyleSheet(
                    "background-color: transparent;"
                    "border: 4px solid #ffffff33;"
                    "border-radius: 80px;"
                    );
                qDebug() << "[Comm] Аватарка собеседника загружена";
            }
        } else {
            qDebug() << "[Comm] Аватарки собеседника нет";
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

// Микрофон
void CommunicationDialog::onMicToggled(bool checked)
{
    if (checked) {
        ui->recognizedTextEdit->clear();
        ui->translatedTextEdit->clear();
        m_recorder->startRecording();
        ui->micButton->setToolTip("Выключить микрофон");
        updateStatus("🔴 Идёт запись... Говорите");
    } else {
        m_recorder->stopRecording();
        ui->micButton->setToolTip("Включить микрофон");
        updateStatus("Обработка...");
    }
}

void CommunicationDialog::onEndClicked()
{
    if (m_recorder->isRecording()) m_recorder->stopRecording();
    if (m_tts->isSpeaking()) m_tts->stop();

    if (m_ws) m_ws->endCall(m_myUserId);

    reject();
}

// Запись завершена → распознавание
void CommunicationDialog::onRecordingFinished(const QString &filePath)
{
    updateStatus("Распознавание речи...");
    qDebug() << "[Comm] Запись завершена:" << filePath;
    m_stt->transcribe(filePath, "");
}

void CommunicationDialog::onRecordingError(const QString &error)
{
    ui->micButton->setChecked(false);
    updateStatus("Ошибка записи: " + error);
}

// Распознано → перевод
void CommunicationDialog::onTranscriptionFinished(const QString &text,
                                                  const QString &detectedLang)
{
    qDebug() << "[Comm] Распознано:" << text
             << "| Язык:" << detectedLang;

    ui->recognizedTextEdit->setPlainText(
        QString("[%1] %2").arg(detectedLang, text));

    if (detectedLang == m_partnerNativeLang) {
        ui->translatedTextEdit->setPlainText(text);

        if (m_ws) {
            m_ws->sendTranslation(m_myUserId, text, detectedLang);
        }

        m_targetLang = detectedLang;
        m_speakingOwnTranslation = true;
        m_tts->speak(text, detectedLang);
        updateStatus("Говорите на языке собеседника");
        return;
    }

    updateStatus(QString("Перевод %1 → %2...")
                     .arg(detectedLang, m_partnerNativeLang));

    m_targetLang = m_partnerNativeLang;
    m_translator->translate(text, detectedLang, m_partnerNativeLang);
}

void CommunicationDialog::onTranscriptionError(const QString &error)
{
    ui->micButton->setChecked(false);
    updateStatus("Ошибка распознавания: " + error);
}

// Перевод → отправить собеседнику
void CommunicationDialog::onTranslationFinished(const QString &translatedText)
{
    ui->translatedTextEdit->setPlainText(translatedText);

    if (m_ws) {
        m_ws->sendTranslation(m_myUserId, translatedText, m_targetLang);
    }

    updateStatus("Озвучка...");
    m_speakingOwnTranslation = true;
    m_tts->speak(translatedText, m_targetLang);
}

// Ошибка перевода — проверка на отсутствие интернета
void CommunicationDialog::onTranslationError(const QString &error)
{
    if (error.contains("network error", Qt::CaseInsensitive) ||
        error.contains("Host not found", Qt::CaseInsensitive) ||
        error.contains("Connection refused", Qt::CaseInsensitive) ||
        error.contains("timeout", Qt::CaseInsensitive) ||
        error.contains("network", Qt::CaseInsensitive) ||
        error.contains("Temporary failure", Qt::CaseInsensitive) ||
        error.contains("Could not connect", Qt::CaseInsensitive)) {

        QMessageBox::warning(this, "Нет интернета",
                             "Не удалось выполнить перевод.\n\n"
                             "Проверьте подключение к интернету.\n"
                             "Распознавание речи работает без интернета, "
                             "однако для перевода требуется подключение.");

        updateStatus("⚠ Нет интернета — перевод недоступен");
    } else {
        updateStatus("Ошибка перевода: " + error);
    }
}

// Озвучка
void CommunicationDialog::onSpeakingFinished()
{
    if (m_speakingOwnTranslation) {
        m_speakingOwnTranslation = false;
        updateStatus("Готово. Можете говорить снова");
    } else {
        updateStatus("Собеседник закончил говорить");
    }
}

void CommunicationDialog::onSpeakingError(const QString &error)
{
    updateStatus("Ошибка озвучки: " + error);
}

// Пришёл перевод от собеседника
void CommunicationDialog::onIncomingTranslation(int fromId,
                                                const QString &text,
                                                const QString &language)
{
    Q_UNUSED(fromId)
    qDebug() << "[Comm] Перевод от собеседника:" << text;

    ui->recognizedTextEdit->setPlainText("💬 " + text);

    m_speakingOwnTranslation = false;
    updateStatus("Собеседник говорит...");
    m_tts->speak(text, language);
}

void CommunicationDialog::onPartnerDisconnected(int userId)
{
    Q_UNUSED(userId)
    QMessageBox::information(this, "Разговор завершён",
                             "Собеседник завершил разговор.");
    reject();
}

void CommunicationDialog::updateStatus(const QString &text)
{
    ui->statusLabel->setText(text);
    qDebug() << "[Comm]" << text;
}