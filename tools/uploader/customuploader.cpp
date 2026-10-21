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
#include <tools/uploader/customuploader.h>

#include <QtNetwork>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

CustomUploader::CustomUploader(QSettings *settings, QNetworkAccessManager *network, QObject *parent) : ImageUploader(settings, network, parent)
{
    mUploaderType = "custom";
    migrateSettings(settings);
    loadSettings();
}

const char *CustomUploader::defaultField()
{
    return "files[]";
}

const char *CustomUploader::defaultLinkPath()
{
    return "files.0.url";
}

bool CustomUploader::migrateSettings(QSettings *settings)
{
    QString pomfUrl = settings->value("upload/pomf/pomf_url").toString().trimmed();

    if (pomfUrl.isEmpty() || settings->contains("upload/custom/url")) {
        return false;
    }

    // The pomf uploader appended "/upload.php" to the URL it was given.
    while (pomfUrl.endsWith('/')) {
        pomfUrl.chop(1);
    }

    settings->setValue("upload/custom/url", pomfUrl + "/upload.php");
    settings->setValue("upload/custom/field", defaultField());
    settings->setValue("upload/custom/linkPath", defaultLinkPath());
    return true;
}

QString CustomUploader::resolvePath(const QJsonValue &value, const QString &path, QString *errorString)
{
    auto fail = [errorString](const QString &message) {
        if (errorString) {
            *errorString = message;
        }

        return QString();
    };

    if (path.trimmed().isEmpty()) {
        return fail(tr("no path given"));
    }

    const QStringList segments = path.trimmed().split('.');
    QStringList walked;
    QJsonValue current = value;

    for (const QString &segment : segments) {
        walked << segment;
        const QString here = walked.join('.');

        if (current.isArray()) {
            bool isIndex = false;
            const int index = segment.toInt(&isIndex);
            const QJsonArray array = current.toArray();

            if (!isIndex || index < 0 || index >= array.size()) {
                return fail(tr("\"%1\" not found").arg(here));
            }

            current = array.at(index);
        } else if (current.isObject()) {
            const QJsonObject object = current.toObject();

            if (!object.contains(segment)) {
                return fail(tr("\"%1\" not found").arg(here));
            }

            current = object.value(segment);
        } else {
            return fail(tr("\"%1\" not found").arg(here));
        }
    }

    if (!current.isString()) {
        return fail(tr("\"%1\" is not a string").arg(path.trimmed()));
    }

    if (current.toString().isEmpty()) {
        return fail(tr("\"%1\" is empty").arg(path.trimmed()));
    }

    return current.toString();
}

void CustomUploader::setUploaderSettings(const QVariantHash &settings)
{
    mSettings = settings;
}

void CustomUploader::upload(const QString &fileName)
{
    const QString urlText = mSettings["url"].toString().trimmed();

    if (urlText.isEmpty()) {
        emit error(ImageUploader::HostError, tr("No upload URL set"), fileName);
        return;
    }

    const QUrl url = QUrl::fromUserInput(urlText);

    if (!url.isValid() || !url.scheme().startsWith("http")) {
        emit error(ImageUploader::HostError, tr("Invalid upload URL: %1").arg(urlText), fileName);
        return;
    }

    QString field = mSettings["field"].toString().trimmed();

    if (field.isEmpty()) {
        field = defaultField();
    }

    QFile *file = new QFile(fileName);

    if (!file->open(QIODevice::ReadOnly)) {
        emit error(ImageUploader::FileError, tr("Unable to read screenshot file"), fileName);
        file->deleteLater();
        return;
    }

    QNetworkRequest request(url);

    // One "Name: value" per line; lines without a name are skipped.
    const QStringList headerLines = mSettings["headers"].toString().split('\n');

    for (const QString &line : headerLines) {
        const int colon = line.indexOf(':');

        if (colon <= 0 || line.left(colon).trimmed().isEmpty()) {
            continue;
        }

        request.setRawHeader(line.left(colon).trimmed().toUtf8(), line.mid(colon + 1).trimmed().toUtf8());
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QMimeDatabase().mimeTypeForFile(fileName, QMimeDatabase::MatchExtension).name());
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"%1\"; filename=\"%2\"").arg(field, QFileInfo(fileName).fileName()));
    imagePart.setBodyDevice(file);

    file->setParent(multiPart);
    multiPart->append(imagePart);

    QNetworkReply *reply = mNetwork->post(request, multiPart);
    this->setProperty("fileName", fileName);
    multiPart->setParent(reply);

    connect(this , &CustomUploader::cancelRequest, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::uploadProgress, this, [this](qint64 bytesSent, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            setProgress(qRound(100.0 * bytesSent / bytesTotal));
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, fileName] {
        reply->deleteLater();

        // Cancelled by the user (Uploader::cancel aborts the reply): not an error (bug 22).
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            emit error(ImageUploader::CancelError, QString(), fileName);
            return;
        }

        const QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

        if (status.isValid() && (status.toInt() < 200 || status.toInt() > 299)) {
            emit error(ImageUploader::HostError, tr("Upload failed: HTTP %1").arg(status.toInt()), fileName);
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            emit error(ImageUploader::NetworkError, tr("Error reaching uploader: %1").arg(reply->errorString()), fileName);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument response = QJsonDocument::fromJson(reply->readAll(), &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit error(ImageUploader::HostError, tr("Invalid response from uploader: %1").arg(parseError.errorString()), fileName);
            return;
        }

        const QJsonValue root = response.isArray() ? QJsonValue(response.array()) : QJsonValue(response.object());

        QString linkPath = mSettings["linkPath"].toString().trimmed();

        if (linkPath.isEmpty()) {
            linkPath = defaultLinkPath();
        }

        QString pathError;
        const QString link = resolvePath(root, linkPath, &pathError);

        if (link.isEmpty()) {
            emit error(ImageUploader::HostError, tr("No link in the response: %1").arg(pathError), fileName);
            return;
        }

        // The delete link is optional: a path that does not resolve gives none, not an error.
        const QString deletePath = mSettings["deletePath"].toString().trimmed();
        const QString deleteLink = deletePath.isEmpty() ? QString() : resolvePath(root, deletePath);

        emit uploaded(fileName, link, deleteLink);
    });
}

void CustomUploader::retry()
{
    upload(property("fileName").toString());
}

void CustomUploader::cancel()
{
    emit cancelRequest();
}
