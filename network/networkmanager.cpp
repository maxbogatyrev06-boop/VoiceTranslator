#include "networkmanager.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent),
      m_serverUrl("http://localhost:5000")
{
    m_manager = new QNetworkAccessManager(this);
}

void NetworkManager::setServerUrl(const QString &url)
{
    m_serverUrl = url;
}

QString NetworkManager::serverUrl() const
{
    return m_serverUrl;
}

// ВСПОМОГАТЕЛЬНЫЙ МЕТОД: отправка POST-запроса
static QNetworkRequest makeJsonRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return request;
}

// РЕГИСТРАЦИЯ
void NetworkManager::registerUser(const QString &email,
                                  const QString &phone,
                                  const QString &username,
                                  const QString &password,
                                  const QString &birthdate,
                                  const QString &country,
                                  const QString &nativeLanguage)
{
    QJsonObject body;
    body["email"] = email;
    body["phone"] = phone;
    body["username"] = username;
    body["password"] = password;
    body["birthdate"] = birthdate;
    body["country"] = country;
    body["native_language"] = nativeLanguage;

    QUrl url(m_serverUrl + "/api/register");
    QNetworkRequest request = makeJsonRequest(url);

    QNetworkReply *reply = m_manager->post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        if (reply->error() != QNetworkReply::NoError) {
            QString err = obj.contains("error")
                              ? obj["error"].toString()
                              : reply->errorString();
            emit requestError("Регистрация: " + err);
            return;
        }

        emit registerSuccess(obj["user_id"].toInt(),
                             obj["debug_code"].toString());
    });
}

// ПОДТВЕРЖДЕНИЕ КОДА
void NetworkManager::verifyCode(int userId, const QString &code)
{
    QJsonObject body;
    body["user_id"] = userId;
    body["code"] = code;

    QUrl url(m_serverUrl + "/api/verify");
    QNetworkRequest request = makeJsonRequest(url);

    QNetworkReply *reply = m_manager->post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonObject obj = QJsonDocument::fromJson(data).object();

        if (reply->error() != QNetworkReply::NoError) {
            QString err = obj.contains("error")
                              ? obj["error"].toString()
                              : reply->errorString();
            emit requestError("Подтверждение: " + err);
            return;
        }

        emit verifySuccess(obj["user_id"].toInt(), "");
    });
}

// ВХОД
void NetworkManager::login(const QString &loginField, const QString &password)
{
    QJsonObject body;
    body["login"] = loginField;
    body["password"] = password;

    QUrl url(m_serverUrl + "/api/login");
    QNetworkRequest request = makeJsonRequest(url);

    QNetworkReply *reply = m_manager->post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonObject obj = QJsonDocument::fromJson(data).object();

        if (reply->error() != QNetworkReply::NoError) {
            QString err = obj.contains("error")
                              ? obj["error"].toString()
                              : reply->errorString();
            emit requestError("Вход: " + err);
            return;
        }

        emit loginSuccess(obj["user_id"].toInt(),
                          obj["username"].toString(),
                          obj["native_language"].toString(),
                          obj["country"].toString());
    });
}

// СПИСОК ОНЛАЙН-ПОЛЬЗОВАТЕЛЕЙ
void NetworkManager::fetchOnlineUsers(const QString &countryFilter)
{
    QUrl url(m_serverUrl + "/api/users");
    if (!countryFilter.isEmpty()) {
        QUrlQuery query;
        query.addQueryItem("country", countryFilter);
        url.setQuery(query);
    }

    QNetworkReply *reply = m_manager->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit requestError("Список пользователей: " + reply->errorString());
            return;
        }

        QJsonArray arr = QJsonDocument::fromJson(data).array();
        emit onlineUsersReceived(arr);
    });
}

// СПИСОК СТРАН
void NetworkManager::fetchCountries()
{
    QUrl url(m_serverUrl + "/api/countries");
    QNetworkReply *reply = m_manager->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit requestError("Список стран: " + reply->errorString());
            return;
        }

        QJsonArray arr = QJsonDocument::fromJson(data).array();
        QStringList list;
        for (const QJsonValue &v : arr) {
            list << v.toString();
        }
        emit countriesReceived(list);
    });
}

// HEARTBEAT (онлайн-статус)
void NetworkManager::sendHeartbeat(int userId)
{
    QJsonObject body;
    body["user_id"] = userId;

    QUrl url(m_serverUrl + "/api/heartbeat");
    QNetworkRequest request = makeJsonRequest(url);

    QNetworkReply *reply = m_manager->post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit heartbeatSent();
        }
    });
}