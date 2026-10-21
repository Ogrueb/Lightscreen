/*
 * Copyright (C) Christian Kaiser
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QVersionNumber>

#include <updater/updater.h>
#include <dialogs/updaterdialog.h>
#include <tools/uploader/uploader.h>

#ifndef UPDATE_REPO
#error "UPDATE_REPO is set in CMakeLists.txt"
#endif

Updater::Updater(QObject *parent) :
    QObject(parent)
{
}

Updater::~Updater()
{
    if (mReply) {
        mReply->disconnect(this);
        mReply->abort();
        mReply->deleteLater();
    }
}

QString Updater::repository()
{
    return QStringLiteral(UPDATE_REPO);
}

QUrl Updater::latestReleaseApiUrl()
{
    return QUrl(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(repository()));
}

QUrl Updater::releasesPageUrl()
{
    return QUrl(QStringLiteral("https://github.com/%1/releases").arg(repository()));
}

void Updater::check()
{
    if (mReply) {
        return; // A check is already running.
    }

    QNetworkRequest request(latestReleaseApiUrl());
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", QStringLiteral("Lightscreen/%1").arg(qApp->applicationVersion()).toUtf8());
    request.setTransferTimeout(30000);

    mReply = Uploader::network()->get(request);
    connect(mReply.data(), &QNetworkReply::finished, this, &Updater::finished);
}

void Updater::checkWithFeedback()
{
    UpdaterDialog updaterDialog;
    connect(this, &Updater::done, &updaterDialog, &UpdaterDialog::updateDone);
    connect(this, &Updater::failed, &updaterDialog, &UpdaterDialog::updateFailed);

    check();
    updaterDialog.exec();
}

void Updater::finished()
{
    QNetworkReply *reply = mReply.data();
    mReply = nullptr;

    if (!reply) {
        return;
    }

    reply->deleteLater();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status == 404) {
        emit failed(tr("No release has been published yet at %1.").arg(releasesPageUrl().toString()));
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit failed(reply->errorString());
        return;
    }

    if (status != 200) {
        emit failed(tr("GitHub answered with HTTP status %1.").arg(status));
        return;
    }

    const QJsonObject release = QJsonDocument::fromJson(reply->readAll()).object();
    QString tag = release.value(QStringLiteral("tag_name")).toString();

    if (tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        tag.remove(0, 1);
    }

    const auto remoteVersion = QVersionNumber::fromString(tag).normalized();

    if (remoteVersion.isNull()) {
        emit failed(tr("Could not read a version from GitHub's answer."));
        return;
    }

    const auto currentVersion = QVersionNumber::fromString(qApp->applicationVersion()).normalized();

    QString htmlUrl = release.value(QStringLiteral("html_url")).toString();

    if (htmlUrl.isEmpty()) {
        htmlUrl = releasesPageUrl().toString();
    }

    emit done(remoteVersion > currentVersion, htmlUrl);
}
