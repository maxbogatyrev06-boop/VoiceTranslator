#ifndef NEARBYDIALOG_H
#define NEARBYDIALOG_H

#include <QDialog>
#include "audio/audiorecorder.h"
#include "stt/sttmanager.h"
#include "translator/translator.h"
#include "tts/ttsmanager.h"

namespace Ui { class NearbyDialog; }

class NearbyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NearbyDialog(const QString &myNativeLang,
                          QWidget *parent = nullptr);
    ~NearbyDialog();

private slots:
    void onMicToggled(bool checked);
    void onEndClicked();
    void onLanguageChanged();

    void onRecordingFinished(const QString &filePath);
    void onRecordingError(const QString &error);

    void onTranscriptionFinished(const QString &text, const QString &language);
    void onTranscriptionError(const QString &error);

    void onTranslationFinished(const QString &translatedText);
    void onTranslationError(const QString &error);

    void onSpeakingFinished();
    void onSpeakingError(const QString &error);

private:
    Ui::NearbyDialog *ui;
    AudioRecorder *m_recorder;
    STTManager *m_stt;
    Translator *m_translator;
    TTSManager *m_tts;

    QString m_myLang;
    QString m_otherLang;
    QString m_detectedLang;
    QString m_targetLang;

    void updateStatus(const QString &text);
    QString languageCodeFromCombo(class QComboBox *combo) const;
};

#endif // NEARBYDIALOG_H