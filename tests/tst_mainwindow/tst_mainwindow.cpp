// Stage 4 safety net, session 7: main window plumbing (docs/modernization-plan.md, section 6, item 7).
//
// Records what LightscreenWindow does with its command-line flags (executeArgument/executeArguments, the "default
// argument means --wake" branch) and how connectHotkeys numbers the global hotkeys (ids 1 to 6, upstream PR #37).
// These tests describe current behaviour, quirks included; a change that moves any of them must update this file
// on purpose.
//
// Every test constructs LightscreenWindow, which creates UGlobalHotkeys: that needs an X display on Linux (run under
// xvfb-run), and on Windows applySettings would write the real HKCU Run key through os::setStartup. So the whole
// test is skipped on Windows and when the platform is not xcb (see docs/decisions.md, Stage 4 settings entry).
#include <QtTest>
#include <algorithm>
#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>

#include <dialogs/areadialog.h>
#include <dialogs/historydialog.h>
#include <dialogs/optionsdialog.h>
#include <lightscreenwindow.h>
#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/uploader/uploader.h>
#include <tools/windowpicker.h>
#include <tools/UGlobalHotkey/uglobalhotkeys.h>

#ifdef Q_OS_LINUX
#include <QX11Info>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#endif

class tst_MainWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void wakeShowsWindow();
    void screenshotFlags_data();
    void screenshotFlags();
    void folderFlag();
    void uploadLastFlag();
    void viewHistoryFlag();
    void optionsFlag();
    void unknownFlagsDoNothing_data();
    void unknownFlagsDoNothing();
    void defaultArgumentMeansWake_data();
    void defaultArgumentMeansWake();
    void programPathIsSkippedAmongOtherArguments();
    void twoScreenshotFlagsTakeOneScreenshot();
    void hotkeyIdsFollowDocumentedOrder();
    void disabledHotkeysKeepTheirIds();
    void hotkeyIdsReachActions_data();
    void hotkeyIdsReachActions();
    void actionOutOfRange();
    void emptyHotkeyShowsError();

    // Last on purpose: it runs QApplication::exec() and quits it.
    void quitFlagQuitsTheApplication();

public slots:
    void openUrl(const QUrl &url);

private:
    void startWindow();
    bool waitForModes(int count, int timeoutMs = 5000);
    void dismissDialogs();

    QTemporaryDir mHome;
    QScopedPointer<QTemporaryDir> mDir;
    QScopedPointer<QSettings> mSettings;
    QScopedPointer<Uploader> mUploader;
    QScopedPointer<QNetworkAccessManager> mNetwork;
    QScopedPointer<ScreenshotManager> mManager;
    QScopedPointer<LightscreenWindow> mWindow;
    QTimer mDismisser;

    QList<int> mModes;          // Screenshot::Options::mode of every screenshot the manager finished
    QStringList mDialogs;       // class name of every dialog the dismisser closed, in order
    QList<QUrl> mOpenedUrls;    // every file:// URL handed to QDesktopServices::openUrl
    QString mMessageBoxText;    // text of the last QMessageBox, which the dismisser answers with "Disable"
};

// Compares two lists with std::equal: QList::operator== does not compile with Qt 5.15.2 on the current MSVC
// (Windows CI), see docs/status.md.
#define COMPARE_LIST(actual, expected)                                                                        \
    do {                                                                                                      \
        const auto a_ = (actual);                                                                             \
        const auto e_ = (expected);                                                                           \
        QVERIFY2(a_.size() == e_.size() && std::equal(a_.cbegin(), a_.cend(), e_.cbegin()),                 \
                 qPrintable(QString("got %1 items, expected %2").arg(a_.size()).arg(e_.size())));            \
    } while (false)

void tst_MainWindow::initTestCase()
{
#ifdef Q_OS_WIN
    QSKIP("Constructs LightscreenWindow, whose applySettings writes the real Windows startup registry key.");
#else
    if (QGuiApplication::platformName() != "xcb") {
        QSKIP("Constructs LightscreenWindow, which needs an X display (run under xvfb-run).");
    }
#endif

    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QVERIFY(mHome.isValid());

    // applySettings calls os::setStartup, which writes or removes ~/.config/autostart/lightscreen.desktop.
    qputenv("HOME", mHome.path().toLocal8Bit());

    // --folder and hotkey 6 open the screenshot folder; record the URL instead of starting a file manager.
    QDesktopServices::setUrlHandler("file", this, "openUrl");

    // Modal dialogs (area selector, history, options, hotkey error) and the window picker would wait for a user.
    mDismisser.setInterval(20);
    connect(&mDismisser, &QTimer::timeout, this, &tst_MainWindow::dismissDialogs);
}

