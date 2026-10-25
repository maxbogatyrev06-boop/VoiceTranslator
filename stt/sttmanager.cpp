#include "sttmanager.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

STTManager::STTManager(QObject *parent)
    : QObject(parent),
      m_pythonExecutable("python")
{
    m_process = new QProcess(this);

    // Путь к Python-скрипту.
        m_pythonScriptPath = "C:/KYRSOVAY/VoiceTranslator/python_scripts/whisper_stt.py";

    if (!QFile::exists(m_pythonScriptPath)) {
        qWarning() << "Python-скрипт не найден:" << m_pythonScriptPath;
    }

    connect(m_process, &QProcess::finished,
            this, &STTManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &STTManager::onProcessError);
}

STTManager::~STTManager()
{
    if (m_process->state() == QProcess::Running) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

void STTManager::transcribe(const QString &audioFilePath, const QString &language)
{
    if (m_process->state() == QProcess::Running) {
        emit transcriptionError("Предыдущий процесс распознавания ещё не завершён");
        return;
    }

    if (!QFile::exists(audioFilePath)) {
        emit transcriptionError("Аудиофайл не найден: " + audioFilePath);
        return;
    }

    // Формируем аргументы: whisper_stt.py <аудиофайл> [язык]
    QStringList args;
    args << m_pythonScriptPath;
    args << audioFilePath;
    if (!language.isEmpty()) {
        args << language;
    }

    qDebug() << "Запуск Python:" << m_pythonExecutable << args;

    m_process->start(m_pythonExecutable, args);
}

bool STTManager::isBusy() const
{
    return m_process->state() == QProcess::Running;
}

void STTManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit) {
        emit transcriptionError("Python-процесс аварийно завершился");
        return;
    }

    // Читаем вывод
    QByteArray output = m_process->readAllStandardOutput();
    QByteArray errOutput = m_process->readAllStandardError();

    if (!errOutput.isEmpty()) {
        qDebug() << "Python stderr:" << errOutput;
    }

    if (exitCode != 0) {
        emit transcriptionError("Python вернул код ошибки: "
                                + QString::number(exitCode)
                                + "\n" + QString::fromUtf8(errOutput));
        return;
    }

    // Парсим JSON
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (doc.isNull()) {
        emit transcriptionError("Не удалось разобрать JSON. Вывод: "
                                + QString::fromUtf8(output));
        return;
    }

    QJsonObject obj = doc.object();

    if (obj.contains("error")) {
        emit transcriptionError(obj["error"].toString());
        return;
    }

    QString text = obj["text"].toString();
    QString language = obj["language"].toString();
    double timeSec = obj["time_sec"].toDouble();

    emit transcriptionFinished(text, language, timeSec);
}

void STTManager::onProcessError(QProcess::ProcessError error)
{
    QString message;
    switch (error) {
    case QProcess::FailedToStart:
        message = "Не удалось запустить Python. Проверьте, что он в PATH.";
        break;
    case QProcess::Crashed:
        message = "Python-процесс упал.";
        break;
    case QProcess::Timedout:
        message = "Превышено время ожидания.";
        break;
    default:
        message = "Неизвестная ошибка QProcess.";
        break;
    }
    emit transcriptionError(message);
}