// Screenshot::optimize with a stub optipng (tests/optipng_stub): optimizationDone runs exactly once per call,
// optipng writes to a temporary file that replaces the original only on success, and a missing binary does not
// start anything (known-bugs.md, bug 10, the part fixed in Stage 5).
//
// Where the app looks for optipng: on PATH on Linux, next to the executable as optipng.exe on Windows. The test puts
// the stub there and takes it away for the missing-binary case.
#include <QtTest>
#include <QTemporaryDir>

#include <tools/screenshot.h>

class tst_Optimize : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mEmptyDir;
    QByteArray mPath;
    QString mStub;

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

    static QStringList runs(const QString &log)
    {
        return QString::fromLocal8Bit(readAll(log)).split('\n', Qt::SkipEmptyParts);
    }

    void installStub(bool installed)
    {
#ifdef Q_OS_WIN
        const QString target = QCoreApplication::applicationDirPath() + "/optipng.exe";
        QFile::remove(target);

        if (installed) {
            QVERIFY(QFile::copy(mStub, target));
        }
#else
        // Without the stub, PATH is only an empty directory, so no installed optipng is found either.
        qputenv("PATH", installed ? QByteArray(OPTIPNG_STUB_DIR) + ":" + mPath : QFile::encodeName(mEmptyDir.path()));
#endif
    }

    // Runs optimize() on fileName and waits until the stub has finished and the events have settled. Returns how
    // often finished() (what optimizationDone() emits without upload) fired.
    int optimize(const QString &fileName)
    {
        Screenshot screenshot(nullptr, options(fileName));
        QSignalSpy finished(&screenshot, &Screenshot::finished);
        screenshot.optimize();
        if (finished.isEmpty()) {
            finished.wait(5000);
        }

        // A second emission, if any, comes when the process exits; give it time.
        QTest::qWait(500);
        return finished.count();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(mEmptyDir.isValid());
        mPath = qgetenv("PATH");
#ifdef Q_OS_WIN
        mStub = QStringLiteral(OPTIPNG_STUB_DIR) + "/optipng.exe";
#else
        mStub = QStringLiteral(OPTIPNG_STUB_DIR) + "/optipng";
#endif
        QVERIFY2(QFile::exists(mStub), qPrintable(mStub));
    }

    void cleanup()
    {
        qunsetenv("OPTIPNG_STUB_MODE");
        qunsetenv("OPTIPNG_STUB_LOG");
#ifdef Q_OS_WIN
        QFile::remove(QCoreApplication::applicationDirPath() + "/optipng.exe");
#else
        qputenv("PATH", mPath);
#endif
    }

    // optipng writes a temporary file next to the original, which then replaces it; finished() fires once.
    void success()
    {
        QTemporaryDir dir;
        const QString file = dir.filePath("shot.png");
        const QString log = dir.filePath("log.txt");
        write(file, "original");
        qputenv("OPTIPNG_STUB_LOG", QFile::encodeName(log));
        installStub(true);

        const int finished = optimize(file);

        QCOMPARE(readAll(file), QByteArray("originaloptipng-stub"));
        const QStringList ran = runs(log);
        QCOMPARE(ran.size(), 1);
        QVERIFY2(ran.first().contains("-out ") && ran.first().endsWith(" " + file), qPrintable(ran.first()));
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files).join(","), QString("log.txt,shot.png"));
        QCOMPARE(finished, 1);
    }

    // A temporary file left behind by an interrupted run does not stop the next one.
    void success_staleTemporary()
    {
        QTemporaryDir dir;
        const QString file = dir.filePath("shot.png");
        const QString log = dir.filePath("log.txt");
        write(file, "original");
        qputenv("OPTIPNG_STUB_LOG", QFile::encodeName(log));
        installStub(true);

        QCOMPARE(optimize(file), 1);
        const QString temporary = runs(log).first().section(' ', 1, 1);
        write(file, "second");
        write(temporary, "stale");

        QCOMPARE(optimize(file), 1);
        QCOMPARE(readAll(file), QByteArray("secondoptipng-stub"));
        QVERIFY(!QFile::exists(temporary));
    }

    // optipng exits non-zero after writing part of its output: the original is untouched and nothing is left over.
    void failure_keepsOriginal()
    {
        QTemporaryDir dir;
        const QString file = dir.filePath("shot.png");
        write(file, "original");
        qputenv("OPTIPNG_STUB_MODE", "fail");
        installStub(true);

        const int finished = optimize(file);

        QCOMPARE(readAll(file), QByteArray("original"));
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files).join(","), QString("shot.png"));
        QCOMPARE(finished, 1);
    }

    // optipng exits 0 without writing an output: the original stays as it is.
    void noOutput_keepsOriginal()
    {
        QTemporaryDir dir;
        const QString file = dir.filePath("shot.png");
        write(file, "original");
        qputenv("OPTIPNG_STUB_MODE", "nooutput");
        installStub(true);

        const int finished = optimize(file);

        QCOMPARE(readAll(file), QByteArray("original"));
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files).join(","), QString("shot.png"));
        QCOMPARE(finished, 1);
    }

    // No optipng to be found: finished() fires once and the file is untouched.
    void missingBinary()
    {
        QTemporaryDir dir;
        const QString file = dir.filePath("shot.png");
        write(file, "original");
        installStub(false);

        const int finished = optimize(file);

        QCOMPARE(readAll(file), QByteArray("original"));
        QCOMPARE(finished, 1);
    }
};

QTEST_MAIN(tst_Optimize)
#include "tst_optimize.moc"