void tst_MainWindow::cleanupTestCase()
{
    QDesktopServices::unsetUrlHandler("file");
}

void tst_MainWindow::init()
{
    mDir.reset(new QTemporaryDir);
    QVERIFY(mDir->isValid());

    // Uploader first: the ScreenshotManager constructor connects to Uploader::instance().
    mUploader.reset(new Uploader);
    mNetwork.reset(new QNetworkAccessManager);
    Uploader::setInstance(mUploader.data());
    mUploader->setNetwork(mNetwork.data());

    mSettings.reset(new QSettings(mDir->filePath("settings.ini"), QSettings::IniFormat));
    mManager.reset(new ScreenshotManager(mSettings.data(), mDir->path() + QDir::separator()));
    ScreenshotManager::setInstance(mManager.data());

    connect(mManager.data(), &ScreenshotManager::windowCleanup, this, [this](const Screenshot::Options &options) {
        mModes << options.mode;
    });

    // Enough for the window to start quietly (no first-run options dialog, no tray, no update check) and for a
    // screenshot to be taken and dropped: no file, no clipboard, no preview.
    mSettings->setValue("file/format", 1);
    mSettings->setValue("file/enabled", false);
    mSettings->setValue("file/target", mDir->filePath("shots"));
    mSettings->setValue("options/clipboard", false);
    mSettings->setValue("options/tray", false);
    mSettings->setValue("options/disableUpdater", true);

    mModes.clear();
    mDialogs.clear();
    mOpenedUrls.clear();
    mMessageBoxText.clear();
    mDismisser.start();
}

void tst_MainWindow::cleanup()
{
    mDismisser.stop();
    dismissDialogs();
    mWindow.reset();
    QTest::qWait(50); // Deferred deletes (screenshots, window picker).

    ScreenshotManager::setInstance(nullptr);
    Uploader::setInstance(nullptr);
    mManager.reset();
    mSettings.reset();
    mNetwork.reset();
    mUploader.reset();
    mDir.reset();
}

void tst_MainWindow::openUrl(const QUrl &url)
{
    mOpenedUrls << url;
}

// Constructs the window and lets its queued applySettings run (tray, hotkeys, last screenshot, startup).
void tst_MainWindow::startWindow()
{
    mWindow.reset(new LightscreenWindow);
    QTest::qWait(50);
}

// Each screenshotAction waits at least 400 ms before it reaches ScreenshotManager::take.
bool tst_MainWindow::waitForModes(int count, int timeoutMs)
{
    QDeadlineTimer deadline(timeoutMs);

    while (mModes.size() < count && !deadline.hasExpired()) {
        QTest::qWait(20);
    }

    return mModes.size() >= count;
}

void tst_MainWindow::dismissDialogs()
{
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (!widget->isVisible()) {
            continue;
        }

        if (auto box = qobject_cast<QMessageBox *>(widget)) {
            mDialogs << "QMessageBox";
            mMessageBoxText = box->text();

            for (QAbstractButton *button : box->buttons()) {
                if (button->text() == "Disable") {
                    button->click();
                }
            }
        } else if (qobject_cast<AreaDialog *>(widget) || qobject_cast<HistoryDialog *>(widget)
                   || qobject_cast<OptionsDialog *>(widget)) {
            mDialogs << widget->metaObject()->className();
            static_cast<QDialog *>(widget)->reject();
        } else if (qobject_cast<WindowPicker *>(widget)) {
            mDialogs << "WindowPicker";
            widget->close();
        }
    }
}

// --wake shows the (hidden) main window.
void tst_MainWindow::wakeShowsWindow()
{
    startWindow();
    QVERIFY(!mWindow->isVisible());

    mWindow->executeArgument("--wake");

    QVERIFY(mWindow->isVisible());
    QVERIFY(mModes.isEmpty());
}

