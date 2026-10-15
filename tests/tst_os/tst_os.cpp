// Safety-net tests for the os helpers (tools/os): they record current behaviour, not intended behaviour.
// Assertions marked "expected to change" document a known bug and flip when it is fixed.
#include <QtTest>
#include <QApplication>
#include <QIcon>
#include <QPalette>
#include <QSettings>
#include <QTemporaryDir>

#include <tools/os.h>

class tst_Os : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
    }

    // getDocumentsPath is never empty. On Linux it is $HOME/Documents, whether or not that directory exists.
    void documentsPathNotEmpty()
    {
        const QString path = os::getDocumentsPath();
        QVERIFY(!path.isEmpty());
        QVERIFY(QDir::isAbsolutePath(path));
#ifndef Q_OS_WIN
        QCOMPARE(path, QDir::homePath() + QDir::separator() + "Documents");
#endif
    }

#ifndef Q_OS_WIN
    void documentsPathFollowsHome()
    {
        QTemporaryDir home;
        QVERIFY(home.isValid());
        const QByteArray oldHome = qgetenv("HOME");
        qputenv("HOME", QFile::encodeName(home.path()));
        const QString path = os::getDocumentsPath();
        qputenv("HOME", oldHome);

        QCOMPARE(path, home.path() + "/Documents");
        QVERIFY(!QDir(path).exists()); // not created
    }
#endif

    // icon() picks the normal set when the background's HSV value is above 125, the inverted set otherwise.
    void iconByBackground_data()
    {
        QTest::addColumn<QColor>("background");
        QTest::addColumn<QString>("expectedPrefix");

        QTest::newRow("white") << QColor(Qt::white) << ":/icons/";
        QTest::newRow("value 126") << QColor(126, 0, 0) << ":/icons/";
        QTest::newRow("value 125") << QColor(125, 125, 125) << ":/icons/inv/";
        QTest::newRow("black") << QColor(Qt::black) << ":/icons/inv/";
        QTest::newRow("saturated blue (value 255)") << QColor(0, 0, 255) << ":/icons/";
    }

    void iconByBackground()
    {
        QFETCH(QColor, background);
        QFETCH(QString, expectedPrefix);

        const QString otherPrefix = expectedPrefix == ":/icons/" ? ":/icons/inv/" : ":/icons/";
        const QIcon icon = os::icon("yes", background);
        QVERIFY(iconIs(icon, expectedPrefix + "yes"));
        QVERIFY(!iconIs(icon, otherPrefix + "yes"));
    }

    // Without a colour, icon() uses the application palette's Button colour.
    void iconFollowsPalette()
    {
        const QPalette oldPalette = qApp->palette();

        QPalette light = oldPalette;
        light.setColor(QPalette::Button, QColor(200, 200, 200));
        qApp->setPalette(light);
        QVERIFY(iconIs(os::icon("configure"), ":/icons/configure"));

        QPalette dark = oldPalette;
        dark.setColor(QPalette::Button, QColor(40, 40, 40));
        qApp->setPalette(dark);
        QVERIFY(iconIs(os::icon("configure"), ":/icons/inv/configure"));

        // Only Button counts: a dark Window with a light Button still gives the normal set.
        QPalette mixed = oldPalette;
        mixed.setColor(QPalette::Window, Qt::black);
        mixed.setColor(QPalette::Button, Qt::white);
        qApp->setPalette(mixed);
        QVERIFY(iconIs(os::icon("configure"), ":/icons/configure"));

        qApp->setPalette(oldPalette);
    }

    // The four lightscreen.* icons have no inverted variant: on a dark background icon() returns an icon with no
    // pixmap. None of them is requested through icon() today.
    void iconWithoutInvertedVariant()
    {
        QVERIFY(!QImage(":/icons/lightscreen.small").isNull());
        QVERIFY(QImage(":/icons/inv/lightscreen.small").isNull());

        QVERIFY(!os::icon("lightscreen.small", Qt::white).pixmap(16, 16).isNull());
        QVERIFY(os::icon("lightscreen.small", Qt::black).pixmap(16, 16).isNull());
    }

    // An unknown name gives an icon with no pixmap rather than a warning or a fallback.
    void iconUnknownName()
    {
        QVERIFY(os::icon("no-such-icon", Qt::white).pixmap(16, 16).isNull());
        QVERIFY(os::icon("no-such-icon", Qt::black).pixmap(16, 16).isNull());
    }

