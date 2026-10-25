#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>

class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();

    void connectToServer(const QString &url, int userId);
    void disconnectFromServer();

    bool isConnected() const;

    void inviteUser(int fromId, int toId);
    void respondToInvite(int fromId, int toId, bool accepted);
    void sendTranslation(int fromId, const QString &text, const QString &language);
    void endCall(int userId);

    // Случайный собеседник
    void findRandom(int userId);
    void cancelSearch(int userId);

signals:
    void connected();
    void disconnected();
    void registered();

    void incomingInvite(int fromId,
                        const QString &fromUsername,
                        const QString &fromCountry,
                        const QString &fromLanguage);
    void inviteRejected(int byUserId);
    void inviteFailed(const QString &reason);

    void callStarted(int partnerId,
                     const QString &partnerUsername,
                     const QString &partnerCountry,
                     const QString &partnerLanguage);

    void incomingTranslation(int fromId,
                             const QString &text,
                             const QString &language);

    void partnerDisconnected(int userId);

    // Поиск
    void waitingForPartner();
    void searchCancelled();

    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);

private:
    QWebSocket *m_socket;
    int m_userId;
    QString m_serverUrl;
};

#endif // WEBSOCKETCLIENT_H