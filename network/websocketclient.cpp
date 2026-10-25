#include "websocketclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent),
    m_socket(new QWebSocket()),
    m_userId(-1)
{
    connect(m_socket, &QWebSocket::connected,
            this, &WebSocketClient::onConnected);
    connect(m_socket, &QWebSocket::disconnected,
            this, &WebSocketClient::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived,
            this, &WebSocketClient::onTextMessageReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &WebSocketClient::onError);
}

WebSocketClient::~WebSocketClient()
{
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
    }
}

void WebSocketClient::connectToServer(const QString &url, int userId)
{
    m_serverUrl = url;
    m_userId = userId;
    qDebug() << "[WS] Подключаюсь к" << url;
    m_socket->open(QUrl(url));
}

void WebSocketClient::disconnectFromServer()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->close();
    }
}

bool WebSocketClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

// Вспомогательная
static void sendEvent(QWebSocket *socket, const QString &event, const QJsonObject &data)
{
    QJsonObject payload = data;
    payload["event"] = event;
    QJsonDocument doc(payload);
    socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

// Действия
void WebSocketClient::inviteUser(int fromId, int toId)
{
    QJsonObject data;
    data["from_id"] = fromId;
    data["to_id"] = toId;
    sendEvent(m_socket, "invite_user", data);
    qDebug() << "[WS] Приглашение:" << fromId << "->" << toId;
}

void WebSocketClient::respondToInvite(int fromId, int toId, bool accepted)
{
    QJsonObject data;
    data["from_id"] = fromId;
    data["to_id"] = toId;
    data["accepted"] = accepted;
    sendEvent(m_socket, "invite_response", data);
}

void WebSocketClient::sendTranslation(int fromId, const QString &text, const QString &language)
{
    QJsonObject data;
    data["from_id"] = fromId;
    data["text"] = text;
    data["language"] = language;
    sendEvent(m_socket, "translation", data);
    qDebug() << "[WS] Отправлен перевод:" << text;
}

void WebSocketClient::endCall(int userId)
{
    QJsonObject data;
    data["user_id"] = userId;
    sendEvent(m_socket, "end_call", data);
}

void WebSocketClient::findRandom(int userId)
{
    QJsonObject data;
    data["user_id"] = userId;
    sendEvent(m_socket, "find_random", data);
    qDebug() << "[WS] Отправлен поиск случайного собеседника";
}

void WebSocketClient::cancelSearch(int userId)
{
    QJsonObject data;
    data["user_id"] = userId;
    sendEvent(m_socket, "cancel_search", data);
    qDebug() << "[WS] Отправлена отмена поиска";
}

// Системные события
void WebSocketClient::onConnected()
{
    qDebug() << "[WS] Подключён к серверу";
    emit connected();

    if (m_userId > 0) {
        QJsonObject data;
        data["user_id"] = m_userId;
        sendEvent(m_socket, "register", data);
        qDebug() << "[WS] Регистрация user_id =" << m_userId;
    }
}

void WebSocketClient::onDisconnected()
{
    qDebug() << "[WS] Отключён от сервера";
    emit disconnected();
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    QString err = m_socket->errorString();
    qWarning() << "[WS] Ошибка:" << err;
    emit errorOccurred(err);
}

// Приём сообщений
void WebSocketClient::onTextMessageReceived(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString event = obj["event"].toString();

    qDebug() << "[WS] Получено событие:" << event;

    if (event == "registered") {
        emit registered();
    }
    else if (event == "incoming_invite") {
        emit incomingInvite(
            obj["from_id"].toInt(),
            obj["from_username"].toString(),
            obj["from_country"].toString(),
            obj["from_language"].toString()
            );
    }
    else if (event == "invite_rejected") {
        emit inviteRejected(obj["by_user_id"].toInt());
    }
    else if (event == "invite_failed") {
        emit inviteFailed(obj["reason"].toString());
    }
    else if (event == "call_started") {
        emit callStarted(
            obj["partner_id"].toInt(),
            obj["partner_username"].toString(),
            obj["partner_country"].toString(),
            obj["partner_language"].toString()
            );
    }
    else if (event == "incoming_translation") {
        emit incomingTranslation(
            obj["from_id"].toInt(),
            obj["text"].toString(),
            obj["language"].toString()
            );
    }
    else if (event == "partner_disconnected") {
        emit partnerDisconnected(obj["user_id"].toInt());
    }
    else if (event == "waiting") {
        emit waitingForPartner();
    }
    else if (event == "search_cancelled") {
        emit searchCancelled();
    }
    else if (event == "connected") {
        // игнорируем
    }
}