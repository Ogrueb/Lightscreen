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
#include <tools/screenshotmanager.h>
#include <tools/settings.h>
#include <tools/optimizer.h>
#include <tools/screenshot.h>
#include <tools/uploader/uploader.h>
#include <tools/os.h>

#include <memory>
#include <utility>

#include <QApplication>
#include <QDateTime>
#include <QStandardPaths>
#include <QFile>
#include <QSettings>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

ScreenshotManager::ScreenshotManager(QObject *parent) : QObject(parent), mHistoryInitialized(false)
{
    if (QFile::exists(qApp->applicationDirPath() + QDir::separator() + "config.ini")) {
        mSettings     = new QSettings(qApp->applicationDirPath() + QDir::separator() + "config.ini", QSettings::IniFormat, this);
        mPortableMode = true;
        mHistoryPath  = qApp->applicationDirPath() + QDir::separator();
    } else {
        mSettings     = new QSettings(this);
        mPortableMode = false;
        mHistoryPath  = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QDir::separator();
    }

    init();
}

// Test seam: uses the given settings (not owned) and history directory (with a trailing separator) instead of
// the user's configuration.
ScreenshotManager::ScreenshotManager(QSettings *settings, const QString &historyPath, QObject *parent) : QObject(parent),
    mSettings(settings), mHistoryPath(historyPath), mPortableMode(false), mHistoryInitialized(false)
{
    init();
}

// The manager owns the uploader, which reads its service settings from the manager's settings, and the optimization
// queue, which keeps its pending list next to the history.
void ScreenshotManager::init()
{
    mUploader = new Uploader(mSettings, this);
    connect(mUploader, &Uploader::done, this, &ScreenshotManager::uploadDone);

    mOptimizer = new Optimizer(mSettings, mHistoryPath, this);
}

void ScreenshotManager::initHistory()
{
    if (mHistoryInitialized) {
        return;
    }

    // Creating the SQLite database.
    QSqlDatabase history = QSqlDatabase::addDatabase("QSQLITE");

    QDir historyPath(mHistoryPath);

    if (!historyPath.exists()) {
        historyPath.mkpath(mHistoryPath);
    }

    history.setHostName("localhost");
    history.setDatabaseName(mHistoryPath + "history.sqlite");

    if (history.open()) {
        QSqlQuery tableQuery;
        mHistoryInitialized = tableQuery.exec("CREATE TABLE IF NOT EXISTS history (fileName text, URL text, deleteURL text, time integer)");

        QSqlQuery(history).exec("CREATE INDEX IF NOT EXISTS fileName_index ON history(fileName)");
    } else {
        qCritical() << "Could not open SQLite DB.";
        mHistoryInitialized = false;
    }
}

int ScreenshotManager::activeCount() const
{
    return mScreenshots.count();
}

bool ScreenshotManager::portableMode()
{
    return mPortableMode;
}

void ScreenshotManager::saveHistory(const QString &fileName, const QString &url, const QString &deleteUrl)
{
    if (!Settings::value(mSettings, "options/history", mPortableMode).toBool()) {
        return;
    }

    if (!mHistoryInitialized) {
        initHistory();
    }

    QSqlQuery saveHistoryQuery;
    saveHistoryQuery.prepare("INSERT INTO history (fileName, URL, deleteURL, time) VALUES(?, ?, ?, ?)");
    saveHistoryQuery.addBindValue(fileName);
    saveHistoryQuery.addBindValue(url);
    saveHistoryQuery.addBindValue(deleteUrl);
    saveHistoryQuery.addBindValue(QDateTime::currentMSecsSinceEpoch());
    saveHistoryQuery.exec();
}

void ScreenshotManager::updateHistory(const QString &fileName, const QString &url, const QString &deleteUrl)
{
    if (!Settings::value(mSettings, "options/history", mPortableMode).toBool() || url.isEmpty()) {
        return;
    }

    if (!mHistoryInitialized) {
        initHistory();
    }

    QSqlQuery query;
    query.prepare("SELECT fileName FROM history WHERE fileName = ?");
    query.addBindValue(fileName);

    if (query.exec() && query.next()) {
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE history SET URL = ?, deleteURL = ?, time = ? WHERE fileName = ?");
        updateQuery.addBindValue(url);
        updateQuery.addBindValue(deleteUrl);
        updateQuery.addBindValue(QDateTime::currentMSecsSinceEpoch());
        updateQuery.addBindValue(fileName);

        updateQuery.exec();
    } else {
        saveHistory(fileName, url, deleteUrl);
    }
}

