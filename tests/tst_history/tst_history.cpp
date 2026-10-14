// Safety-net tests for the screenshot history (tools/screenshotmanager): they record current behaviour, not intended
// behaviour. Assertions marked "expected to change" document a known bug and flip when it is fixed.
#include <QtTest>
#include <QSettings>
#include <QTemporaryDir>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

#include <tools/screenshotmanager.h>
#include <tools/uploader/uploader.h>

struct HistoryRow {
    QString fileName;
    QString url;
    QString deleteUrl;
    qint64 time;
};

class tst_History : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
    }

    void init()
    {
        mDir = new QTemporaryDir;
        QVERIFY(mDir->isValid());
        mHistoryPath = mDir->path() + QDir::separator() + "history" + QDir::separator();

        Uploader::setInstance(&mUploader);
        mSettings = new QSettings(mDir->filePath("settings.ini"), QSettings::IniFormat);
        mManager = new ScreenshotManager(mSettings, mHistoryPath);
        ScreenshotManager::setInstance(mManager);
    }

    void cleanup()
    {
        ScreenshotManager::setInstance(nullptr);
        delete mManager;
        mManager = nullptr;
        delete mSettings;
        mSettings = nullptr;
        Uploader::setInstance(nullptr);

        // The manager opens the default connection; close it so the temporary directory can be removed on Windows.
        closeDatabase();
        delete mDir;
        mDir = nullptr;
    }

    // initHistory creates the history directory, the database file and the table, in the directory given to the seam.
    void initHistoryCreatesDatabase()
    {
        QVERIFY(!QDir(mHistoryPath).exists());

        mManager->initHistory();

        QVERIFY(QDir(mHistoryPath).exists());
        QVERIFY(QFile::exists(mHistoryPath + "history.sqlite"));
        QCOMPARE(QSqlDatabase::database().databaseName(), mHistoryPath + "history.sqlite");

        QSqlRecord columns = QSqlDatabase::database().record("history");
        QCOMPARE(columns.count(), 4);
        QCOMPARE(columns.fieldName(0), QStringLiteral("fileName"));
        QCOMPARE(columns.fieldName(1), QStringLiteral("URL"));
        QCOMPARE(columns.fieldName(2), QStringLiteral("deleteURL"));
        QCOMPARE(columns.fieldName(3), QStringLiteral("time"));
        QCOMPARE(rows().count(), 0);
    }

    // History is on when the key is absent: a row is written with the time of the call.
    void saveHistoryDefaultOn()
    {
        const qint64 before = QDateTime::currentMSecsSinceEpoch();
        mManager->saveHistory("/shots/a.png");
        const qint64 after = QDateTime::currentMSecsSinceEpoch();

        const QList<HistoryRow> all = rows();
        QCOMPARE(all.count(), 1);
        QCOMPARE(all[0].fileName, QStringLiteral("/shots/a.png"));
        QCOMPARE(all[0].url, QString(""));
        QCOMPARE(all[0].deleteUrl, QString(""));
        QVERIFY(all[0].time >= before && all[0].time <= after);
    }

    // The third argument is a delete hash, stored with the Imgur delete URL prefix (M12 changes this).
    void saveHistoryWithUrlAndDeleteHash()
    {
        mManager->saveHistory("/shots/a.png", "https://example.com/a.png", "hash123");

        const QList<HistoryRow> all = rows();
        QCOMPARE(all.count(), 1);
        QCOMPARE(all[0].url, QStringLiteral("https://example.com/a.png"));
        QCOMPARE(all[0].deleteUrl, QStringLiteral("https://imgur.com/delete/hash123"));
    }

    // Saving the same file twice gives two rows; nothing de-duplicates.
    void saveHistoryTwiceGivesTwoRows()
    {
        mManager->saveHistory("/shots/a.png");
        mManager->saveHistory("/shots/a.png");

        QCOMPARE(rows().count(), 2);
    }

    // With history off nothing is written, and the database is not even created.
    void saveHistoryOff()
    {
        mSettings->setValue("options/history", false);

        mManager->saveHistory("/shots/a.png", "https://example.com/a.png", "hash123");

        QVERIFY(!QFile::exists(mHistoryPath + "history.sqlite"));
        mManager->initHistory();
        QCOMPARE(rows().count(), 0);
    }

    // Bug 13 (fixed): updating a file that already has a row fills in that row's URL and delete URL.
    void updateHistoryExistingFile()
    {
        mManager->saveHistory("/shots/a.png");
        mManager->updateHistory("/shots/a.png", "https://example.com/a.png", "hash123");

        const QList<HistoryRow> all = rows();
        QCOMPARE(all.count(), 1);
        QCOMPARE(all[0].fileName, QStringLiteral("/shots/a.png"));
        QCOMPARE(all[0].url, QStringLiteral("https://example.com/a.png"));
        QCOMPARE(all[0].deleteUrl, QStringLiteral("https://imgur.com/delete/hash123"));
    }

    // A second update for the same file overwrites the first URL. With no delete hash the delete URL stays empty,
    // as saveHistory writes it.
    void updateHistoryTwice()
    {
        mManager->updateHistory("/shots/a.png", "https://example.com/1.png", "hash1");
        mManager->updateHistory("/shots/a.png", "https://example.com/2.png", "");

        const QList<HistoryRow> all = rows();
        QCOMPARE(all.count(), 1);
        QCOMPARE(all[0].url, QStringLiteral("https://example.com/2.png"));
        QCOMPARE(all[0].deleteUrl, QString(""));
    }

    // A file with no row gets one, with the URL and delete URL.
    void updateHistoryNewFile()
    {
        mManager->saveHistory("/shots/other.png");
        mManager->updateHistory("/shots/a.png", "https://example.com/a.png", "hash123");

        const QList<HistoryRow> all = rows();
        QCOMPARE(all.count(), 2);
        QCOMPARE(all[1].fileName, QStringLiteral("/shots/a.png"));
        QCOMPARE(all[1].url, QStringLiteral("https://example.com/a.png"));
        QCOMPARE(all[1].deleteUrl, QStringLiteral("https://imgur.com/delete/hash123"));
    }

    // An empty URL is ignored.
    void updateHistoryEmptyUrl()
    {
        mManager->saveHistory("/shots/a.png");
        mManager->updateHistory("/shots/a.png", "", "hash123");
        mManager->updateHistory("/shots/b.png", "", "hash123");

        const QList<HistoryRow> all = rows();
        QCOMPARE(all.count(), 1);
        QCOMPARE(all[0].url, QString(""));
    }

    // With history off, updates write nothing.
    void updateHistoryOff()
    {
        mSettings->setValue("options/history", false);

        mManager->updateHistory("/shots/a.png", "https://example.com/a.png", "hash123");

        mManager->initHistory();
        QCOMPARE(rows().count(), 0);
    }

    // An upload the manager is not tracking (history dialog, upload later) goes through updateHistory.
    void uploaderDoneUpdatesHistory()
    {
        mManager->saveHistory("/shots/a.png");
        emit mUploader.done("/shots/a.png", "https://example.com/a.png", "hash123");

        const QList<HistoryRow> all = rows();
        QCOMPARE(all.count(), 1);
        QCOMPARE(all[0].url, QStringLiteral("https://example.com/a.png"));
    }

    // removeHistory deletes only the row matching both the file name and the time.
    void removeHistory()
    {
        mManager->saveHistory("/shots/a.png");
        QTest::qWait(5);
        mManager->saveHistory("/shots/a.png");
        mManager->saveHistory("/shots/b.png");

        QList<HistoryRow> all = rows();
        QCOMPARE(all.count(), 3);
        QVERIFY(all[0].time != all[1].time);

        mManager->removeHistory("/shots/a.png", all[0].time);
        all = rows();
        QCOMPARE(all.count(), 2);
        QCOMPARE(all[0].fileName, QStringLiteral("/shots/a.png"));
        QCOMPARE(all[1].fileName, QStringLiteral("/shots/b.png"));

        // Wrong time, or the right time with the wrong file: nothing is removed.
        mManager->removeHistory("/shots/b.png", all[1].time + 1);
        mManager->removeHistory("/shots/c.png", all[1].time);
        QCOMPARE(rows().count(), 2);
    }

    // removeHistory and clearHistory do not check the history setting.
    void removeAndClearIgnoreHistorySetting()
    {
        mManager->saveHistory("/shots/a.png");
        mManager->saveHistory("/shots/b.png");
        const QList<HistoryRow> all = rows();
        mSettings->setValue("options/history", false);

        mManager->removeHistory("/shots/a.png", all[0].time);
        QCOMPARE(rows().count(), 1);

        mManager->clearHistory();
        QCOMPARE(rows().count(), 0);
    }

    // clearHistory empties the table and keeps the database usable.
    void clearHistory()
    {
        mManager->saveHistory("/shots/a.png");
        mManager->saveHistory("/shots/b.png", "https://example.com/b.png");
        QCOMPARE(rows().count(), 2);

        mManager->clearHistory();
        QCOMPARE(rows().count(), 0);
        QVERIFY(QFile::exists(mHistoryPath + "history.sqlite"));

        mManager->saveHistory("/shots/c.png");
        QCOMPARE(rows().count(), 1);
    }

    // The seam constructor is never in portable mode.
    void seamIsNotPortable()
    {
        QVERIFY(!mManager->portableMode());
        QCOMPARE(mManager->settings(), mSettings);
    }

    // The default constructor, without a config.ini next to the executable: user settings, and the history in the
    // application data location.
    void defaultConstructorNotPortable()
    {
        const QString configPath = qApp->applicationDirPath() + QDir::separator() + "config.ini";
        if (QFile::exists(configPath)) {
            QSKIP("A config.ini already exists next to the test executable");
        }
        resetManager();

        ScreenshotManager manager;
        QVERIFY(!manager.portableMode());
        QVERIFY(manager.settings()->fileName() != configPath);

        manager.initHistory();
        const QString expected = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator()
                + "history.sqlite";
        QCOMPARE(QSqlDatabase::database().databaseName(), expected);

        closeDatabase();
        QFile::remove(expected);
    }

    // The default constructor with a config.ini next to the executable: portable mode, settings from that file and
    // the history next to the executable. The executable's own directory is the only one reachable without a code
    // change, so the test writes there and removes what it wrote.
    void defaultConstructorPortable()
    {
        const QString appDir = qApp->applicationDirPath() + QDir::separator();
        const QString configPath = appDir + "config.ini";
        const QString historyFile = appDir + "history.sqlite";
        if (QFile::exists(configPath) || QFile::exists(historyFile)) {
            QSKIP("A config.ini or history.sqlite already exists next to the test executable");
        }
        resetManager();

        {
            QFile config(configPath);
            QVERIFY(config.open(QIODevice::WriteOnly));
            config.write("[options]\nhistory=true\n");
        }

        {
            ScreenshotManager manager;
            QVERIFY(manager.portableMode());
            QCOMPARE(QDir::toNativeSeparators(manager.settings()->fileName()), QDir::toNativeSeparators(configPath));
            QCOMPARE(manager.settings()->format(), QSettings::IniFormat);

            manager.saveHistory("/shots/a.png");
            QCOMPARE(QSqlDatabase::database().databaseName(), historyFile);
            QCOMPARE(rows().count(), 1);
        }

        closeDatabase();
        QVERIFY(QFile::remove(configPath));
        QVERIFY(QFile::remove(historyFile));
    }

private:
    // Rows in insertion order.
    QList<HistoryRow> rows() const
    {
        QList<HistoryRow> result;
        QSqlQuery query("SELECT fileName, URL, deleteURL, time FROM history ORDER BY rowid");
        while (query.next()) {
            result.append({query.value(0).toString(), query.value(1).toString(), query.value(2).toString(),
                           query.value(3).toLongLong()});
        }
        return result;
    }

    void closeDatabase()
    {
        if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
            QSqlDatabase::database(QSqlDatabase::defaultConnection, false).close();
            QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
        }
    }

    // Drops the manager built by init() so a test can build its own with the default constructor.
    void resetManager()
    {
        ScreenshotManager::setInstance(nullptr);
        delete mManager;
        mManager = nullptr;
        closeDatabase();
    }

    QTemporaryDir *mDir = nullptr;
    QString mHistoryPath;
    QSettings *mSettings = nullptr;
    ScreenshotManager *mManager = nullptr;
    Uploader mUploader;
};

QTEST_MAIN(tst_History)
#include "tst_history.moc"