void tst_MainWindow::screenshotFlags_data()
{
    QTest::addColumn<QString>("flag");
    QTest::addColumn<int>("mode");
    QTest::addColumn<QString>("dialog");

    QTest::newRow("--screen")       << "--screen"       << int(Screenshot::WholeScreen)    << QString();
    QTest::newRow("--area")         << "--area"         << int(Screenshot::SelectedArea)   << "AreaDialog";
    QTest::newRow("--activewindow") << "--activewindow" << int(Screenshot::ActiveWindow)   << QString();
    QTest::newRow("--pickwindow")   << "--pickwindow"   << int(Screenshot::SelectedWindow) << "WindowPicker";
}

// Each screenshot flag reaches screenshotAction with its mode: ScreenshotManager::take builds a Screenshot with it.
// (The area selector and window picker are dismissed, so those screenshots are cancelled.)
void tst_MainWindow::screenshotFlags()
{
    QFETCH(QString, flag);
    QFETCH(int, mode);
    QFETCH(QString, dialog);

    startWindow();
    mWindow->executeArgument(flag);

    QVERIFY(waitForModes(1));
    QTest::qWait(100);
    COMPARE_LIST(mModes, QList<int>() << mode);
    COMPARE_LIST(mDialogs, dialog.isEmpty() ? QStringList() : QStringList(dialog));
    QVERIFY(!mWindow->isVisible());
}

// --folder opens file/target (created first if missing) through QDesktopServices, with a trailing separator.
void tst_MainWindow::folderFlag()
{
    startWindow();
    const QString target = mDir->filePath("shots");
    QVERIFY(!QDir(target).exists());

    mWindow->executeArgument("--folder");

    QVERIFY(QDir(target).exists());
    QCOMPARE(mOpenedUrls.size(), 1);
    QCOMPARE(mOpenedUrls.at(0), QUrl::fromLocalFile(target + QDir::separator()));
    QVERIFY(mModes.isEmpty());
}

// --uploadlast uploads the "lastScreenshot" setting (read by applySettings) with the configured service. With the
// pomf service and no pomf URL the uploader fails before any network request, which shows the call was made.
// The tray icon is on because LightscreenWindow::showUploaderError crashes without one (docs/known-bugs.md).
void tst_MainWindow::uploadLastFlag()
{
    const QString last = mDir->filePath("last.jpg");
    mSettings->setValue("lastScreenshot", last);
    mSettings->setValue("upload/service", 1);
    mSettings->setValue("options/tray", true);

    startWindow();

    QSignalSpy done(mUploader.data(), &Uploader::done);
    QSignalSpy error(mUploader.data(), &Uploader::error);

    mWindow->executeArgument("--uploadlast");

    QCOMPARE(error.count(), 1);
    QCOMPARE(error.at(0).at(0).toString(), QStringLiteral("Invalid pomf uploader URL!"));
    QCOMPARE(done.count(), 1);
    QCOMPARE(done.at(0).at(0).toString(), last);
}

// --viewhistory opens the history dialog (modal).
void tst_MainWindow::viewHistoryFlag()
{
    startWindow();
    mWindow->executeArgument("--viewhistory");

    COMPARE_LIST(mDialogs, QStringList("HistoryDialog"));
    QVERIFY(mModes.isEmpty());
}

// --options opens the options dialog (modal); cancelling it still re-applies the settings.
void tst_MainWindow::optionsFlag()
{
    startWindow();
    mWindow->executeArgument("--options");

    COMPARE_LIST(mDialogs, QStringList("OptionsDialog"));
    QVERIFY(mModes.isEmpty());
}

void tst_MainWindow::unknownFlagsDoNothing_data()
{
    QTest::addColumn<QStringList>("arguments");

    // "-h" is what os::setStartup puts in the autostart entry when "start hidden" is on. No flag handles it: the
    // window stays hidden only because main() shows it when there are no arguments at all.
    QTest::newRow("-h")         << (QStringList() << "-h");
    QTest::newRow("program -h") << (QStringList() << qApp->arguments().at(0) << "-h");
    QTest::newRow("unknown")    << (QStringList() << "--nonsense");
    QTest::newRow("case")       << (QStringList() << "--SCREEN");
    QTest::newRow("no dashes")  << (QStringList() << "screen");
    QTest::newRow("empty list") << QStringList();
}

