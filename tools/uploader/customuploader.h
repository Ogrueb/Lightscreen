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
#ifndef CUSTOMUPLOADER_H
#define CUSTOMUPLOADER_H

#include <tools/uploader/imageuploader.h>

class QJsonValue;

// Uploads to a host the user configures under "upload/custom": a multipart POST of the file to "url" under the
// form field "field", with the extra "headers" (one "Name: value" per line), and the link read from the JSON
// response at "linkPath" (and the delete link at "deletePath", if set).
class CustomUploader : public ImageUploader
{
    Q_OBJECT

public:
    CustomUploader(QSettings *settings, QNetworkAccessManager *network, QObject *parent = 0);

    static const char *defaultField();
    static const char *defaultLinkPath();

    // One-time migration from the pomf uploader: if "upload/pomf/pomf_url" is set and "upload/custom/url" is not,
    // fills in the custom settings that reach the same host. The old keys are left in place. Returns whether it wrote.
    static bool migrateSettings(QSettings *settings);

    // Follows a dotted path ("files.0.url") through a JSON value: names select object members, integers select
    // array elements. Returns the string found there; anything else (missing key, out of range, not a string)
    // returns a null string and, if errorString is given, says why.
    static QString resolvePath(const QJsonValue &value, const QString &path, QString *errorString = nullptr);

    // Replaces the settings loaded from QSettings, for the options dialog's "Test upload" (unsaved values).
    void setUploaderSettings(const QVariantHash &settings);

public slots:
    void upload(const QString &fileName);
    void retry();
    void cancel();

signals:
    void cancelRequest();

};

#endif // CUSTOMUPLOADER_H
