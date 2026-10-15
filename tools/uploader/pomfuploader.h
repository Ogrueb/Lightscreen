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
#ifndef POMFUPLOADER_H
#define POMFUPLOADER_H

#include <tools/uploader/imageuploader.h>
#include <functional>

class PomfUploader : public ImageUploader
{
    Q_OBJECT

public:
    PomfUploader(QObject *parent = 0);

    typedef std::function<void(bool)> VerificationCallback;
    static QNetworkReply* verify(const QString &url, VerificationCallback callback);

public slots:
    void upload(const QString &fileName);
    void retry();
    void cancel();

signals:
    void cancelRequest();

};

#endif // POMFUPLOADER_H
