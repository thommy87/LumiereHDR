/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * @author Daniel Kaneider
 */

#include <QtGlobal>
#include <QDebug>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QVersionNumber>

#include "Common/LuminanceOptions.h"
#include "Common/config.h"
#include "Common/global.h"
#include "UI/UMessageBox.h"
#include "UpdateChecker.h"

// releases are published on GitHub; only pages under this prefix are opened
static const char RELEASES_API[] =
    "https://api.github.com/repos/thommy87/LumiereHDR/releases/latest";
static const char RELEASES_PAGE_PREFIX[] =
    "https://github.com/thommy87/LumiereHDR/releases/";

UpdateChecker::UpdateChecker(
    QWidget *parent)  //, QNetworkAccessManager* networkManager)
    : QObject(parent),
      m_tray(new QSystemTrayIcon(parent->windowIcon())),
      m_networkManager(new QNetworkAccessManager()) {
    LuminanceOptions options;
    if (options.checkForUpdate()) {
        connect(this, SIGNAL(updateAvailable()), parent,
                SLOT(onUpdateAvailable()));
        connect(m_networkManager, &QNetworkAccessManager::finished, this,
                &UpdateChecker::requestFinished);
        connect(m_tray, &QSystemTrayIcon::messageClicked, this,
                &UpdateChecker::trayMessageClicked);
        connect(m_tray, &QSystemTrayIcon::activated, this,
                &UpdateChecker::trayMessageClicked);

        QNetworkRequest request{QUrl(QLatin1String(RELEASES_API))};
        request.setRawHeader("Accept", "application/vnd.github+json");
        request.setRawHeader("User-Agent", "LumiereHDR/" LUMINANCEVERSION);
        m_networkManager->get(request);
    }
}

UpdateChecker::~UpdateChecker() {
    m_tray->hide();
    delete m_tray;
    delete m_networkManager;
}

void UpdateChecker::trayMessageClicked() const {
    // the URL comes from the network: open it only if it is our release page
    if (m_latestUrl.startsWith(QLatin1String(RELEASES_PAGE_PREFIX))) {
        QDesktopServices::openUrl(QUrl(m_latestUrl));
    }
    m_tray->hide();
}

void UpdateChecker::requestFinished(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() == QNetworkReply::NoError) {
        LuminanceOptions options;
        options.setUpdateChecked();

        // GitHub "latest release" JSON: {"tag_name": "v2.7.0",
        // "html_url": "https://github.com/thommy87/LumiereHDR/releases/tag/v2.7.0"}
        const QJsonObject release =
            QJsonDocument::fromJson(reply->readAll()).object();
        QString tag = release.value(QStringLiteral("tag_name")).toString();
        const QString url = release.value(QStringLiteral("html_url")).toString();
        if (tag.startsWith(QLatin1Char('v'))) tag.remove(0, 1);

        const QVersionNumber latest = QVersionNumber::fromString(tag);
        if (!latest.isNull() &&
            url.startsWith(QLatin1String(RELEASES_PAGE_PREFIX))) {
            m_version = tag;
            m_latestUrl = url;

            // only announce releases newer than the running one
            if (latest <= QVersionNumber::fromString(
                              QLatin1String(LUMINANCEVERSION)))
                return;

            QString msgTitle =
                QStringLiteral("LumiereHDR %1").arg(m_version);
            QString msgContent = tr("A new release is ready for download!");
            QWidget *widgetP = (QWidget *)parent();

            emit updateAvailable();

#if defined(Q_OS_MACOS) || defined(Q_OS_X11)
            if (UMessageBox::question(
                    msgTitle,
                    msgContent + "\n\n" + tr("Do you want to open the "
                                             "webpage for download now?"),
                    widgetP) == QMessageBox::Yes) {
                trayMessageClicked();
            }
#else
            if (QSystemTrayIcon::supportsMessages()) {
                m_tray->setToolTip(widgetP->windowTitle());
                m_tray->show();
                m_tray->showMessage(
                    msgTitle,
                    msgContent + "\n" +
                        tr("Click to download, or select Help->Update!"));
            } else {
                if (UMessageBox::question(
                        msgTitle,
                        msgContent + "\n\n" +
                            tr("Do you want to open the webpage for "
                               "download now?"),
                        widgetP) == QMessageBox::Yes) {
                    trayMessageClicked();
                }
            }
#endif
        }
    }
}
