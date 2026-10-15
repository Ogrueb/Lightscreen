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
#include <tools/uploader/pomfuploader.h>
#include <tools/uploader/uploader.h>

#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonObject>

PomfUploader::PomfUploader(QObject *parent) : ImageUploader(parent)
{
    mUploaderType = "pomf";
    loadSettings();
}

QNetworkReply* PomfUploader::verify(const QString &url, VerificationCallback callback)
{
    QNetworkRequest request(QUrl::fromUserInput(QString("%1/upload.php").arg(url)));

    if (!request.url().isValid()) {
        callback(false);
        return 0;
    }

    QNetworkReply *reply = Uploader::network()->get(request);

    connect(reply, &QNetworkReply::finished, [reply, callback] {
        reply->deleteLater();

        const QJsonObject pomfResponse = QJsonDocument::fromJson(reply->readAll()).object();

        if (!pomfResponse.isEmpty() && pomfResponse.contains("success")) {
            callback(true);
        } else {
            callback(false);
        }
    });

    connect(reply, &QNetworkReply::sslErrors, [callback](const QList<QSslError> &errors) {
        Q_UNUSED(errors);
        callback(false);
    });

    return reply;
}

void PomfUploader::upload(const QString &fileName)
{
    QString pomfUrl = mSettings["pomf_url"].toString();

    if (pomfUrl.isEmpty()) {
        emit error(ImageUploader::HostError, tr("Invalid pomf uploader URL!"), fileName);
        return;
    }

    QUrl url = QUrl::fromUserInput(pomfUrl + "/upload.php");

    QFile *file = new QFile(fileName);

    if (!file->open(QIODevice::ReadOnly)) {
        emit error(ImageUploader::FileError, tr("Unable to read screenshot file"), fileName);
        file->deleteLater();
        return;
    }

    QNetworkRequest request(url);

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QMimeDatabase().mimeTypeForFile(fileName, QMimeDatabase::MatchExtension).name());
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"files[]\"; filename=\"%1\"").arg(QFileInfo(fileName).fileName()));
    imagePart.setBodyDevice(file);

    file->setParent(multiPart);
    multiPart->append(imagePart);

    QNetworkReply *reply = Uploader::network()->post(request, multiPart);
    this->setProperty("fileName", fileName);
    multiPart->setParent(reply);

    connect(this , &PomfUploader::cancelRequest, reply, &QNetworkReply::abort);
    connect(this , &PomfUploader::cancelRequest, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::uploadProgress, this, [&](qint64 bytesSent, qint64 bytesTotal) {
        float b = (float) bytesSent / bytesTotal;
        int p = qRound(b * 100);
        setProgress(p);
    });

    connect(reply, &QNetworkReply::finished, this, [&, reply, fileName] {
        const QJsonObject pomfResponse = QJsonDocument::fromJson(reply->readAll()).object();

        if (reply->error() != QNetworkReply::NoError && pomfResponse.isEmpty()) {
            emit error(ImageUploader::NetworkError, tr("Error reaching uploader"), fileName);
            return;
        }

        if (!pomfResponse.contains("success") || !pomfResponse.contains("files")) {
            emit error(ImageUploader::HostError, tr("Invalid response from uploader"), fileName);
            return;
        }

        if (pomfResponse["success"].toBool()) {
            emit uploaded(fileName, pomfResponse["files"].toArray().at(0).toObject()["url"].toString(), "");
        } else {
            QString description;

            if (pomfResponse.contains("description")) {
                description = pomfResponse["description"].toString();
            }

            if (description.isEmpty()) {
                description = tr("Host error");
            }

            emit error(ImageUploader::HostError, description, fileName);
        }
    });
}

void PomfUploader::retry()
{
    upload(property("fileName").toString());
}

void PomfUploader::cancel()
{
    emit cancelRequest();
}