void tst_MainWindow::unknownFlagsDoNothing()
{
    QFETCH(QStringList, arguments);

    startWindow();
    QSignalSpy finished(mWindow.data(), &LightscreenWindow::finished);

    mWindow->executeArguments(arguments);
    QTest::qWait(600);

    QVERIFY(!mWindow->isVisible());
    QVERIFY(mModes.isEmpty());
    QVERIFY(mDialogs.isEmpty());
    QVERIFY(mOpenedUrls.isEmpty());
    QCOMPARE(finished.count(), 0);
}

void tst_MainWindow::defaultArgumentMeansWake_data()
{
    QTest::addColumn<QString>("argument");
    QTest::addColumn<bool>("wakes");

    const QString program = qApp->arguments().at(0);
    const QString fileName = QFileInfo(qApp->applicationFilePath()).fileName();

    // A second instance started without arguments forwards just its program path (SingleApplication).
    QTest::newRow("same path")      << program                                  << true;
    // The check is "contains the executable's file name", so any path to a same-named program wakes too...
    QTest::newRow("other path")     << "/somewhere/else/" + fileName            << true;
    // ...and so does any single argument that merely contains it.
    QTest::newRow("contains name")  << "--" + fileName + "-not-a-flag"          << true;
    QTest::newRow("bare name")      << fileName                                 << true;
    // A single flag is executed as a flag.
    QTest::newRow("single flag")    << "--folder"                               << false;
}

// executeArguments with one argument that is the program itself means --wake.
void tst_MainWindow::defaultArgumentMeansWake()
{
    QFETCH(QString, argument);
    QFETCH(bool, wakes);

    startWindow();
    QVERIFY(!mWindow->isVisible());

    mWindow->executeArguments(QStringList(argument));

    QCOMPARE(mWindow->isVisible(), wakes);
    QCOMPARE(mOpenedUrls.size(), wakes ? 0 : 1);
}

// With more than one argument (the normal command line: program path first) every argument is executed in turn;
// the program path matches no flag and is ignored, so the window is not woken.
void tst_MainWindow::programPathIsSkippedAmongOtherArguments()
{
    startWindow();

    mWindow->executeArguments(QStringList() << qApp->arguments().at(0) << "--folder" << "--viewhistory");

    QVERIFY(!mWindow->isVisible());
    QCOMPARE(mOpenedUrls.size(), 1);
    COMPARE_LIST(mDialogs, QStringList("HistoryDialog"));

    // The program path alone with another argument does not wake either.
    mWindow->executeArguments(QStringList() << qApp->arguments().at(0) << qApp->arguments().at(0));
    QVERIFY(!mWindow->isVisible());
}

// Current behaviour, recorded as a bug (docs/known-bugs.md): screenshotAction keeps a pending request in a single
// mLastMode field. A second request while the first waits out its 400 ms delay takes the *first* request's mode at
// once, the second mode is lost, and when the first timer fires with mLastMode reset to None it re-arms itself
// forever. With "hide while taking a screenshot" on, every re-arm hides the window again.
void tst_MainWindow::twoScreenshotFlagsTakeOneScreenshot()
{
    mSettings->setValue("options/hide", true);
    startWindow();
    mWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(mWindow.data()));

    mWindow->executeArguments(QStringList() << "--screen" << "--area");

    // The second flag took the first flag's mode synchronously; cleanup showed the window again.
    COMPARE_LIST(mModes, QList<int>() << int(Screenshot::WholeScreen));
    QVERIFY(mWindow->isVisible());

    // The first timer then fires with mode None: nothing is taken, the window is hidden, and it stays that way.
    QTest::qWait(1500);
    COMPARE_LIST(mModes, QList<int>() << int(Screenshot::WholeScreen));
    QVERIFY(mDialogs.isEmpty());
    QVERIFY(!mWindow->isVisible());

    // A later request still works (the loop hands it over), it just lands on the loop's schedule.
    mWindow->executeArgument("--screen");
    QVERIFY(waitForModes(2));
    COMPARE_LIST(mModes, QList<int>() << int(Screenshot::WholeScreen) << int(Screenshot::WholeScreen));
}

