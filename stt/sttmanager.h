#ifndef STTMANAGER_H
#define STTMANAGER_H

#include <QObject>
#include <QProcess>


class STTManager : public QObject
{
    Q_OBJECT
public:
    explicit STTManager(QObject *parent = nullptr);
    ~STTManager();

    void transcribe(const QString &audioFilePath, const QString &language = "ru");

    bool isBusy() const;

signals:
    // Успешное распознавание
    void transcriptionFinished(const QString &text,
                               const QString &language,
                               double timeSec);
    // Ошибка
    void transcriptionError(const QString &error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    QProcess *m_process;
    QString m_pythonScriptPath;   // путь к whisper_stt.py
    QString m_pythonExecutable;   // путь к python.exe 
};

#endif 