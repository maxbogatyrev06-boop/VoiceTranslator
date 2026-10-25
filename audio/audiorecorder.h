#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QMediaCaptureSession>
#include <QAudioInput>
#include <QMediaRecorder>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QMediaFormat>

class AudioRecorder : public QObject
{
    Q_OBJECT
public:
    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder();

    void startRecording();
    void stopRecording();
    bool isRecording() const;
    QString getLastAudioFilePath() const;

signals:
    void recordingFinished(const QString &filePath);
    void recordingError(const QString &error);

private slots:
    void onRecorderStateChanged(QMediaRecorder::RecorderState state);

private:
    QMediaCaptureSession m_session;
    QAudioInput m_audioInput;
    QMediaRecorder m_recorder;
    QString m_currentAudioFile;
    bool m_isRecording;
};

#endif