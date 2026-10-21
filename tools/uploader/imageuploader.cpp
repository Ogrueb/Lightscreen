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
#include <tools/uploader/imageuploader.h>
#include <tools/uploader/pomfuploader.h>

#include <utility>

#include <QSettings>

ImageUploader *ImageUploader::factory(const QString &name, QSettings *settings, QNetworkAccessManager *network)
{
    // pomf is the only service until S03; unknown names fall back to it instead of returning nullptr.
    Q_UNUSED(name);
    return new PomfUploader(settings, network);
}

QVariantHash ImageUploader::loadSettings(QSettings *globalSettings, const QString &uploaderType)
{
    globalSettings->beginGroup("upload/" + uploaderType);
    auto keys = globalSettings->childKeys();

    QVariantHash settings;

    for (auto key : std::as_const(keys)) {
        settings[key] = globalSettings->value(key);
    }

    globalSettings->endGroup();
    return settings;
}

void ImageUploader::loadSettings()
{
    mSettings = loadSettings(mGlobalSettings, mUploaderType);
}

void ImageUploader::saveSettings(QSettings *globalSettings, const QString &uploaderType, const QVariantHash &settings) {
    globalSettings->beginGroup("upload/" + uploaderType);

    for (auto key : settings.keys()) {
        globalSettings->setValue(key, settings[key]);
    }

    globalSettings->endGroup();
}

void ImageUploader::saveSettings()
{
    saveSettings(mGlobalSettings, mUploaderType, mSettings);
}

int ImageUploader::progress() const {
    return mProgress;
}

void ImageUploader::setProgress(int progress)
{
    if (mProgress != progress) {
        mProgress = progress;
        emit progressChanged(mProgress);
    }
}
