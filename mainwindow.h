#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QMap>
#include "network/networkmanager.h"
#include "network/websocketclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(NetworkManager *network,
                        int userId,
                        const QString &username,
                        const QString &nativeLanguage,
                        const QString &country,
                        QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onUsersReceived(const QJsonArray &users);
    void onCountriesReceived(const QStringList &countries);
    void onRequestError(const QString &error);
    void onSearchTextChanged(const QString &text);
    void onCountryFilterChanged(int index);
    void onHeartbeatTimer();
    void onRefreshTimer();

    void onWsConnected();
    void onWsError(const QString &error);
    void onIncomingInvite(int fromId,
                          const QString &fromUsername,
                          const QString &fromCountry,
                          const QString &fromLanguage);
    void onCallStarted(int partnerId,
                       const QString &partnerUsername,
                       const QString &partnerCountry,
                       const QString &partnerLanguage);
    void onInviteRejected(int byUserId);
    void onInviteFailed(const QString &reason);
    void onPartnerDisconnected(int userId);

    void onProfileClicked();
    void onUserDoubleClicked(QListWidgetItem *item);

    // Случайный собеседник
    void onRandomClicked();
    void onWaitingForPartner();
    void onSearchCancelled();

private:
    Ui::MainWindow *ui;
    NetworkManager *m_network;
    WebSocketClient *m_ws;

    int m_userId;
    QString m_username;
    QString m_nativeLanguage;
    QString m_country;

    QJsonArray m_allUsers;
    QTimer *m_heartbeatTimer;
    QTimer *m_refreshTimer;
    QMap<int, QPixmap> m_avatarCache;
    bool m_searching = false;

    void refreshUsersList();
    void updateStatus(const QString &text);
    void loadMyAvatar();
    void loadAvatarForUser(int userId);
    QPixmap makeCircularPixmap(const QPixmap &source, int diameter);
};

#endif // MAINWINDOW_H