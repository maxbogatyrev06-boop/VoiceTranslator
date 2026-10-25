#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>

/**
 * @brief Сетевой слой для общения с сервером VoiceTranslator.
 *
 * Инкапсулирует HTTP-запросы: регистрация, вход, подтверждение,
 * список пользователей, heartbeat, профиль.
 */
class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);

    // Базовый URL сервера (можно менять для теста на реальном устройстве)
    void setServerUrl(const QString &url);
    QString serverUrl() const;

    // === Регистрация и вход ===
    void registerUser(const QString &email,
                      const QString &phone,
                      const QString &username,
                      const QString &password,
                      const QString &birthdate,
                      const QString &country,
                      const QString &nativeLanguage);

    void verifyCode(int userId, const QString &code);

    void login(const QString &loginField, const QString &password);

    // === Пользователи и онлайн ===
    void fetchOnlineUsers(const QString &countryFilter = "");

    void fetchCountries();

    void sendHeartbeat(int userId);

signals:
    // Успешные ответы
    void registerSuccess(int userId, const QString &debugCode);
    void verifySuccess(int userId, const QString &username);
    void loginSuccess(int userId, const QString &username,
                      const QString &nativeLanguage, const QString &country);
    void onlineUsersReceived(const QJsonArray &users);
    void countriesReceived(const QStringList &countries);
    void heartbeatSent();

    // Ошибки (общие для всех запросов)
    void requestError(const QString &error);

private:
    QNetworkAccessManager *m_manager;
    QString m_serverUrl;
};

#endif // NETWORKMANAGER_H