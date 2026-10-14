// Safety net for tools/screenshot: file naming, extensions, the " (n)" collision names and save().
// These tests record current behaviour, quirks included, not intended behaviour (docs/modernization-plan.md, section 6).
#include <QtTest>
#include <QClipboard>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/uploader/uploader.h>

class tst_Screenshot : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir *mHistoryDir = nullptr;
    QSettings *mSettings = nullptr;
    Uploader *mUploader = nullptr;
    ScreenshotManager *mManager = nullptr;

    static void touch(const QDir &dir, const QString &name)
    {
        QFile file(dir.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("x");
    }

    static Screenshot::NamingOptions naming(Screenshot::Naming naming, bool flip = false, int leadingZeros = 0,
                                            const QString &dateFormat = QString())
    {
        Screenshot::NamingOptions options;
        options.naming = naming;
        options.flip = flip;
        options.leadingZeros = leadingZeros;
        options.dateFormat = dateFormat;
        return options;
    }

    // Saves to <dir>/shot<extension> (Empty naming), nothing else enabled.
    static Screenshot::Options options(const QString &dir)
    {
        Screenshot::Options options;
        options.fileName = QString();
        options.result = Screenshot::Failure;
        options.format = Screenshot::PNG;
        options.namingOptions = naming(Screenshot::Empty);
        options.directory = QDir(dir);
        options.prefix = QStringLiteral("shot");
        options.mode = Screenshot::None;
        options.quality = 90;
        options.animations = false;
        options.clipboard = false;
        options.urlClipboard = false;
        options.currentMonitor = false;
        options.cursor = false;
        options.file = false;
        options.magnify = false;
        options.optimize = false;
        options.preview = false;
        options.replace = false;
        options.saveAs = false;
        options.upload = false;
        return options;
    }

    // A pixmap with enough detail that JPEG quality changes the file size.
    static QPixmap testPixmap()
    {
        QImage image(64, 48, QImage::Format_RGB32);

        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                image.setPixel(x, y, qRgb((x * 37 + y * 11) % 256, (x * y) % 256, (x ^ y) * 4 % 256));
            }
        }

        return QPixmap::fromImage(image);
    }

    static QString native(const QString &path)
    {
        return QDir::toNativeSeparators(path);
    }

    static int historyRows(const QString &fileName)
    {
        QSqlQuery query;
        query.prepare("SELECT COUNT(*) FROM history WHERE fileName = ?");
        query.addBindValue(fileName);

        if (!query.exec() || !query.next()) {
            return -1;
        }

        return query.value(0).toInt();
    }

    static int historyRows()
    {
        QSqlQuery query("SELECT COUNT(*) FROM history");
        return query.next() ? query.value(0).toInt() : -1;
    }

    // Records the order of the signals a screenshot emits.
    static void record(Screenshot *screenshot, QStringList *log)
    {
        connect(screenshot, &Screenshot::finished, [log] { log->append("finished"); });
        connect(screenshot, &Screenshot::cleanup, [log] { log->append("cleanup"); });
        connect(screenshot, &Screenshot::askConfirmation, [log] { log->append("askConfirmation"); });
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);

        mHistoryDir = new QTemporaryDir;
        QVERIFY(mHistoryDir->isValid());

        mUploader = new Uploader;
        Uploader::setInstance(mUploader);

        mSettings = new QSettings(mHistoryDir->filePath("settings.ini"), QSettings::IniFormat);
        mManager = new ScreenshotManager(mSettings, mHistoryDir->path() + QDir::separator());
        ScreenshotManager::setInstance(mManager);

        // Opens the history database, so historyRows() works before the first save.
        mManager->initHistory();
    }

    void cleanupTestCase()
    {
        ScreenshotManager::setInstance(nullptr);
        Uploader::setInstance(nullptr);
        delete mManager;
        delete mSettings;
        delete mUploader;
        delete mHistoryDir;
    }

    void init()
    {
        QApplication::clipboard()->clear();
    }

    // getName: Numeric

    void getName_numeric_emptyDirectory()
    {
        QTemporaryDir dir;
        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric), "screenshot", QDir(dir.path())), QString("screenshot1"));
    }

    void getName_numeric_existingFiles()
    {
        QTemporaryDir dir;
        touch(dir.path(), "screenshot1.png");
        touch(dir.path(), "screenshot3.png");
        touch(dir.path(), "other9.png");
        QDir(dir.path()).mkdir("screenshot50"); // directories are not counted

        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric), "screenshot", QDir(dir.path())), QString("screenshot4"));
    }

    // Any extension counts, not only the current format's.
    void getName_numeric_otherExtensionsCount()
    {
        QTemporaryDir dir;
        touch(dir.path(), "screenshot2.png");
        touch(dir.path(), "screenshot7.jpg");

        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric), "screenshot", QDir(dir.path())), QString("screenshot8"));
    }

    // Names whose remainder is not a plain number (including the " (n)" collision names) count as 0.
    void getName_numeric_nonNumericIgnored()
    {
        QTemporaryDir dir;
        touch(dir.path(), "screenshot (5).png");
        touch(dir.path(), "screenshotabc.png");
        touch(dir.path(), "screenshot");

        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric), "screenshot", QDir(dir.path())), QString("screenshot1"));
    }

    // The prefix is matched anywhere in the name and every occurrence is removed, so flipped and unflipped names
    // share one counter.
    void getName_numeric_prefixMatchedAnywhere()
    {
        QTemporaryDir dir;
        touch(dir.path(), "5screenshot.png");

        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric), "screenshot", QDir(dir.path())), QString("screenshot6"));
        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric, true), "screenshot", QDir(dir.path())), QString("6screenshot"));
    }

    // An empty prefix matches every file.
    void getName_numeric_emptyPrefix()
    {
        QTemporaryDir dir;
        touch(dir.path(), "3.png");
        touch(dir.path(), "10.bmp");
        touch(dir.path(), "name.png");

        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric), "", QDir(dir.path())), QString("11"));
    }

    void getName_numeric_leadingZeros_data()
    {
        QTest::addColumn<int>("leadingZeros");
        QTest::addColumn<QString>("existing");
        QTest::addColumn<QString>("expected");

        // The field width is leadingZeros + 1.
        QTest::newRow("1, empty") << 1 << "" << "screenshot01";
        QTest::newRow("2, empty") << 2 << "" << "screenshot001";
        QTest::newRow("2, after 009") << 2 << "screenshot009.png" << "screenshot010";
        QTest::newRow("1, wider than the field") << 1 << "screenshot120.png" << "screenshot121";
    }

    void getName_numeric_leadingZeros()
    {
        QFETCH(int, leadingZeros);
        QFETCH(QString, existing);
        QFETCH(QString, expected);

        QTemporaryDir dir;

        if (!existing.isEmpty()) {
            touch(dir.path(), existing);
        }

        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric, false, leadingZeros), "screenshot", QDir(dir.path())), expected);
    }

    void getName_numeric_flip()
    {
        QTemporaryDir dir;
        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric, true), "screenshot", QDir(dir.path())), QString("1screenshot"));

        touch(dir.path(), "4screenshot.png");
        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric, true), "screenshot", QDir(dir.path())), QString("5screenshot"));
        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric, true, 2), "screenshot", QDir(dir.path())), QString("005screenshot"));
    }

    // The prefix goes through QString::arg, so a "%1" in it is replaced too.
    void getName_percentInPrefix()
    {
        QTemporaryDir dir;
        QCOMPARE(Screenshot::getName(naming(Screenshot::Numeric), "a%1b", QDir(dir.path())), QString("a1b1"));
        QCOMPARE(Screenshot::getName(naming(Screenshot::Empty), "a%1b", QDir(dir.path())), QString("ab"));
    }

    // getName: Date, Timestamp, Empty

    void getName_date()
    {
        QTemporaryDir dir;
        const QString format = "yyyy-MM-dd";

        const QString before = QLocale().toString(QDateTime::currentDateTime(), format);
        const QString name = Screenshot::getName(naming(Screenshot::Date, false, 0, format), "shot_", QDir(dir.path()));
        const QString flipped = Screenshot::getName(naming(Screenshot::Date, true, 0, format), "shot_", QDir(dir.path()));
        const QString after = QLocale().toString(QDateTime::currentDateTime(), format);

        QVERIFY2(name == "shot_" + before || name == "shot_" + after, qPrintable(name));
        QVERIFY2(flipped == before + "shot_" || flipped == after + "shot_", qPrintable(flipped));
    }

    // Seconds since the epoch; only the shape is checked.
    void getName_timestamp()
    {
        QTemporaryDir dir;

        const qint64 before = QDateTime::currentSecsSinceEpoch();
        const QString name = Screenshot::getName(naming(Screenshot::Timestamp), "shot_", QDir(dir.path()));
        const QString flipped = Screenshot::getName(naming(Screenshot::Timestamp, true), "shot_", QDir(dir.path()));
        const qint64 after = QDateTime::currentSecsSinceEpoch();

        QRegularExpressionMatch match = QRegularExpression("^shot_(\\d+)$").match(name);
        QVERIFY2(match.hasMatch(), qPrintable(name));
        QVERIFY(match.captured(1).toLongLong() >= before && match.captured(1).toLongLong() <= after);

        match = QRegularExpression("^(\\d+)shot_$").match(flipped);
        QVERIFY2(match.hasMatch(), qPrintable(flipped));
        QVERIFY(match.captured(1).toLongLong() >= before && match.captured(1).toLongLong() <= after);
    }

    void getName_empty()
    {
        QTemporaryDir dir;
        touch(dir.path(), "shot_.png");

        QCOMPARE(Screenshot::getName(naming(Screenshot::Empty), "shot_", QDir(dir.path())), QString("shot_"));
        QCOMPARE(Screenshot::getName(naming(Screenshot::Empty, true), "shot_", QDir(dir.path())), QString("shot_"));
    }

    // extensionFor

    void extensionFor_data()
    {
        QTest::addColumn<int>("format");
        QTest::addColumn<QString>("expected");

        QTest::newRow("PNG") << int(Screenshot::PNG) << ".png";
        QTest::newRow("JPEG") << int(Screenshot::JPEG) << ".jpg";
        QTest::newRow("BMP") << int(Screenshot::BMP) << ".bmp";
        QTest::newRow("WEBP") << int(Screenshot::WEBP) << ".webp";
    }

    void extensionFor()
    {
        QFETCH(int, format);
        QFETCH(QString, expected);

        QCOMPARE(Screenshot::extensionFor(Screenshot::Format(format)), expected);
    }

    // Bug 2 (docs/known-bugs.md), fixed in Stage 5: a format outside 0-3 falls back to PNG. -1 is what the options
    // dialog saves for WEBP (bug 19).
    void extensionFor_outOfRange_data()
    {
        QTest::addColumn<int>("format");

        QTest::newRow("7") << 7;
        QTest::newRow("4") << 4;
        QTest::newRow("-1") << -1;
    }

    void extensionFor_outOfRange()
    {
        QFETCH(int, format);

        QCOMPARE(Screenshot::extensionFor(Screenshot::Format(format)), QString(".png"));
    }

    void formatFromSetting_data()
    {
        QTest::addColumn<QVariant>("value");
        QTest::addColumn<int>("expected");

        QTest::newRow("PNG") << QVariant(0) << int(Screenshot::PNG);
        QTest::newRow("JPEG") << QVariant(1) << int(Screenshot::JPEG);
        QTest::newRow("BMP") << QVariant(2) << int(Screenshot::BMP);
        QTest::newRow("WEBP") << QVariant(3) << int(Screenshot::WEBP);
        QTest::newRow("INI string") << QVariant(QStringLiteral("1")) << int(Screenshot::JPEG);
        QTest::newRow("7") << QVariant(7) << int(Screenshot::PNG);
        QTest::newRow("-1") << QVariant(-1) << int(Screenshot::PNG);
        QTest::newRow("text") << QVariant(QStringLiteral("jpg")) << int(Screenshot::PNG);
        QTest::newRow("missing") << QVariant() << int(Screenshot::PNG);
    }

    // What the window uses to read "file/format".
    void formatFromSetting()
    {
        QFETCH(QVariant, value);
        QFETCH(int, expected);

        QCOMPARE(int(Screenshot::formatFromSetting(value)), expected);
    }

    // A Screenshot built with an out-of-range format saves a PNG, with PNG's quality rule (bug 20).
    void save_outOfRangeFormat_data()
    {
        extensionFor_outOfRange_data();
    }

    void save_outOfRangeFormat()
    {
        QFETCH(int, format);

        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.file = true;
        opts.format = Screenshot::Format(format);
        opts.quality = 10;

        Screenshot screenshot(nullptr, opts);
        QCOMPARE(screenshot.options().format, Screenshot::PNG);
        QCOMPARE(screenshot.options().quality, 80);

        screenshot.pixmap() = testPixmap();
        screenshot.save();

        const QString expected = native(dir.filePath("shot.png"));
        QCOMPARE(screenshot.options().result, Screenshot::Success);
        QCOMPARE(screenshot.options().fileName, expected);
        QCOMPARE(QImageReader(expected).format(), QByteArray("png"));
    }

    // uniqueName

    void uniqueName_firstCollision()
    {
        QTemporaryDir dir;
        touch(dir.path(), "shot.png");

        QCOMPARE(Screenshot::uniqueName(dir.filePath("shot"), ".png"), dir.filePath("shot (1)"));
    }

    // One more than the largest number found, not the first free one.
    void uniqueName_largestPlusOne()
    {
        QTemporaryDir dir;
        touch(dir.path(), "shot.png");
        touch(dir.path(), "shot (1).png");
        touch(dir.path(), "shot (4).png");

        QCOMPARE(Screenshot::uniqueName(dir.filePath("shot"), ".png"), dir.filePath("shot (5)"));
    }

    // Only the given extension is stripped; numbers on files with another extension do not parse and count as 0.
    void uniqueName_otherExtensionIgnored()
    {
        QTemporaryDir dir;
        touch(dir.path(), "shot.png");
        touch(dir.path(), "shot (3).jpg");

        QCOMPARE(Screenshot::uniqueName(dir.filePath("shot"), ".png"), dir.filePath("shot (1)"));
    }

    // Quirk: any file containing the name counts, so a longer numeric name raises the suffix.
    void uniqueName_prefixMatch()
    {
        QTemporaryDir dir;
        touch(dir.path(), "screenshot1.png");
        touch(dir.path(), "screenshot12.png");

        QCOMPARE(Screenshot::uniqueName(dir.filePath("screenshot1"), ".png"), dir.filePath("screenshot1 (3)"));
    }

    // Quirk: every " (" and ")" is removed and the digits left over are joined.
    void uniqueName_nestedSuffix()
    {
        QTemporaryDir dir;
        touch(dir.path(), "shot.png");
        touch(dir.path(), "shot (1) (1).png");

        QCOMPARE(Screenshot::uniqueName(dir.filePath("shot"), ".png"), dir.filePath("shot (12)"));
    }

    // Quirk: every occurrence of the name is removed, digits inside the suffix included. With the name "2",
    // "2 (2).png" reads as 0, so the result names a file that already exists (save() would overwrite it).
    void uniqueName_returnsExistingName()
    {
        QTemporaryDir dir;
        touch(dir.path(), "2.png");
        touch(dir.path(), "2 (1).png");
        touch(dir.path(), "2 (2).png");

        const QString name = Screenshot::uniqueName(dir.filePath("2"), ".png");
        QCOMPARE(name, dir.filePath("2 (2)"));
        QVERIFY(QFile::exists(name + ".png"));
    }

    // Files in other directories are not looked at.
    void uniqueName_onlyItsDirectory()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkdir("sub");
        touch(dir.path(), "shot (7).png");
        touch(dir.filePath("sub"), "shot.png");

        QCOMPARE(Screenshot::uniqueName(dir.filePath("sub/shot"), ".png"), dir.filePath("sub/shot (1)"));
    }

    // save()

    void save_fileOnly()
    {
        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.file = true;

        Screenshot screenshot(nullptr, opts);
        QStringList log;
        record(&screenshot, &log);
        screenshot.pixmap() = testPixmap();
        screenshot.save();

        const QString expected = native(dir.filePath("shot.png"));
        QCOMPARE(screenshot.options().result, Screenshot::Success);
        QCOMPARE(screenshot.options().fileName, expected);
        QVERIFY(QFile::exists(expected));
        QCOMPARE(QImageReader(expected).size(), QSize(64, 48));
        QCOMPARE(log, QStringList() << "finished");
        QVERIFY(QApplication::clipboard()->pixmap().isNull());
        QCOMPARE(historyRows(expected), 1);
    }

    void save_clipboardOnly()
    {
        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.clipboard = true;

        Screenshot screenshot(nullptr, opts);
        QStringList log;
        record(&screenshot, &log);
        screenshot.pixmap() = testPixmap();
        const int rows = historyRows();
        screenshot.save();

        QCOMPARE(screenshot.options().result, Screenshot::Success);
        QCOMPARE(screenshot.options().fileName, QString());
        QCOMPARE(QApplication::clipboard()->pixmap().size(), QSize(64, 48));
        QVERIFY(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).isEmpty());
        QCOMPARE(log, QStringList() << "finished");
        QCOMPARE(historyRows(), rows);
    }

    void save_fileAndClipboard()
    {
        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.file = true;
        opts.clipboard = true;

        Screenshot screenshot(nullptr, opts);
        QStringList log;
        record(&screenshot, &log);
        screenshot.pixmap() = testPixmap();
        screenshot.save();

        const QString expected = native(dir.filePath("shot.png"));
        QCOMPARE(screenshot.options().result, Screenshot::Success);
        QCOMPARE(screenshot.options().fileName, expected);
        QVERIFY(QFile::exists(expected));
        QCOMPARE(QApplication::clipboard()->pixmap().size(), QSize(64, 48));
        QCOMPARE(log, QStringList() << "finished");
        QCOMPARE(historyRows(expected), 1);
    }

    // Quirk: with neither file nor clipboard the result is Failure and finished is emitted twice.
    void save_neither()
    {
        QTemporaryDir dir;
        Screenshot screenshot(nullptr, options(dir.path()));
        QStringList log;
        record(&screenshot, &log);
        screenshot.pixmap() = testPixmap();
        screenshot.save();

        QCOMPARE(screenshot.options().result, Screenshot::Failure);
        QCOMPARE(screenshot.options().fileName, QString());
        QCOMPARE(log, QStringList() << "finished" << "finished");
    }

    // Quirk: a failed file save emits finished twice and still writes a history row for the file.
    void save_failure()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkdir("shot.png"); // a directory where the file should go
        auto opts = options(dir.path());
        opts.file = true;
        opts.replace = true;

        Screenshot screenshot(nullptr, opts);
        QStringList log;
        record(&screenshot, &log);
        screenshot.pixmap() = testPixmap();
        screenshot.save();

        const QString expected = native(dir.filePath("shot.png"));
        QCOMPARE(screenshot.options().result, Screenshot::Failure);
        QCOMPARE(screenshot.options().fileName, expected);
        QCOMPARE(log, QStringList() << "finished" << "finished");
        QCOMPARE(historyRows(expected), 1);
    }

    void save_replaceOff()
    {
        QTemporaryDir dir;
        touch(dir.path(), "shot.png");
        auto opts = options(dir.path());
        opts.file = true;

        Screenshot screenshot(nullptr, opts);
        screenshot.pixmap() = testPixmap();
        screenshot.save();

        const QString expected = native(dir.filePath("shot (1).png"));
        QCOMPARE(screenshot.options().result, Screenshot::Success);
        QCOMPARE(screenshot.options().fileName, expected);
        QCOMPARE(QImageReader(expected).size(), QSize(64, 48));
        QCOMPARE(QFileInfo(dir.filePath("shot.png")).size(), qint64(1)); // untouched
    }

    void save_replaceOn()
    {
        QTemporaryDir dir;
        touch(dir.path(), "shot.png");
        auto opts = options(dir.path());
        opts.file = true;
        opts.replace = true;

        Screenshot screenshot(nullptr, opts);
        screenshot.pixmap() = testPixmap();
        screenshot.save();

        const QString expected = native(dir.filePath("shot.png"));
        QCOMPARE(screenshot.options().result, Screenshot::Success);
        QCOMPARE(screenshot.options().fileName, expected);
        QCOMPARE(QImageReader(expected).size(), QSize(64, 48));
        QVERIFY(!QFile::exists(dir.filePath("shot (1).png")));
    }

    void save_createsDirectory()
    {
        QTemporaryDir dir;
        auto opts = options(dir.filePath("a/b"));
        opts.file = true;

        Screenshot screenshot(nullptr, opts);
        screenshot.pixmap() = testPixmap();
        screenshot.save();

        QCOMPARE(screenshot.options().result, Screenshot::Success);
        QVERIFY(QFile::exists(dir.filePath("a/b/shot.png")));
    }

    void save_numericNaming()
    {
        QTemporaryDir dir;
        touch(dir.path(), "shot4.png");
        auto opts = options(dir.path());
        opts.file = true;
        opts.namingOptions = naming(Screenshot::Numeric);

        Screenshot screenshot(nullptr, opts);
        screenshot.pixmap() = testPixmap();
        screenshot.save();

        QCOMPARE(screenshot.options().fileName, native(dir.filePath("shot5.png")));
    }

    void save_formats_data()
    {
        QTest::addColumn<int>("format");
        QTest::addColumn<QString>("extension");
        QTest::addColumn<QByteArray>("readerFormat");

        QTest::newRow("PNG") << int(Screenshot::PNG) << ".png" << QByteArray("png");
        QTest::newRow("JPEG") << int(Screenshot::JPEG) << ".jpg" << QByteArray("jpeg");
        QTest::newRow("BMP") << int(Screenshot::BMP) << ".bmp" << QByteArray("bmp");
        QTest::newRow("WEBP") << int(Screenshot::WEBP) << ".webp" << QByteArray("webp");
    }

    // The format follows from the extension. WEBP needs the qtimageformats plugin; without it the save fails.
    void save_formats()
    {
        QFETCH(int, format);
        QFETCH(QString, extension);
        QFETCH(QByteArray, readerFormat);

        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.file = true;
        opts.format = Screenshot::Format(format);
        opts.quality = 50;

        Screenshot screenshot(nullptr, opts);
        screenshot.pixmap() = testPixmap();
        screenshot.save();

        const QString expected = native(dir.filePath("shot" + extension));
        QCOMPARE(screenshot.options().fileName, expected);

        if (QImageWriter::supportedImageFormats().contains(readerFormat)) {
            QCOMPARE(screenshot.options().result, Screenshot::Success);
            QImageReader reader(expected);
            QCOMPARE(reader.format(), readerFormat);
            QCOMPARE(reader.size(), QSize(64, 48));
        } else {
            QCOMPARE(screenshot.options().result, Screenshot::Failure);
            QVERIFY(!QFile::exists(expected));
        }
    }

    // The constructor forces quality 80 for PNG and keeps it for the other formats.
    void quality_pngForcedTo80()
    {
        QTemporaryDir dir;

        for (int format : { Screenshot::PNG, Screenshot::JPEG, Screenshot::BMP, Screenshot::WEBP }) {
            auto opts = options(dir.path());
            opts.format = Screenshot::Format(format);
            opts.quality = 10;
            Screenshot screenshot(nullptr, opts);
            QCOMPARE(screenshot.options().quality, format == Screenshot::PNG ? 80 : 10);
        }
    }

    void quality_jpeg()
    {
        QTemporaryDir low, high;
        qint64 sizes[2];
        QTemporaryDir *dirs[2] = { &low, &high };
        const int qualities[2] = { 5, 95 };

        for (int i = 0; i < 2; ++i) {
            auto opts = options(dirs[i]->path());
            opts.file = true;
            opts.format = Screenshot::JPEG;
            opts.quality = qualities[i];
            Screenshot screenshot(nullptr, opts);
            screenshot.pixmap() = testPixmap();
            screenshot.save();
            QCOMPARE(screenshot.options().result, Screenshot::Success);
            sizes[i] = QFileInfo(dirs[i]->filePath("shot.jpg")).size();
        }

        QVERIFY2(sizes[0] < sizes[1], qPrintable(QString("%1 >= %2").arg(sizes[0]).arg(sizes[1])));
    }

    // confirmation(), unloadPixmap() and the .screenshot. temporary file

    // With file saving on, confirmation() writes the pixmap to <directory>/.screenshot.<number><extension> and drops it.
    void unload_tempFile()
    {
        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.file = true;

        auto *screenshot = new Screenshot(nullptr, opts);
        QStringList log;
        record(screenshot, &log);
        screenshot->setPixmap(testPixmap()); // calls confirmation()

        const QString temp = screenshot->unloadedFileName();
        QVERIFY2(QRegularExpression("^" + QRegularExpression::escape(dir.path()) + "/\\.screenshot\\.\\d+\\.png$").match(temp).hasMatch(),
                 qPrintable(temp));
        QVERIFY(QFile::exists(temp));
        QCOMPARE(QImageReader(temp).size(), QSize(64, 48));
        QVERIFY(screenshot->pixmap().isNull());
        QCOMPARE(log, QStringList() << "askConfirmation");

        // The destructor deletes it.
        delete screenshot;
        QVERIFY(!QFile::exists(temp));
    }

    // The temp file name follows the format.
    void unload_tempFileExtension()
    {
        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.file = true;
        opts.format = Screenshot::BMP;

        Screenshot screenshot(nullptr, opts);
        screenshot.setPixmap(testPixmap());
        QVERIFY2(screenshot.unloadedFileName().endsWith(".bmp"), qPrintable(screenshot.unloadedFileName()));
    }

    // Without file saving, the pixmap stays in memory.
    void unload_notForClipboardOnly()
    {
        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.clipboard = true;

        Screenshot screenshot(nullptr, opts);
        screenshot.setPixmap(testPixmap());

        QVERIFY(screenshot.unloadedFileName().isEmpty());
        QVERIFY(!screenshot.pixmap().isNull());
        QVERIFY(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).isEmpty());
    }

    // confirm(true) after unloading, file only: the temp file is renamed to the final name.
    void confirm_true_afterUnload()
    {
        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.file = true;

        Screenshot screenshot(nullptr, opts);
        QStringList log;
        record(&screenshot, &log);
        screenshot.setPixmap(testPixmap());
        const QString temp = screenshot.unloadedFileName();
        QVERIFY(QFile::exists(temp));

        screenshot.confirm(true);

        const QString expected = native(dir.filePath("shot.png"));
        QCOMPARE(screenshot.options().result, Screenshot::Success);
        QCOMPARE(screenshot.options().fileName, expected);
        QVERIFY(!QFile::exists(temp));
        QCOMPARE(QImageReader(expected).size(), QSize(64, 48));
        QCOMPARE(log, QStringList() << "askConfirmation" << "finished" << "cleanup");
        QVERIFY(screenshot.pixmap().isNull());
        QCOMPARE(historyRows(expected), 1);
    }

    // Quirk: with the clipboard on as well, save() reloads the pixmap from the temp file for the clipboard and then
    // saves it again under the final name instead of renaming, so the temp file stays until the destructor.
    void confirm_true_afterUnload_withClipboard()
    {
        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.file = true;
        opts.clipboard = true;

        auto *screenshot = new Screenshot(nullptr, opts);
        QStringList log;
        record(screenshot, &log);
        screenshot->setPixmap(testPixmap());
        const QString temp = screenshot->unloadedFileName();
        QVERIFY(QFile::exists(temp));

        screenshot->confirm(true);

        const QString expected = native(dir.filePath("shot.png"));
        QCOMPARE(screenshot->options().result, Screenshot::Success);
        QCOMPARE(screenshot->options().fileName, expected);
        QCOMPARE(QImageReader(expected).size(), QSize(64, 48));
        QCOMPARE(QApplication::clipboard()->pixmap().size(), QSize(64, 48));
        QCOMPARE(log, QStringList() << "askConfirmation" << "finished" << "cleanup");
        QVERIFY(QFile::exists(temp));

        delete screenshot;
        QVERIFY(!QFile::exists(temp));
    }

    // confirm(false): result Cancel, finished then cleanup, nothing saved. The temp file stays until the destructor.
    void confirm_false()
    {
        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.file = true;

        auto *screenshot = new Screenshot(nullptr, opts);
        QStringList log;
        record(screenshot, &log);
        screenshot->setPixmap(testPixmap());
        const QString temp = screenshot->unloadedFileName();

        screenshot->confirm(false);

        QCOMPARE(screenshot->options().result, Screenshot::Cancel);
        QCOMPARE(screenshot->options().fileName, QString());
        QCOMPARE(log, QStringList() << "askConfirmation" << "finished" << "cleanup");
        QVERIFY(!QFile::exists(dir.filePath("shot.png")));
        QVERIFY(QFile::exists(temp));

        delete screenshot;
        QVERIFY(!QFile::exists(temp));
    }

    // A null pixmap cancels straight away.
    void setPixmap_null()
    {
        QTemporaryDir dir;
        auto opts = options(dir.path());
        opts.file = true;

        Screenshot screenshot(nullptr, opts);
        QStringList log;
        record(&screenshot, &log);
        screenshot.setPixmap(QPixmap());

        QCOMPARE(screenshot.options().result, Screenshot::Cancel);
        QCOMPARE(log, QStringList() << "finished" << "cleanup");
        QVERIFY(screenshot.unloadedFileName().isEmpty());
    }
};

QTEST_MAIN(tst_Screenshot)
#include "tst_screenshot.moc"