#ifdef Q_OS_LINUX
// Feeds UGlobalHotkeys an X key press for an unmodified key, as the X server would deliver a grabbed hotkey.
// Returns whether UGlobalHotkeys took it as one of its hotkeys.
static bool pressHotkey(UGlobalHotkeys *hotkeys, xcb_keysym_t keysym)
{
    xcb_key_symbols_t *symbols = xcb_key_symbols_alloc(QX11Info::connection());
    xcb_keycode_t *codes = xcb_key_symbols_get_keycode(symbols, keysym);
    xcb_keycode_t code = codes ? codes[0] : 0;
    free(codes);
    xcb_key_symbols_free(symbols);

    if (code == 0) {
        qWarning("No keycode for keysym %x", keysym);
        return false;
    }

    xcb_key_press_event_t event;
    memset(&event, 0, sizeof(event));
    event.response_type = XCB_KEY_PRESS;
    event.detail = code;
    event.state = 0;

    long result = 0;
    return static_cast<QAbstractNativeEventFilter *>(hotkeys)->nativeEventFilter("xcb_generic_event_t", &event, &result);
}
#endif

// connectHotkeys registers the six actions in this order with ids 1 to 6 (id 0 means "no hotkey" to UGlobalHotkeys,
// which is why the ids start at Screenshot::WholeScreen; upstream PR #37). Ids are what LightscreenWindow::action
// receives: 1-4 are Screenshot::Mode values, 5 is ShowMainWindow, 6 is OpenScreenshotFolder.
void tst_MainWindow::hotkeyIdsFollowDocumentedOrder()
{
#ifdef Q_OS_LINUX
    const QStringList actions = {"screen", "window", "area", "windowPicker", "open", "directory"};
    const QList<xcb_keysym_t> keys = {XK_F1, XK_F2, XK_F3, XK_F4, XK_F5, XK_F6};

    for (int i = 0; i < actions.size(); ++i) {
        mSettings->setValue("actions/" + actions[i] + "/enabled", true);
        mSettings->setValue("actions/" + actions[i] + "/hotkey", QString("F%1").arg(i + 1));
    }

    startWindow();

    UGlobalHotkeys *hotkeys = mWindow->findChild<UGlobalHotkeys *>();
    QVERIFY(hotkeys);

    // Listen to the ids alone: detach the window's own handler so no screenshot is started.
    QList<int> ids;
    disconnect(hotkeys, &UGlobalHotkeys::activated, nullptr, nullptr);
    connect(hotkeys, &UGlobalHotkeys::activated, this, [&ids](size_t id) { ids << int(id); });

    for (xcb_keysym_t key : keys) {
        QVERIFY(pressHotkey(hotkeys, key));
    }

    COMPARE_LIST(ids, QList<int>() << 1 << 2 << 3 << 4 << 5 << 6);
    QCOMPARE(int(Screenshot::WholeScreen), 1);
    QCOMPARE(int(Screenshot::ActiveWindow), 2);
    QCOMPARE(int(Screenshot::SelectedArea), 3);
    QCOMPARE(int(Screenshot::SelectedWindow), 4);
    QCOMPARE(int(LightscreenWindow::ShowMainWindow), 5);
    QCOMPARE(int(LightscreenWindow::OpenScreenshotFolder), 6);

    // A key that is not registered is passed on.
    QVERIFY(!pressHotkey(hotkeys, XK_F7));
#else
    QSKIP("X11 only.");
#endif
}

// A disabled action is skipped but still uses up its id, so the others keep theirs.
void tst_MainWindow::disabledHotkeysKeepTheirIds()
{
#ifdef Q_OS_LINUX
    mSettings->setValue("actions/screen/enabled", false);
    mSettings->setValue("actions/screen/hotkey", "F1");
    mSettings->setValue("actions/area/enabled", true);
    mSettings->setValue("actions/area/hotkey", "F3");
    mSettings->setValue("actions/directory/enabled", true);
    mSettings->setValue("actions/directory/hotkey", "F6");

    startWindow();

    UGlobalHotkeys *hotkeys = mWindow->findChild<UGlobalHotkeys *>();
    QVERIFY(hotkeys);

    QList<int> ids;
    disconnect(hotkeys, &UGlobalHotkeys::activated, nullptr, nullptr);
    connect(hotkeys, &UGlobalHotkeys::activated, this, [&ids](size_t id) { ids << int(id); });

    QVERIFY(!pressHotkey(hotkeys, XK_F1));
    QVERIFY(pressHotkey(hotkeys, XK_F3));
    QVERIFY(pressHotkey(hotkeys, XK_F6));

    COMPARE_LIST(ids, QList<int>() << 3 << 6);
#else
    QSKIP("X11 only.");
#endif
}

