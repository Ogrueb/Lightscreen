// tools/optimizer, the PNG optimization queue (S05, bug 10), with a stub built as both oxipng and optipng
// (tests/optimizer_stub): one process at a time in order, the temporary file that replaces the original only on
// success, optimizationDone exactly once per file, the pending list that survives a restart, the choice between the
// two tools with its fallback, and the screenshot flow through the manager. Until S05 this was tst_optimize, which
// tested Screenshot::optimize with an optipng stub; those cases are kept here, against both tools.
//
// Where the app looks for the tools: on PATH on Linux, next to the executable as <tool>.exe on Windows. The test puts
// the stubs there and takes them away for the missing-binary cases.
#include <QtTest>
#include <QImage>
#include <QSettings>
#include <QTemporaryDir>

#include <tools/optimizer.h>
#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>

class tst_Optimizer : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mEmptyDir;
    QByteArray mPath;
    QTemporaryDir *mDir = nullptr;
    QSettings *mSettings = nullptr;

    static Screenshot::Options options(const QString &fileName)
    {
        Screenshot::Options options;
        options.fileName = fileName;
        options.result = Screenshot::Success;
        options.format = Screenshot::PNG;
        options.namingOptions = {Screenshot::Empty, false, 0, QString()};
        options.mode = Screenshot::None;
        options.quality = 90;
        options.animations = false;
        options.clipboard = false;
        options.urlClipboard = false;
        options.currentMonitor = false;
        options.cursor = false;
        options.file = true;
        options.magnify = false;
        options.optimize = true;
        options.preview = false;
        options.replace = false;
        options.saveAs = false;
        options.upload = false;
        return options;
    }

    static QByteArray readAll(const QString &fileName)
    {
        QFile file(fileName);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    static void write(const QString &fileName, const QByteArray &data)
    {
        QFile file(fileName);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(data);
    }

    static QStringList lines(const QString &log)
    {
        return QString::fromLocal8Bit(readAll(log)).split('\n', Qt::SkipEmptyParts);
    }

    static QStringList starts(const QString &log)
    {
        return lines(log).filter(QRegularExpression("^start "));
    }

    static QString stub(const QString &tool)
    {
#ifdef Q_OS_WIN
        return QStringLiteral(OPTIMIZER_STUB_DIR) + "/" + tool + "/" + tool + ".exe";
#else
        return QStringLiteral(OPTIMIZER_STUB_DIR) + "/" + tool + "/" + tool;
#endif
    }

    // Makes exactly these stubs findable where the app looks.
    void install(const QStringList &tools)
    {
#ifdef Q_OS_WIN
        for (const QString &tool : Optimizer::toolNames()) {
            const QString target = QCoreApplication::applicationDirPath() + "/" + tool + ".exe";
            QFile::remove(target);

            if (tools.contains(tool)) {
                QVERIFY(QFile::copy(stub(tool), target));
            }
        }
#else
        // Without stubs, PATH is only an empty directory, so no installed tool is found either.
        QByteArrayList path;

        for (const QString &tool : tools) {
            path << QFile::encodeName(QStringLiteral(OPTIMIZER_STUB_DIR) + "/" + tool);
        }

        path << QFile::encodeName(mEmptyDir.path());
        qputenv("PATH", path.join(':'));
#endif
    }

    Optimizer *optimizer()
    {
        return new Optimizer(mSettings, mDir->filePath("data"));
    }

    // Waits until done has count entries, then a little longer for any extra emission.
    static void settle(QSignalSpy &done, int count)
    {
        QTRY_COMPARE_WITH_TIMEOUT(done.count(), count, 10000);
        QTest::qWait(200);
    }

    static QStringList doneFiles(const QSignalSpy &done)
    {
        QStringList files;

        for (const QList<QVariant> &arguments : done) {
            files << arguments.first().toString();
        }

        return files;
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(mEmptyDir.isValid());
        mPath = qgetenv("PATH");

        for (const QString &tool : Optimizer::toolNames()) {
            QVERIFY2(QFile::exists(stub(tool)), qPrintable(stub(tool)));
        }
    }

    void init()
    {
        mDir = new QTemporaryDir;
        QVERIFY(mDir->isValid());
        mSettings = new QSettings(mDir->filePath("config/settings.ini"), QSettings::IniFormat);
    }

    void cleanup()
    {
        qunsetenv("OPTIMIZER_STUB_MODE");
        qunsetenv("OPTIMIZER_STUB_LOG");
        qunsetenv("OPTIMIZER_STUB_SLEEP");
#ifdef Q_OS_WIN
        for (const QString &tool : Optimizer::toolNames()) {
            QFile::remove(QCoreApplication::applicationDirPath() + "/" + tool + ".exe");
        }
#else
        qputenv("PATH", mPath);
#endif
        delete mSettings;
        mSettings = nullptr;
        delete mDir;
        mDir = nullptr;
    }

    void tools_data()
    {
        QTest::addColumn<QString>("tool");
        QTest::addColumn<QString>("expectedArguments");

        QTest::newRow("oxipng") << "oxipng" << "-q --out";
        QTest::newRow("optipng") << "optipng" << "-quiet -out";
    }

    // The tool writes a temporary file next to the original, which then replaces it; done fires once.
    void success_data() { tools_data(); }
    void success()
    {
        QFETCH(QString, tool);
        QFETCH(QString, expectedArguments);

        const QString file = mDir->filePath("shot.png");
        const QString log = mDir->filePath("log.txt");
        write(file, "original");
        qputenv("OPTIMIZER_STUB_LOG", QFile::encodeName(log));
        install({ tool });

        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);
        o->add(file);
        settle(done, 1);

        QCOMPARE(doneFiles(done), QStringList{ file });
        QCOMPARE(readAll(file), QByteArray("original") + tool.toLatin1() + "-stub");
        const QStringList ran = starts(log);
        QCOMPARE(ran.size(), 1);
        QCOMPARE(ran.first(), QString("start %1 %2 %3 %4").arg(tool, expectedArguments, Optimizer::temporaryFileName(file), file));
        QCOMPARE(QDir(mDir->path()).entryList(QDir::Files).join(","), QString("log.txt,shot.png"));
        QCOMPARE(o->pending(), 0);
    }

    // A temporary file left behind by an interrupted run does not stop the next one.
    void success_staleTemporary_data() { tools_data(); }
    void success_staleTemporary()
    {
        QFETCH(QString, tool);

        const QString file = mDir->filePath("shot.png");
        write(file, "original");
        write(Optimizer::temporaryFileName(file), "stale");
        install({ tool });

        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);
        o->add(file);
        settle(done, 1);

        QCOMPARE(readAll(file), QByteArray("original") + tool.toLatin1() + "-stub");
        QVERIFY(!QFile::exists(Optimizer::temporaryFileName(file)));
    }

    // The tool exits non-zero after writing part of its output: the original is untouched and nothing is left over.
    void failure_keepsOriginal_data() { tools_data(); }
    void failure_keepsOriginal()
    {
        QFETCH(QString, tool);

        const QString file = mDir->filePath("shot.png");
        write(file, "original");
        qputenv("OPTIMIZER_STUB_MODE", "fail");
        install({ tool });

        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);
        o->add(file);
        settle(done, 1);

        QCOMPARE(readAll(file), QByteArray("original"));
        QCOMPARE(QDir(mDir->path()).entryList(QDir::Files).join(","), QString("shot.png"));
    }

    // The tool exits 0 without writing an output: the original stays as it is.
    void noOutput_keepsOriginal_data() { tools_data(); }
    void noOutput_keepsOriginal()
    {
        QFETCH(QString, tool);

        const QString file = mDir->filePath("shot.png");
        write(file, "original");
        qputenv("OPTIMIZER_STUB_MODE", "nooutput");
        install({ tool });

        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);
        o->add(file);
        settle(done, 1);

        QCOMPARE(readAll(file), QByteArray("original"));
        QCOMPARE(QDir(mDir->path()).entryList(QDir::Files).join(","), QString("shot.png"));
    }

    // Neither tool to be found: done fires once per file, in order, and the files are untouched.
    void missingBinary()
    {
        const QStringList files = { mDir->filePath("a.png"), mDir->filePath("b.png"), mDir->filePath("c.png") };

        for (const QString &file : files) {
            write(file, "original");
        }

        install({});

        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);

        for (const QString &file : files) {
            o->add(file);
        }

        settle(done, 3);
        QCOMPARE(doneFiles(done), files);

        for (const QString &file : files) {
            QCOMPARE(readAll(file), QByteArray("original"));
        }

        QCOMPARE(o->pending(), 0);
    }

    // A file deleted before its turn is skipped without starting anything.
    void missingFile()
    {
        const QString file = mDir->filePath("gone.png");
        const QString log = mDir->filePath("log.txt");
        qputenv("OPTIMIZER_STUB_LOG", QFile::encodeName(log));
        install(Optimizer::toolNames());

        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);
        o->add(file);
        settle(done, 1);

        QVERIFY(starts(log).isEmpty());
    }

    // A burst of five files runs one process at a time, in order, and each file is reported once.
    void burst_oneAtATime()
    {
        const QString log = mDir->filePath("log.txt");
        qputenv("OPTIMIZER_STUB_LOG", QFile::encodeName(log));
        qputenv("OPTIMIZER_STUB_SLEEP", "100");
        install({ "oxipng" });

        QStringList files;

        for (int i = 0; i < 5; ++i) {
            files << mDir->filePath(QString("shot%1.png").arg(i));
            write(files.last(), "original");
        }

        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);
        QSignalSpy pending(o.data(), &Optimizer::pendingChanged);

        for (const QString &file : files) {
            o->add(file);
        }

        QCOMPARE(o->pending(), 5);
        settle(done, 5);

        QCOMPARE(doneFiles(done), files);
        QCOMPARE(pending.last().first().toInt(), 0);

        // "start" and "end" alternate: no second process starts before the first one has ended.
        const QStringList ran = lines(log);
        QCOMPARE(ran.size(), 10);

        for (int i = 0; i < ran.size(); i += 2) {
            QVERIFY2(ran.at(i).startsWith("start ") && ran.at(i).endsWith(" " + files.at(i / 2)), qPrintable(ran.join('\n')));
            QCOMPARE(ran.at(i + 1), "end " + files.at(i / 2));
        }

        QVERIFY(!QFile::exists(o->pendingFileName()));
    }

    // A file whose optimization never finishes is still pending after the optimizer is destroyed (the app stops), its
    // original intact, and runs when the next optimizer resumes.
    void pending_survivesRestart()
    {
        const QString file = mDir->filePath("shot.png");
        const QString log = mDir->filePath("log.txt");
        write(file, "original");
        qputenv("OPTIMIZER_STUB_LOG", QFile::encodeName(log));
        qputenv("OPTIMIZER_STUB_MODE", "hang");
        install({ "optipng" });

        QString pendingFile;

        {
            QScopedPointer<Optimizer> o(optimizer());
            pendingFile = o->pendingFileName();
            QSignalSpy done(o.data(), &Optimizer::optimizationDone);
            o->add(file);
            QTRY_COMPARE(starts(log).size(), 1);
            QCOMPARE(o->pending(), 1);
            QCOMPARE(lines(pendingFile), QStringList{ file });
            QVERIFY(done.isEmpty());
        }

        QCOMPARE(lines(pendingFile), QStringList{ file });
        QCOMPARE(readAll(file), QByteArray("original"));
        QVERIFY(!QFile::exists(Optimizer::temporaryFileName(file)));

        qunsetenv("OPTIMIZER_STUB_MODE");
        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);
        o->resume();
        settle(done, 1);

        QCOMPARE(doneFiles(done), QStringList{ file });
        QCOMPARE(readAll(file), QByteArray("originaloptipng-stub"));
        QVERIFY(!QFile::exists(pendingFile));
    }

    // Nothing pending: resume does nothing.
    void resume_nothingPending()
    {
        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);
        o->resume();
        QCOMPARE(o->pending(), 0);
        QVERIFY(done.isEmpty());
    }

    // The tool that runs for each setting and set of installed tools ("options/optimizer").
    void choice_data()
    {
        QTest::addColumn<QVariant>("setting");
        QTest::addColumn<QStringList>("installed");
        QTest::addColumn<QString>("expected");

        QTest::newRow("absent, both") << QVariant() << QStringList{ "oxipng", "optipng" } << "oxipng";
        QTest::newRow("absent, optipng only") << QVariant() << QStringList{ "optipng" } << "optipng";
        QTest::newRow("oxipng, both") << QVariant("oxipng") << QStringList{ "oxipng", "optipng" } << "oxipng";
        QTest::newRow("optipng, both") << QVariant("optipng") << QStringList{ "oxipng", "optipng" } << "optipng";
        QTest::newRow("oxipng missing, falls back") << QVariant("oxipng") << QStringList{ "optipng" } << "optipng";
        QTest::newRow("optipng missing, falls back") << QVariant("optipng") << QStringList{ "oxipng" } << "oxipng";
        QTest::newRow("unknown value, both") << QVariant("pngcrush") << QStringList{ "oxipng", "optipng" } << "oxipng";
        QTest::newRow("oxipng, neither") << QVariant("oxipng") << QStringList() << "";
    }

    void choice()
    {
        QFETCH(QVariant, setting);
        QFETCH(QStringList, installed);
        QFETCH(QString, expected);

        const QString file = mDir->filePath("shot.png");
        const QString log = mDir->filePath("log.txt");
        write(file, "original");
        qputenv("OPTIMIZER_STUB_LOG", QFile::encodeName(log));
        install(installed);

        if (setting.isValid()) {
            mSettings->setValue("options/optimizer", setting);
        }

        QCOMPARE(Optimizer::resolveTool(setting.toString()), expected);

        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);
        o->add(file);
        settle(done, 1);

        if (expected.isEmpty()) {
            QVERIFY(starts(log).isEmpty());
        } else {
            QCOMPARE(starts(log).size(), 1);
            QVERIFY2(starts(log).first().startsWith("start " + expected + " "), qPrintable(starts(log).first()));
        }
    }

    // The screenshot flow: a saved PNG goes through the manager's optimizer, and the screenshot finishes once its file
    // is done. Two screenshots of the same file (replace mode) each finish once.
    void screenshotFlow()
    {
        const QString file = mDir->filePath("shot.png");
        write(file, "original");
        install({ "oxipng" });

        ScreenshotManager manager(mSettings, mDir->filePath("data") + "/");
        Screenshot first(nullptr, options(file));
        Screenshot second(nullptr, options(file));
        manager.connectScreenshot(&first);
        manager.connectScreenshot(&second);
        QSignalSpy firstFinished(&first, &Screenshot::finished);
        QSignalSpy secondFinished(&second, &Screenshot::finished);

        first.optimize();
        second.optimize();

        QTRY_COMPARE(firstFinished.count(), 1);
        QTRY_COMPARE(secondFinished.count(), 1);
        QTest::qWait(200);
        QCOMPARE(firstFinished.count(), 1);
        QCOMPARE(secondFinished.count(), 1);
        QCOMPARE(readAll(file), QByteArray("originaloxipng-stuboxipng-stub"));
        QCOMPARE(manager.optimizer()->pending(), 0);
    }

    // Quitting the app stops the manager's running optimizer: the tool does not go on to write its temporary file
    // (it was left in the screenshot folder, found in the Windows QA pass), the original is intact and the file is
    // still pending for the next start.
    void quit_stopsTheRunningTool()
    {
        const QString file = mDir->filePath("shot.png");
        const QString log = mDir->filePath("log.txt");
        write(file, "original");
        qputenv("OPTIMIZER_STUB_LOG", QFile::encodeName(log));
        qputenv("OPTIMIZER_STUB_SLEEP", "1500");
        install({ "oxipng" });

        ScreenshotManager manager(mSettings, mDir->filePath("data") + "/");
        manager.optimizer()->add(file);
        QTRY_COMPARE(starts(log).size(), 1);

        QTimer::singleShot(0, [] { QCoreApplication::exit(0); });
        QCoreApplication::exec();

        QTest::qWait(2500);
        QVERIFY(!QFile::exists(Optimizer::temporaryFileName(file)));
        QCOMPARE(readAll(file), QByteArray("original"));
        QCOMPARE(manager.optimizer()->pending(), 1);
        QCOMPARE(lines(manager.optimizer()->pendingFileName()), QStringList{ file });
    }

    // The real tools, when installed (Linux CI installs both): a PNG saved with little compression gets no larger and
    // keeps every pixel.
    void realTool_data()
    {
        QTest::addColumn<QString>("tool");
        QTest::newRow("oxipng") << "oxipng";
        QTest::newRow("optipng") << "optipng";
    }

    void realTool()
    {
        QFETCH(QString, tool);

        if (Optimizer::program(tool).isEmpty()) {
            QSKIP(qPrintable(tool + " is not installed"));
        }

        QImage image(320, 200, QImage::Format_RGB32);

        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                image.setPixel(x, y, qRgb(x % 256, (x / 8 + y / 8) % 2 ? 255 : 0, y));
            }
        }

        const QString file = mDir->filePath("real.png");
        QVERIFY(image.save(file, "PNG", 100));
        const qint64 before = QFileInfo(file).size();
        mSettings->setValue("options/optimizer", tool);

        QScopedPointer<Optimizer> o(optimizer());
        QSignalSpy done(o.data(), &Optimizer::optimizationDone);
        o->add(file);
        QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 60000);

        const qint64 after = QFileInfo(file).size();
        QVERIFY2(after <= before, qPrintable(QString("%1 -> %2").arg(before).arg(after)));
        QCOMPARE(QImage(file).convertToFormat(QImage::Format_RGB32), image);
        QVERIFY(!QFile::exists(Optimizer::temporaryFileName(file)));
    }
};

QTEST_MAIN(tst_Optimizer)
#include "tst_optimizer.moc"
