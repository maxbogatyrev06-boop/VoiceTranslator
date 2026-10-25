#include "ttsmanager.h"
#include <QLocale>
#include <QDebug>

TTSManager::TTSManager(QObject *parent)
    : QObject(parent)
{
    m_speech = new QTextToSpeech(this);

    connect(m_speech, &QTextToSpeech::stateChanged,
            this, &TTSManager::onStateChanged);

    // Выводим список доступных голосов (для отладки)
    qDebug() << "Доступные голоса:";
    for (const QVoice &voice : m_speech->availableVoices()) {
        qDebug() << "  " << voice.name()
                 << "| язык:" << voice.locale().name();
    }
}

TTSManager::~TTSManager()
{
    if (m_speech->state() == QTextToSpeech::Speaking) {
        m_speech->stop();
    }
}

void TTSManager::speak(const QString &text, const QString &language)
{
    if (text.isEmpty()) {
        emit speakingError("Пустой текст для озвучки");
        return;
    }

    // Устанавливаем язык
    QLocale locale(language);
    if (m_speech->availableLocales().contains(locale)) {
        m_speech->setLocale(locale);
    } else {
        qWarning() << "Язык" << language << "не поддерживается, использую системный";
    }

    // Скорость и высота голоса (0 — по умолчанию)
    m_speech->setRate(0.0);
    m_speech->setPitch(0.0);
    m_speech->setVolume(1.0);

    qDebug() << "[TTS] Озвучиваю:" << text << "| Язык:" << language;

    m_speech->say(text);
}

void TTSManager::stop()
{
    m_speech->stop();
}

bool TTSManager::isSpeaking() const
{
    return m_speech->state() == QTextToSpeech::Speaking;
}

void TTSManager::onStateChanged(QTextToSpeech::State state)
{
    if (state == QTextToSpeech::Ready) {
        emit speakingFinished();
    }
}