void ScreenshotManager::removeHistory(const QString &fileName, qint64 time)
{
    if (!mHistoryInitialized) {
        initHistory();
    }

    QSqlQuery removeQuery;
    removeQuery.prepare("DELETE FROM history WHERE fileName = ? AND time = ?");
    removeQuery.addBindValue(fileName);
    removeQuery.addBindValue(time);

    removeQuery.exec();
}

void ScreenshotManager::clearHistory()
{
    if (!mHistoryInitialized) {
        initHistory();
    }

    QSqlQuery deleteQuery("DELETE FROM history");
    deleteQuery.exec();

    QSqlQuery vacQuery("VACUUM");
    vacQuery.exec();
}

//

void ScreenshotManager::areaSelectionNeeded()
{
    Screenshot *s = qobject_cast<Screenshot *>(sender());
    emit areaSelection(s);
}

void ScreenshotManager::askConfirmation()
{
    Screenshot *s = qobject_cast<Screenshot *>(sender());
    emit confirm(s);
}

void ScreenshotManager::cleanup()
{
    Screenshot *screenshot = qobject_cast<Screenshot *>(sender());
    emit windowCleanup(screenshot->options());
}

void ScreenshotManager::finished()
{
    Screenshot *screenshot = qobject_cast<Screenshot *>(sender());
    mScreenshots.removeOne(screenshot);
    emit activeCountChange();
    screenshot->deleteLater();
}

// A saved file goes into the history and an upload request to the uploader. take() does this for every
// screenshot; tests that build a Screenshot themselves call it directly.
void ScreenshotManager::connectScreenshot(Screenshot *screenshot)
{
    connect(screenshot, &Screenshot::saved, this, [this](const QString &fileName) {
        saveHistory(fileName);
    });
    connect(screenshot, &Screenshot::uploadRequested, mUploader, &Uploader::upload);

    // The screenshot goes on once the optimizer is done with its file: once, even if the same file name is queued
    // again by another screenshot (replace mode).
    connect(screenshot, &Screenshot::optimizeRequested, this, [this, screenshot](const QString &fileName) {
        auto connection = std::make_shared<QMetaObject::Connection>();
        *connection = connect(mOptimizer, &Optimizer::optimizationDone, screenshot, [screenshot, fileName, connection](const QString &done) {
            if (done == fileName) {
                QObject::disconnect(*connection);
                screenshot->optimizationDone();
            }
        });

        mOptimizer->add(fileName);
    });
}

void ScreenshotManager::take(Screenshot::Options &options)
{
    Screenshot *newScreenshot = new Screenshot(this, options);
    mScreenshots.append(newScreenshot);

    connectScreenshot(newScreenshot);
    connect(newScreenshot, &Screenshot::areaSelectionNeeded, this, &ScreenshotManager::areaSelectionNeeded);
    connect(newScreenshot, &Screenshot::askConfirmation    , this, &ScreenshotManager::askConfirmation);
    connect(newScreenshot, &Screenshot::cleanup            , this, &ScreenshotManager::cleanup);
    connect(newScreenshot, &Screenshot::finished           , this, &ScreenshotManager::finished);

    newScreenshot->take();
}

void ScreenshotManager::uploadDone(const QString &fileName, const QString &url, const QString &deleteHash)
{
    for (Screenshot *screenshot : std::as_const(mScreenshots)) {
        if (screenshot->options().fileName == fileName
                || screenshot->unloadedFileName() == fileName) {
            screenshot->uploadDone(url);

            if (screenshot->options().file) {
                updateHistory(fileName, url, deleteHash);
            } else {
                saveHistory("", url, deleteHash);
            }

            return;
        }
    }

    // If we get here, it's because the screenshot upload wasn't on the current screenshot list, which means it's a View History/Upload Later upload.
    updateHistory(fileName, url, deleteHash);
}

// Singleton
ScreenshotManager *ScreenshotManager::mInstance = nullptr;

ScreenshotManager *ScreenshotManager::instance()
{
    if (!mInstance) {
        mInstance = new ScreenshotManager();
    }

    return mInstance;
}

// Test seam: replaces the singleton. The previous instance is not deleted.
void ScreenshotManager::setInstance(ScreenshotManager *manager)
{
    mInstance = manager;
}
