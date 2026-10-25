#ifndef COMMUNICATIONDIALOG_H
#define COMMUNICATIONDIALOG_H

#include <QDialog>
#include "audio/audiorecorder.h"
#include "stt/sttmanager.h"
#include "translator/translator.h"
#include "tts/ttsmanager.h"
#include "network/websocketclient.h"

namespace Ui { class CommunicationDialog; }

class CommunicationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CommunicationDialog(int myUserId,
                                 int partnerId,
                                 const QString &partnerUsername,
                                 const QString &partnerCountry,
                                 const QString &myNativeLang,
                                 const QString &partnerNativeLang,
                                 WebSocketClient *ws,
                                 QWidget *parent = nullptr);
    ~CommunicationDialog();

private slots:
    void onMicToggled(bool checked);
    void onEndClicked();

    void onRecordingFinished(const QString &filePath);
    void onRecordingError(const QString &error);

    void onTranscriptionFinished(const QString &text, const QString &language);
    void onTranscriptionError(const QString &error);

    void onTranslationFinished(const QString &translatedText);
    void onTranslationError(const QString &error);

    void onSpeakingFinished();
    void onSpeakingError(const QString &error);

    void onIncomingTranslation(int fromId,
                               const QString &text,
                               const QString &language);
    void onPartnerDisconnected(int userId);

private:
    Ui::CommunicationDialog *ui;
    AudioRecorder *m_recorder;
    STTManager *m_stt;
    Translator *m_translator;
    TTSManager *m_tts;
    WebSocketClient *m_ws;

    int m_myUserId;
    int m_partnerId;
    QString m_partnerUsername;
    QString m_myNativeLang;
    QString m_partnerNativeLang;
    QString m_targetLang;

    bool m_speakingOwnTranslation;

    void loadPartnerAvatar();
    void updateStatus(const QString &text);
};

#endif // COMMUNICATIONDIALOG_H