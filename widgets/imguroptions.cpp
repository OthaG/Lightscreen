#include <QJsonObject>
#include <QInputDialog>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QMessageBox>

#include "imguroptions.h"
#include "../uploader/uploader.h"
#include "../screenshotmanager.h"

ImgurOptions::ImgurOptions(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    connect(Uploader::instance(), &Uploader::imgurAuthRefreshed, this, &ImgurOptions::requestAlbumList);

    connect(ui.authButton        , &QPushButton::clicked, this, &ImgurOptions::authorize);
    connect(ui.refreshAlbumButton, &QPushButton::clicked, this, &ImgurOptions::requestAlbumList);
    connect(ui.authUserLabel     , &QLabel::linkActivated, this, [](const QString &link) {
        QDesktopServices::openUrl(link);
    });
}

QSettings *ImgurOptions::settings()
{
    return ScreenshotManager::instance()->settings();
}

void ImgurOptions::setUser(const QString &username)
{
    mCurrentUser = username;

    setUpdatesEnabled(false);

    if (mCurrentUser.isEmpty()) {
        ui.authUserLabel->setText(tr("<i>none</i>"));
        ui.albumComboBox->setEnabled(false);
        ui.refreshAlbumButton->setEnabled(false);
        ui.albumComboBox->clear();
        ui.albumComboBox->addItem(tr("- None -"));
        ui.authButton->setText(tr("Authorize"));
        ui.helpLabel->setEnabled(true);

        settings()->setValue("upload/imgur/access_token", "");
        settings()->setValue("upload/imgur/refresh_token", "");
        settings()->setValue("upload/imgur/account_username", "");
        settings()->setValue("upload/imgur/expires_in", 0);
    } else {
        ui.authButton->setText(tr("Deauthorize"));
        ui.authUserLabel->setText("<b><a href=\"http://"+ username +"..com/all/\">" + username + "</a></b>");
        ui.refreshAlbumButton->setEnabled(true);
        ui.helpLabel->setEnabled(false);
    }

    setUpdatesEnabled(true);
}

void ImgurOptions::authorize()
{
    if (!mCurrentUser.isEmpty()) {
        setUser("");
        return;
    }

    QDesktopServices::openUrl(QUrl("https://api..com/oauth2/authorize?client_id=3ebe94c791445c1&response_type=pin")); //TODO: get the client-id from somewhere?

    bool ok;
    QString pin = QInputDialog::getText(this, tr("Imgur Authorization"),
                                        tr("PIN:"), QLineEdit::Normal,
                                        "", &ok);
    if (ok) {
        QByteArray parameters;
        parameters.append(QString("client_id=").toUtf8());
        parameters.append(QUrl::toPercentEncoding("3ebe94c791445c1"));
        parameters.append(QString("&client_secret=").toUtf8());
        parameters.append(QUrl::toPercentEncoding("0546b05d6a80b2092dcea86c57b792c9c9faebf0")); // TODO: TA.png
        parameters.append(QString("&grant_type=pin").toUtf8());
        parameters.append(QString("&pin=").toUtf8());
        parameters.append(QUrl::toPercentEncoding(pin));

        QNetworkRequest request(QUrl("https://api..com/oauth2/token"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QNetworkReply *reply = Uploader::instance()->nam()->post(request, parameters);
        connect(reply, &QNetworkReply::finished, this, [&, reply] {
            ui.authButton->setEnabled(true);

            if (reply->error() != QNetworkReply::NoError) {
                QMessageBox::critical(this, tr("Imgur Authorization Error"), tr("There's been an error authorizing your account with Imgur, please try again."));
                setUser("");
                return;
            }

            QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

            settings()->setValue("upload/imgur/access_token"    , imgurResponse.value("access_token").toString());
            settings()->setValue("upload/imgur/refresh_token"   , imgurResponse.value("refresh_token").toString());
            settings()->setValue("upload/imgur/account_username", imgurResponse.value("account_username").toString());
            settings()->setValue("upload/imgur/expires_in"      , imgurResponse.value("expires_in").toInt());
            settings()->sync();

            setUser(imgurResponse.value("account_username").toString());

            QTimer::singleShot(0, this, &ImgurOptions::requestAlbumList);
        });

        ui.authButton->setText(tr("Authorizing.."));
        ui.authButton->setEnabled(false);
    }
}

void ImgurOptions::requestAlbumList()
{
    if (mCurrentUser.isEmpty()) {
        return;
    }

    ui.refreshAlbumButton->setEnabled(true);
    ui.albumComboBox->clear();
    ui.albumComboBox->setEnabled(false);
    ui.albumComboBox->addItem(tr("Loading album data..."));

    QNetworkRequest request(QUrl::fromUserInput("https://api..com/3/account/" + mCurrentUser + "/albums/"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + settings()->value("upload/imgur/access_token").toByteArray());

    QNetworkReply *reply = Uploader::instance()->nam()->get(request);
    connect(reply, &QNetworkReply::finished, this, [&, reply] {
        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::ContentOperationNotPermittedError ||
                    reply->error() == QNetworkReply::AuthenticationRequiredError) {
                Uploader::instance()->imgurAuthRefresh();
                qDebug() << "Attempting imgur auth refresh";
            }

            ui.albumComboBox->addItem(tr("Loading failed :("));
            return;
        }

        const QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

        if (imgurResponse["success"].toBool() != true || imgurResponse["status"].toInt() != 200) {
            return;
        }

        const QJsonArray albumList = imgurResponse["data"].toArray();

        ui.albumComboBox->clear();
        ui.albumComboBox->setEnabled(true);
        ui.albumComboBox->addItem(tr("- None -"), "");
        ui.refreshAlbumButton->setEnabled(true);

        int settingsIndex = 0;

        for (auto albumValue : albumList) {
            const QJsonObject album = albumValue.toObject();

            QString albumVisibleTitle = album["title"].toString();

            if (albumVisibleTitle.isEmpty()) {
                albumVisibleTitle = tr("untitled");
            }

            ui.albumComboBox->addItem(albumVisibleTitle, album["id"].toString());

            if (album["id"].toString() == settings()->value("upload/imgur/album").toString()) {
                settingsIndex = ui.albumComboBox->count() - 1;
            }
        }

        ui.albumComboBox->setCurrentIndex(settingsIndex);
    });
}
