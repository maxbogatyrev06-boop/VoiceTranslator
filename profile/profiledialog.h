#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include "network/networkmanager.h"

namespace Ui { class ProfileDialog; }

class ProfileDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProfileDialog(NetworkManager *network,
                           int userId,
                           const QString &username,
                           const QString &country,
                           const QString &nativeLanguage,
                           const QString &birthdate,
                           QWidget *parent = nullptr);
    ~ProfileDialog();

    bool wasUpdated() const { return m_wasUpdated; }

signals:
    void profileUpdated(const QString &newUsername,
                        const QString &newCountry,
                        const QString &newLanguage);

private slots:
    void onAvatarClicked();
    void onSaveClicked();
    void onLogoutClicked();

private:
    Ui::ProfileDialog *ui;
    NetworkManager *m_network;
    int m_userId;
    bool m_wasUpdated;

    QString languageCodeFromCombo() const;
    void loadAvatar();
};

#endif // PROFILEDIALOG_H