#include "nearbydialog.h"
#include "ui_nearbydialog.h"
#include <QMessageBox>
#include <QComboBox>
#include <QDebug>

NearbyDialog::NearbyDialog(const QString &myNativeLang, QWidget *parent)
    : QDialog(parent),
    ui(new Ui::NearbyDialog),
    m_myLang(myNativeLang)
{
    ui->setupUi(this);

    QStringList langNames = {
        "Русский", "English", "Deutsch", "Español", "Français",
        "Italiano", "Português", "中文", "日本語", "한국어",
        "العربية", "Türkçe", "Polski", "Nederlands"
    };

    ui->myLangCombo->addItems(langNames);
    ui->otherLangCombo->addItems(langNames);

    static const QStringList codes = {
        "ru", "en", "de", "es", "fr", "it", "pt",
        "zh", "ja", "ko", "ar", "tr", "pl", "nl"
    };
    int myIdx = codes.indexOf(myNativeLang);
    if (myIdx >= 0) ui->myLangCombo->setCurrentIndex(myIdx);

    ui->otherLangCombo->setCurrentIndex(1);

    connect(ui->myLangCombo, &QComboBox::currentIndexChanged,
            this, &NearbyDialog::onLanguageChanged);
    connect(ui->otherLangCombo, &QComboBox::currentIndexChanged,
            this, &NearbyDialog::onLanguageChanged);

    m_recorder   = new AudioRecorder(this);
    m_stt        = new STTManager(this);
    m_translator = new Translator(this);
    m_tts        = new TTSManager(this);

    connect(ui->micButton, &QPushButton::toggled,
            this, &NearbyDialog::onMicToggled);
    connect(ui->endButton, &QPushButton::clicked,
            this, &NearbyDialog::onEndClicked);

    connect(m_recorder, &AudioRecorder::recordingFinished,
            this, &NearbyDialog::onRecordingFinished);
    connect(m_recorder, &AudioRecorder::recordingError,
            this, &NearbyDialog::onRecordingError);

    connect(m_stt, &STTManager::transcriptionFinished,
            this, &NearbyDialog::onTranscriptionFinished);
    connect(m_stt, &STTManager::transcriptionError,
            this, &NearbyDialog::onTranscriptionError);

    connect(m_translator, &Translator::translationFinished,
            this, &NearbyDialog::onTranslationFinished);
    connect(m_translator, &Translator::translationError,
            this, &NearbyDialog::onTranslationError);

    connect(m_tts, &TTSManager::speakingFinished,
            this, &NearbyDialog::onSpeakingFinished);
    connect(m_tts, &TTSManager::speakingError,
            this, &NearbyDialog::onSpeakingError);

    onLanguageChanged();
    updateStatus("Выберите языки и нажмите 🎤");
}

NearbyDialog::~NearbyDialog()
{
    delete ui;
}

QString NearbyDialog::languageCodeFromCombo(QComboBox *combo) const
{
    static const QStringList codes = {
        "ru", "en", "de", "es", "fr", "it", "pt",
        "zh", "ja", "ko", "ar", "tr", "pl", "nl"
    };
    int i = combo->currentIndex();
    return (i >= 0 && i < codes.size()) ? codes[i] : "en";
}

void NearbyDialog::onLanguageChanged()
{
    m_myLang    = languageCodeFromCombo(ui->myLangCombo);
    m_otherLang = languageCodeFromCombo(ui->otherLangCombo);

    qDebug() << "[Nearby] Мой язык:" << m_myLang
             << "| Язык друга:" << m_otherLang;

    if (m_myLang == m_otherLang) {
        updateStatus("⚠ Выберите разные языки");
    } else {
        updateStatus("Готово. Говорите — определю язык автоматически");
    }
}

// Микрофон
void NearbyDialog::onMicToggled(bool checked)
{
    if (m_myLang == m_otherLang) {
        ui->micButton->setChecked(false);
        QMessageBox::warning(this, "Ошибка",
                             "Выберите разные языки для собеседников");
        return;
    }

    if (checked) {
        ui->recognizedTextEdit->clear();
        ui->translatedTextEdit->clear();
        m_recorder->startRecording();
        ui->micButton->setToolTip("Остановить запись");
        updateStatus("🔴 Идёт запись... Говорите");
    } else {
        m_recorder->stopRecording();
        ui->micButton->setToolTip("Начать запись");
        updateStatus("Обработка...");
    }
}

void NearbyDialog::onEndClicked()
{
    if (m_recorder->isRecording()) m_recorder->stopRecording();
    if (m_tts->isSpeaking()) m_tts->stop();
    reject();
}

// Запись завершена → распознавание
void NearbyDialog::onRecordingFinished(const QString &filePath)
{
    updateStatus("Распознавание речи...");
    m_stt->transcribe(filePath, "");
}

void NearbyDialog::onRecordingError(const QString &error)
{
    ui->micButton->setChecked(false);
    updateStatus("Ошибка записи: " + error);
}

// Распознано → перевод
void NearbyDialog::onTranscriptionFinished(const QString &text,
                                           const QString &detectedLang)
{
    m_detectedLang = detectedLang;
    ui->recognizedTextEdit->setPlainText(
        QString("[%1] %2").arg(detectedLang, text));

    qDebug() << "[Nearby] Распознано:" << text
             << "| Язык:" << detectedLang;

    if (detectedLang == m_myLang) {
        m_targetLang = m_otherLang;
    } else if (detectedLang == m_otherLang) {
        m_targetLang = m_myLang;
    } else {
        m_targetLang = m_otherLang;
    }

    updateStatus(QString("Перевод %1 → %2...")
                     .arg(detectedLang, m_targetLang));

    m_translator->translate(text, detectedLang, m_targetLang);
}

void NearbyDialog::onTranscriptionError(const QString &error)
{
    ui->micButton->setChecked(false);
    updateStatus("Ошибка распознавания: " + error);
}

// Перевод → озвучка
void NearbyDialog::onTranslationFinished(const QString &translatedText)
{
    ui->translatedTextEdit->setPlainText(translatedText);
    updateStatus("Озвучка...");
    m_tts->speak(translatedText, m_targetLang);
}

// Ошибка перевода — проверка на отсутствие интернета
void NearbyDialog::onTranslationError(const QString &error)
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
void NearbyDialog::onSpeakingFinished()
{
    updateStatus("Готово. Можете говорить снова");
}

void NearbyDialog::onSpeakingError(const QString &error)
{
    updateStatus("Ошибка озвучки: " + error);
}

// Статус
void NearbyDialog::updateStatus(const QString &text)
{
    ui->statusLabel->setText(text);
    qDebug() << "[Nearby]" << text;
}