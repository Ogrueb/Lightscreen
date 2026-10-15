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
#ifndef IMGURUPLOADER_H
#define IMGURUPLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <tools/uploader/imageuploader.h>
#include <functional>

class ImgurUploader : public ImageUploader
{
    Q_OBJECT

public:
    typedef std::function<void(bool)> AuthorizationCallback;

    ImgurUploader(QObject *parent = 0);
    static const QString clientId();
    static const QString clientSecret();
    static void authorize(const QString &pin, AuthorizationCallback callback);
    static void refreshAuthorization(const QString &refresh_token, AuthorizationCallback callback);

public slots:
    void upload(const QString &fileName);
    void retry();
    void cancel();

private slots:
    void finished();
    void uploadProgress(qint64 bytesReceived, qint64 bytesTotal);

signals:
    void cancelRequest();

private:
    static void authorizationReply(QNetworkReply *reply, AuthorizationCallback callback);

};

#endif // IMGURUPLOADER_H
