#include "translator.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

Translator::Translator(QObject *parent)
    : QObject(parent),
    m_busy(false),
    m_lastErrorWasNetwork(false)
{
    m_manager = new QNetworkAccessManager(this);
}

// Определение сервиса по URL
QString Translator::serviceFromUrl(const QUrl &url) const
{
    QString host = url.host();
    if (host.contains("googleapis.com"))  return "google";
    if (host.contains("lingva"))          return "lingva";
    if (host.contains("mymemory"))        return "mymemory";
    return "unknown";
}

// Проверка: сетевая ли ошибка
bool Translator::isNetworkError(QNetworkReply::NetworkError err) const
{
    switch (err) {
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::TimeoutError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::UnknownNetworkError:
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
        return true;
    default:
        return false;
    }
}

// Главный метод
void Translator::translate(const QString &text,
                           const QString &fromLang,
                           const QString &toLang)
{
    if (m_busy) {
        emit translationError("Предыдущий перевод ещё не завершён");
        return;
    }

    if (text.isEmpty()) {
        emit translationError("Пустой текст для перевода");
        return;
    }

    m_busy = true;
    m_lastErrorWasNetwork = false;
    m_lastNetworkError.clear();

    tryGoogle(text, fromLang, toLang);
}

bool Translator::isBusy() const
{
    return m_busy;
}

// Ступень 1: Google
void Translator::tryGoogle(const QString &text,
                           const QString &fromLang,
                           const QString &toLang)
{
    QUrl url("https://translate.googleapis.com/translate_a/single");
    QUrlQuery query;
    query.addQueryItem("client", "gtx");
    query.addQueryItem("sl", fromLang);
    query.addQueryItem("tl", toLang);
    query.addQueryItem("dt", "t");
    query.addQueryItem("q", text);
    url.setQuery(query);

    qDebug() << "[Translator] Ступень 1 — Google:" << text;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");

    QNetworkReply *reply = m_manager->get(request);
    reply->setProperty("text", text);
    reply->setProperty("fromLang", fromLang);
    reply->setProperty("toLang", toLang);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

// Ступень 2: Lingva
void Translator::tryLingva(const QString &text,
                           const QString &fromLang,
                           const QString &toLang)
{
    QString encodedText = QString::fromUtf8(QUrl::toPercentEncoding(text));
    QString urlStr = QString("https://lingva.lunar.icu/api/v1/%1/%2/%3")
                         .arg(fromLang, toLang, encodedText);

    QUrl url(urlStr);
    qDebug() << "[Translator] Ступень 2 — Lingva:" << text;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");

    QNetworkReply *reply = m_manager->get(request);
    reply->setProperty("text", text);
    reply->setProperty("fromLang", fromLang);
    reply->setProperty("toLang", toLang);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

// Ступень 3: MyMemory
void Translator::tryMyMemory(const QString &text,
                             const QString &fromLang,
                             const QString &toLang)
{
    QUrl url("https://api.mymemory.translated.net/get");
    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("langpair", fromLang + "|" + toLang);
    url.setQuery(query);

    qDebug() << "[Translator] Ступень 3 — MyMemory:" << text;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "VoiceTranslator/1.0");

    QNetworkReply *reply = m_manager->get(request);
    reply->setProperty("text", text);
    reply->setProperty("fromLang", fromLang);
    reply->setProperty("toLang", toLang);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

// Обработка ответа
void Translator::onReplyFinished(QNetworkReply *reply)
{
    QString service = serviceFromUrl(reply->url());
    QString text     = reply->property("text").toString();
    QString fromLang = reply->property("fromLang").toString();
    QString toLang   = reply->property("toLang").toString();

    // Ошибка сети
    if (reply->error() != QNetworkReply::NoError) {
        QNetworkReply::NetworkError netErr = reply->error();
        QString errString = reply->errorString();

        if (isNetworkError(netErr)) {
            m_lastErrorWasNetwork = true;
            m_lastNetworkError = errString;
            qWarning() << "[Translator]" << service << "сетевая ошибка:" << errString;
        } else {
            qWarning() << "[Translator]" << service << "ошибка:" << errString;
        }

        reply->deleteLater();

        if (service == "google") {
            tryLingva(text, fromLang, toLang);
        } else if (service == "lingva") {
            tryMyMemory(text, fromLang, toLang);
        } else {
            m_busy = false;
            if (m_lastErrorWasNetwork) {
                emit translationError(
                    "network error: нет подключения к интернету (" + m_lastNetworkError + ")");
            } else {
                emit translationError("Все сервисы перевода недоступны");
            }
        }
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QString result;

    if (service == "google") {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            const QJsonArray root = doc.array();
            if (!root.isEmpty()) {
                const QJsonArray firstBlock = root[0].toArray();
                for (const QJsonValue &item : firstBlock) {
                    const QJsonArray frag = item.toArray();
                    if (!frag.isEmpty()) {
                        result += frag[0].toString();
                    }
                }
            }
        }
    }
    else if (service == "lingva") {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            result = doc.object().value("translation").toString();
        }
    }
    else if (service == "mymemory") {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            const QJsonObject obj = doc.object();
            if (obj.value("responseStatus").toInt() == 200) {
                const QJsonObject responseData = obj.value("responseData").toObject();
                result = responseData.value("translatedText").toString();
            }
        }
    }

    // Пустой результат → следующая ступень
    if (result.isEmpty() || result.startsWith("PLEASE SELECT")) {
        qWarning() << "[Translator]" << service << "вернул пустой результат";

        if (service == "google") {
            tryLingva(text, fromLang, toLang);
        } else if (service == "lingva") {
            tryMyMemory(text, fromLang, toLang);
        } else {
            m_busy = false;
            if (m_lastErrorWasNetwork) {
                emit translationError(
                    "network error: нет подключения к интернету");
            } else {
                emit translationError("Все сервисы вернули пустой результат");
            }
        }
        return;
    }

    // Успех
    m_busy = false;
    m_lastErrorWasNetwork = false;
    qDebug() << "[Translator]" << service << "→" << result;
    emit translationFinished(result);
}