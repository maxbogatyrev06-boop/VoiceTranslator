#ifndef TTSMANAGER_H
#define TTSMANAGER_H

#include <QObject>
#include <QTextToSpeech>


class TTSManager : public QObject
{
    Q_OBJECT
public:
    explicit TTSManager(QObject *parent = nullptr);
    ~TTSManager();

    // Произнести текст на указанном языке ("ru", "en", "de"...)
    void speak(const QString &text, const QString &language = "en");

    void stop();
    bool isSpeaking() const;

signals:
    void speakingFinished();
    void speakingError(const QString &error);

private slots:
    void onStateChanged(QTextToSpeech::State state);

private:
    QTextToSpeech *m_speech;
};

#endif // TTSMANAGER_H