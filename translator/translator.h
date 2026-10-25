#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>       // ← ЭТА СТРОКА БЫЛА ПРОПУЩЕНА

/**
 * @brief Переводит текст через каскад из трёх сервисов:
 *   1. Google Translate (напрямую)
 *   2. Lingva Translate (прокси Google)
 *   3. MyMemory
 *
 * Различает сетевые ошибки (нет интернета) и API-ошибки.
 */
class Translator : public QObject
{
    Q_OBJECT
public:
    explicit Translator(QObject *parent = nullptr);

    void translate(const QString &text,
                   const QString &fromLang,
                   const QString &toLang);

    bool isBusy() const;

signals:
    void translationFinished(const QString &translatedText);
    void translationError(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_manager;
    bool m_busy;

    // Отслеживаем, были ли ошибки сетевыми
    bool m_lastErrorWasNetwork;
    QString m_lastNetworkError;

    void tryGoogle(const QString &text, const QString &fromLang, const QString &toLang);
    void tryLingva(const QString &text, const QString &fromLang, const QString &toLang);
    void tryMyMemory(const QString &text, const QString &fromLang, const QString &toLang);

    QString serviceFromUrl(const QUrl &url) const;
    bool isNetworkError(QNetworkReply::NetworkError err) const;
};

#endif // TRANSLATOR_H