void tst_MainWindow::hotkeyIdsReachActions_data()
{
    QTest::addColumn<int>("id");
    QTest::addColumn<int>("mode");      // expected screenshot mode, or -1 for none
    QTest::addColumn<bool>("shows");    // window shown
    QTest::addColumn<bool>("folder");   // screenshot folder opened

    QTest::newRow("1 screen")       << 1 << int(Screenshot::WholeScreen)    << false << false;
    QTest::newRow("2 window")       << 2 << int(Screenshot::ActiveWindow)   << false << false;
    QTest::newRow("3 area")         << 3 << int(Screenshot::SelectedArea)   << false << false;
    QTest::newRow("4 windowPicker") << 4 << int(Screenshot::SelectedWindow) << false << false;
    QTest::newRow("5 open")         << 5 << -1                              << true  << false;
    QTest::newRow("6 directory")    << 6 << -1                              << false << true;
}

// UGlobalHotkeys::activated(id) reaches LightscreenWindow::action(id).
void tst_MainWindow::hotkeyIdsReachActions()
{
    QFETCH(int, id);
    QFETCH(int, mode);
    QFETCH(bool, shows);
    QFETCH(bool, folder);

    startWindow();

    UGlobalHotkeys *hotkeys = mWindow->findChild<UGlobalHotkeys *>();
    QVERIFY(hotkeys);

    emit hotkeys->activated(size_t(id));

    if (mode >= 0) {
        QVERIFY(waitForModes(1));
        COMPARE_LIST(mModes, QList<int>() << mode);
    } else {
        QTest::qWait(600);
        QVERIFY(mModes.isEmpty());
    }

    QCOMPARE(mWindow->isVisible(), shows);
    QCOMPARE(mOpenedUrls.size(), folder ? 1 : 0);
}

// action() outside 1-6: 7 and above only warn; 0 (Screenshot::None) passes the "mode <= SelectedWindow" check and
// starts screenshotAction(None), which never takes anything (its delay timer re-arms itself, see
// twoScreenshotFlagsTakeOneScreenshot).
void tst_MainWindow::actionOutOfRange()
{
    startWindow();

    QTest::ignoreMessage(QtWarningMsg, "Unknown hotkey ID:  7");
    mWindow->action(7);

    mWindow->action(0);
    QTest::qWait(1000);

    QVERIFY(mModes.isEmpty());
    QVERIFY(!mWindow->isVisible());
    QVERIFY(mOpenedUrls.isEmpty());
}

// An enabled action whose hotkey cannot be registered (here: empty) makes connectHotkeys show a message box naming
// the action's settings name (not the key). Its "Disable" button turns the action off in the settings.
void tst_MainWindow::emptyHotkeyShowsError()
{
    mSettings->setValue("actions/area/enabled", true);
    mSettings->setValue("actions/area/hotkey", "");

    startWindow();
    QTRY_VERIFY(!mDialogs.isEmpty());

    COMPARE_LIST(mDialogs, QStringList("QMessageBox"));
    QVERIFY2(mMessageBoxText.contains("The failed hotkey is <b>area</b>"), qPrintable(mMessageBoxText));
    QCOMPARE(mSettings->value("actions/area/enabled").toBool(), false);
}

// --quit calls QApplication::quit() directly. Unlike LightscreenWindow::quit (the tray menu's Quit), it does not
// ask when screenshots are processing or uploading, does not save the window position, and does not emit finished.
void tst_MainWindow::quitFlagQuitsTheApplication()
{
    startWindow();
    QSignalSpy finished(mWindow.data(), &LightscreenWindow::finished);
    mSettings->remove("position");

    QTimer::singleShot(0, mWindow.data(), [this] { mWindow->executeArgument("--quit"); });
    QTimer::singleShot(3000, qApp, [] { QCoreApplication::exit(1); });

    QCOMPARE(qApp->exec(), 0);
    QCOMPARE(finished.count(), 0);
    QVERIFY(!mSettings->contains("position"));
}

QTEST_MAIN(tst_MainWindow)
#include "tst_mainwindow.moc"
