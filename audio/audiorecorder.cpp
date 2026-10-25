#include "audiorecorder.h"
#include <QDateTime>
#include <QUrl>
#include <QDebug>
#include <QDir>

AudioRecorder::AudioRecorder(QObject *parent)
    : QObject(parent), m_isRecording(false)
{
    const auto devices = QMediaDevices::audioInputs();
    if (!devices.isEmpty()) {
        m_audioInput.setDevice(devices.first());
        qDebug() << "Микрофон:" << devices.first().description();
    } else {
        qWarning() << "Микрофон не найден!";
    }

    m_session.setAudioInput(&m_audioInput);
    m_session.setRecorder(&m_recorder);

    // Формат записи: WAV
    QMediaFormat format;
    format.setFileFormat(QMediaFormat::Wave);
    format.setAudioCodec(QMediaFormat::AudioCodec::Unspecified);
    m_recorder.setMediaFormat(format);

    m_recorder.setQuality(QMediaRecorder::NormalQuality);

    connect(&m_recorder, &QMediaRecorder::recorderStateChanged,
            this, &AudioRecorder::onRecorderStateChanged);

    connect(&m_recorder, &QMediaRecorder::errorOccurred,
            this, [this](QMediaRecorder::Error error, const QString &errorString){
        Q_UNUSED(error)
        emit recordingError(errorString);
    });
}

AudioRecorder::~AudioRecorder() {}

void AudioRecorder::startRecording()
{
    if (m_isRecording) return;

    QString dir = QDir::currentPath() + "/recordings";
    QDir().mkpath(dir);

    m_currentAudioFile = dir + "/record_" +
                         QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".wav";

    m_recorder.setOutputLocation(QUrl::fromLocalFile(m_currentAudioFile));
    m_recorder.record();
    m_isRecording = true;
}

void AudioRecorder::stopRecording()
{
    if (!m_isRecording) return;
    m_recorder.stop();
    m_isRecording = false;
}

bool AudioRecorder::isRecording() const
{
    return m_isRecording;
}

QString AudioRecorder::getLastAudioFilePath() const
{
    return m_currentAudioFile;
}

void AudioRecorder::onRecorderStateChanged(QMediaRecorder::RecorderState state)
{
    if (state == QMediaRecorder::StoppedState) {
        emit recordingFinished(m_currentAudioFile);
    }
}