#if defined(Q_OS_LINUX)
    // setStartup writes $HOME/.config/autostart/lightscreen.desktop. HOME points at a temporary directory here.
    void startupWritesDesktopFile()
    {
        QVERIFY(QDir(mHome->path()).mkpath(".config/autostart"));

        os::setStartup(true, false);

        QCOMPARE(readDesktopFile(), expectedDesktopFile(false));
    }

    void startupHideAddsFlag()
    {
        QVERIFY(QDir(mHome->path()).mkpath(".config/autostart"));

        os::setStartup(true, true);

        QCOMPARE(readDesktopFile(), expectedDesktopFile(true));
    }

    // An existing file (ours or the user's) is replaced, not appended to.
    void startupReplacesExistingFile()
    {
        QVERIFY(QDir(mHome->path()).mkpath(".config/autostart"));
        QFile old(desktopFilePath());
        QVERIFY(old.open(QIODevice::WriteOnly));
        old.write("[Desktop Entry]\nExec=something-else\nName=Old\nType=Application\n");
        old.close();

        os::setStartup(true, false);

        QCOMPARE(readDesktopFile(), expectedDesktopFile(false));
    }

    void startupFalseRemovesDesktopFile()
    {
        QVERIFY(QDir(mHome->path()).mkpath(".config/autostart"));
        os::setStartup(true, false);
        QVERIFY(QFile::exists(desktopFilePath()));

        os::setStartup(false, false);
        QVERIFY(!QFile::exists(desktopFilePath()));
        QVERIFY(QDir(mHome->path() + "/.config/autostart").exists()); // the directory stays

        // Turning it off again with no file is harmless.
        os::setStartup(false, true);
        QVERIFY(!QFile::exists(desktopFilePath()));
    }

    // With no ~/.config/autostart directory, setStartup(true) creates it and writes the file (bug 25).
    void startupWithoutAutostartDir()
    {
        QVERIFY(!QDir(mHome->path() + "/.config").exists());

        os::setStartup(true, false);

        QVERIFY(QDir(mHome->path() + "/.config/autostart").exists());
        QCOMPARE(readDesktopFile(), expectedDesktopFile(false));
    }

    // When the directory cannot be created (here a plain file is in the way), setStartup warns instead of
    // failing silently.
    void startupUnwritableWarns()
    {
        QVERIFY(QDir(mHome->path()).mkpath(".config"));
        QFile blocker(mHome->path() + "/.config/autostart");
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Could not write .*lightscreen\\.desktop"));
        os::setStartup(true, false);

        QVERIFY(!QFile::exists(desktopFilePath()));
    }

    // The Exec value quotes the program per the Desktop Entry spec: spaces survive, and ", `, $ and \ are
    // escaped inside the quotes (\ twice, once more for the string-value escape). % is doubled.
    void desktopExec_data()
    {
        QTest::addColumn<QString>("program");
        QTest::addColumn<bool>("hide");
        QTest::addColumn<QString>("exec");

        QTest::newRow("plain") << "/usr/bin/lightscreen" << false << "\"/usr/bin/lightscreen\"";
        QTest::newRow("hide") << "/usr/bin/lightscreen" << true << "\"/usr/bin/lightscreen\" -h";
        QTest::newRow("space") << "/home/a b/lightscreen" << false << "\"/home/a b/lightscreen\"";
        QTest::newRow("reserved") << "/x/\"`$/ls" << false << "\"/x/\\\\\"\\\\`\\\\$/ls\"";
        QTest::newRow("backslash") << "/x\\y/ls" << false << "\"/x\\\\\\\\y/ls\"";
        QTest::newRow("percent") << "/x/100%/ls" << false << "\"/x/100%%/ls\"";
    }

    void desktopExec()
    {
        QFETCH(QString, program);
        QFETCH(bool, hide);
        QFETCH(QString, exec);

        QCOMPARE(os::desktopExec(program, hide), exec);
    }

    void init()
    {
        mHome = new QTemporaryDir;
        QVERIFY(mHome->isValid());
        mOldHome = qgetenv("HOME");
        qputenv("HOME", QFile::encodeName(mHome->path()));
        QCOMPARE(QDir::homePath(), mHome->path());
    }

    void cleanup()
    {
        if (mHome) {
            qputenv("HOME", mOldHome);
            delete mHome;
            mHome = nullptr;
        }
    }
#endif

private:
    // True when the icon's pixmap is the image stored under resourcePath. Compared premultiplied, because the
    // gray+alpha PNGs come back from QIcon with different colour values in fully transparent pixels.
    static bool iconIs(const QIcon &icon, const QString &resourcePath)
    {
        const QImage expected = QImage(resourcePath).convertToFormat(QImage::Format_ARGB32_Premultiplied);
        if (expected.isNull()) {
            qWarning("No resource %s", qPrintable(resourcePath));
            return false;
        }

        const QImage actual =
            icon.pixmap(expected.size()).toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        return actual == expected;
    }

#if defined(Q_OS_LINUX)
    QString desktopFilePath() const
    {
        return mHome->path() + "/.config/autostart/lightscreen.desktop";
    }

    QString readDesktopFile() const
    {
        QFile file(desktopFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            return QString();
        }
        return QString::fromUtf8(file.readAll());
    }

    static QString expectedDesktopFile(bool hide)
    {
        return "[Desktop Entry]\nType=Application\nName=Lightscreen\nExec=" + os::desktopExec(appPath(), hide) + "\n";
    }

    static QString appPath()
    {
        return QDir::toNativeSeparators(qApp->applicationFilePath());
    }

    QTemporaryDir *mHome = nullptr;
    QByteArray mOldHome;
#endif
};

QTEST_MAIN(tst_Os)
#include "tst_os.moc"
