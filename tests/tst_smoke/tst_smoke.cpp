// Placeholder test: proves the harness builds the app's sources and that the M02 seams can be installed.
#include <QtTest>
#include <QSettings>
#include <QTemporaryDir>

#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/uploader/uploader.h>

class tst_Smoke : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
    }

    void extension()
    {
        QCOMPARE(Screenshot::extensionFor(Screenshot::PNG), QStringLiteral(".png"));
    }

    void seams()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QSettings settings(dir.filePath("settings.ini"), QSettings::IniFormat);
        ScreenshotManager manager(&settings, dir.path() + QDir::separator());
        ScreenshotManager::setInstance(&manager);
        QCOMPARE(ScreenshotManager::instance(), &manager);
        QCOMPARE(ScreenshotManager::instance()->settings(), &settings);
        QVERIFY(!manager.portableMode());

        // S02: the manager owns the uploader.
        QNetworkAccessManager network;
        QVERIFY(manager.uploader());
        QCOMPARE(manager.uploader()->parent(), static_cast<QObject *>(&manager));
        manager.uploader()->setNetwork(&network);
        QCOMPARE(manager.uploader()->network(), &network);

        ScreenshotManager::setInstance(nullptr);
    }
};

QTEST_MAIN(tst_Smoke)
#include "tst_smoke.moc"
