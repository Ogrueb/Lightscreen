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
#ifndef IMAGEUPLOADER_H
#define IMAGEUPLOADER_H

#include <QHash>
#include <QVariant>

class QNetworkReply;
class ImageUploader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)

public:
    static ImageUploader *factory(const QString &type);

    enum Error {
        FileError,
        NetworkError,
        HostError,
        AuthorizationError,
        CancelError,
        OtherError
    };
    Q_ENUM(Error)

public:
    inline ImageUploader(QObject *parent = 0) : QObject(parent), mProgress(0) {}

    static QVariantHash loadSettings(const QString &uploaderType);
    void loadSettings();

    static void saveSettings(const QString &uploaderType, const QVariantHash &settings);
    void saveSettings();

public slots:
    virtual void upload(const QString &fileName) = 0;
    virtual void cancel() = 0;
    virtual void retry() = 0;
    int progress() const;
    void setProgress(int progress);

signals:
    void uploaded(const QString &fileName, const QString &url, const QString &deleteHash); // TODO: Make last param generic
    void error(ImageUploader::Error errorCode, const QString &errorString, const QString &fileName);
    void progressChanged(int progress);

protected:
    int mProgress;
    QString mUploaderType;
    QVariantHash mSettings;

};

#endif // IMAGEUPLOADER_H
