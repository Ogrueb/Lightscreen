// Stage 4 safety net, session 1: the settings contract (docs/modernization-plan.md, section 6, item 1).
//
// Records what OptionsDialog writes on empty settings (defaults.golden), what survives a save/reload round trip
// with every widget changed, and what LightscreenWindow::screenshotAction reads from the same settings. These tests
// describe current behaviour, quirks included; a change that moves any of them must update this file on purpose.
#include <QtTest>
#include <algorithm>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QTemporaryDir>

#include <dialogs/optionsdialog.h>
#include <lightscreenwindow.h>
#include <tools/os.h>
#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/uploader/uploader.h>
#include <widgets/hotkeywidget.h>

class tst_Settings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void dialogDefaults();
    void dialogRoundTrip();
    void dialogOutOfRangeIndexes();
    void dialogLegacyImgurSettings();
    void windowReadsEmptySettings();
    void windowReadsDialogDefaults();
    void windowCachesOptions();
    void windowPngQualityIsForced();

private:
    QStringList dump() const;
    QString placeholders(QString value) const;
    QStringList golden(const QString &fileName) const;
    QStringList describe(const Screenshot::Options &options) const;
    bool probeWindow(LightscreenWindow &window, Screenshot::Options *options);
    void setChecked(OptionsDialog &dialog, const char *name, bool checked);

    QTemporaryDir mHome;
    QScopedPointer<QTemporaryDir> mDir;
    QScopedPointer<QSettings> mSettings;
    QScopedPointer<Uploader> mUploader;
    QScopedPointer<QNetworkAccessManager> mNetwork;
    QScopedPointer<ScreenshotManager> mManager;
};

// Compares two key=value lists and fails with every difference listed, not just the first. Uses std::equal, not
// QStringList::operator==: with Qt 5.15.2 and current MSVC (CI's windows-latest), QList::operator== does not compile
// (it calls stdext::make_checked_array_iterator, which the newer MSVC standard library removed).
#define COMPARE_LINES(actual, expected)                                                                     \
    do {                                                                                                    \
        const QStringList a_ = (actual), e_ = (expected);                                                   \
        if (a_.size() != e_.size() || !std::equal(a_.cbegin(), a_.cend(), e_.cbegin())) {                \
            QStringList diff_;                                                                              \
            for (const QString &line : e_) if (!a_.contains(line)) diff_ << "- " + line;                    \
            for (const QString &line : a_) if (!e_.contains(line)) diff_ << "+ " + line;                    \
            QFAIL(qPrintable("Settings differ (- expected, + actual):\n" + diff_.join('\n')));              \
        }                                                                                                   \
    } while (false)

void tst_Settings::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QVERIFY(mHome.isValid());

#ifdef Q_OS_LINUX
    // LightscreenWindow::applySettings calls os::setStartup, which removes ~/.config/autostart/lightscreen.desktop,
    // and the dialog's default target is under ~/Documents. Point HOME at a temporary directory for both.
    qputenv("HOME", mHome.path().toLocal8Bit());
#endif
}

void tst_Settings::init()
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
}

void tst_Settings::cleanup()
{
    ScreenshotManager::setInstance(nullptr);
    Uploader::setInstance(nullptr);
    mManager.reset();
    mSettings.reset();
    mNetwork.reset();
    mUploader.reset();
    mDir.reset();
}

// The settings file as written to disk, one "key=value" per line, sorted, with machine-specific paths replaced.
QStringList tst_Settings::dump() const
{
    mSettings->sync();

    QSettings file(mSettings->fileName(), QSettings::IniFormat);
    QStringList lines;

    for (const QString &key : file.allKeys()) {
        lines << key + "=" + placeholders(file.value(key).toString());
    }

    lines.sort();
    return lines;
}

QString tst_Settings::placeholders(QString value) const
{
    // Longest first: the temporary directory may live under HOME.
    value.replace(QDir::toNativeSeparators(mDir->path()), "<tmp>");
    value.replace(mDir->path(), "<tmp>");
    value.replace(os::getDocumentsPath(), "<documents>");
    value.replace(QDir::toNativeSeparators(QDir::currentPath()), "<cwd>");
    value.replace(QDir::currentPath(), "<cwd>");

    if (value.startsWith('<')) {
        value.replace('\\', '/');
    }

    return value;
}

// Reads a golden list next to this file. Lines starting with "linux:" or "windows:" apply only on that platform
// and replace the common line for the same key. Blank lines and lines starting with "#" are ignored.
QStringList tst_Settings::golden(const QString &fileName) const
{
    const QString path = QFINDTESTDATA(fileName);
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open golden file" << fileName;
        return {};
    }

#ifdef Q_OS_WIN
    const QString platform = "windows:";
#else
    const QString platform = "linux:";
#endif

    QMap<QString, QString> common, specific;

    for (QString line : QString::fromUtf8(file.readAll()).split('\n')) {
        line = line.trimmed();

        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        QMap<QString, QString> *target = &common;

        if (line.startsWith("linux:") || line.startsWith("windows:")) {
            if (!line.startsWith(platform)) {
                continue;
            }

            line.remove(0, platform.size());
            target = &specific;
        }

        target->insert(line.section('=', 0, 0), line);
    }

    for (auto it = specific.cbegin(); it != specific.cend(); ++it) {
        common.insert(it.key(), it.value());
    }

    QStringList lines = common.values();
    lines.sort();
    return lines;
}

QStringList tst_Settings::describe(const Screenshot::Options &o) const
{
    auto b = [](bool value) { return value ? QStringLiteral("true") : QStringLiteral("false"); };

    return {
        QString("animations=%1").arg(b(o.animations)),
        QString("clipboard=%1").arg(b(o.clipboard)),
        QString("currentMonitor=%1").arg(b(o.currentMonitor)),
        QString("cursor=%1").arg(b(o.cursor)),
        QString("directory=%1").arg(placeholders(o.directory.path())),
        QString("file=%1").arg(b(o.file)),
        QString("format=%1").arg(o.format),
        QString("magnify=%1").arg(b(o.magnify)),
        QString("naming.dateFormat=%1").arg(o.namingOptions.dateFormat),
        QString("naming.flip=%1").arg(b(o.namingOptions.flip)),
        QString("naming.leadingZeros=%1").arg(o.namingOptions.leadingZeros),
        QString("naming.naming=%1").arg(o.namingOptions.naming),
        QString("optimize=%1").arg(b(o.optimize)),
        QString("prefix=%1").arg(o.prefix),
        QString("preview=%1").arg(b(o.preview)),
        QString("quality=%1").arg(o.quality),
        QString("replace=%1").arg(b(o.replace)),
        QString("saveAs=%1").arg(b(o.saveAs)),
        QString("upload=%1").arg(b(o.upload)),
        QString("uploadService=%1").arg(o.uploadService),
        QString("urlClipboard=%1").arg(b(o.urlClipboard)),
    };
}

// Runs LightscreenWindow::screenshotAction and captures the Screenshot::Options it hands to ScreenshotManager.
// Mode 5 is not an enumerator, so Screenshot::take() captures nothing and cancels straight away; that path still
// emits ScreenshotManager::windowCleanup with the screenshot's options. Note those are the options after the
// Screenshot constructor, which forces quality to 80 for PNG.
bool tst_Settings::probeWindow(LightscreenWindow &window, Screenshot::Options *options)
{
    bool captured = false;
    auto connection = connect(mManager.data(), &ScreenshotManager::windowCleanup, this,
                              [&](const Screenshot::Options &o) { *options = o; captured = true; });

    window.screenshotAction(static_cast<Screenshot::Mode>(5));

    // screenshotAction always delays by at least 400 ms before calling ScreenshotManager::take.
    QDeadlineTimer deadline(5000);
    while (!captured && !deadline.hasExpired()) {
        QTest::qWait(20);
    }

    disconnect(connection);
    return captured;
}

void tst_Settings::setChecked(OptionsDialog &dialog, const char *name, bool checked)
{
    QAbstractButton *button = dialog.findChild<QAbstractButton *>(name);

    if (button) {
        button->setChecked(checked);
    } else {
        QGroupBox *group = dialog.findChild<QGroupBox *>(name);
        QVERIFY2(group, name);
        group->setChecked(checked);
    }
}

// OptionsDialog on empty settings, saved without changes: every key it writes, with its default.
void tst_Settings::dialogDefaults()
{
    OptionsDialog dialog;
    QTRY_VERIFY(dialog.isEnabled()); // init() and loadSettings() are queued from the constructor.

    dialog.saveSettings();

    if (qEnvironmentVariableIsSet("LIGHTSCREEN_PRINT_GOLDEN")) {
        qInfo().noquote() << "\n" + dump().join('\n');
    }

    COMPARE_LINES(dump(), golden("defaults.golden"));
}

// Every widget set to a non-default value, saved, loaded into a second dialog, saved again.
void tst_Settings::dialogRoundTrip()
{
    const QString target = mDir->filePath("shots");

    {
        OptionsDialog dialog;
        QTRY_VERIFY(dialog.isEnabled());

        dialog.findChild<QComboBox *>("formatComboBox")->setCurrentIndex(2);
        dialog.findChild<QLineEdit *>("prefixLineEdit")->setText("shot-");
        dialog.findChild<QComboBox *>("namingComboBox")->setCurrentIndex(2);
        dialog.findChild<QLineEdit *>("targetLineEdit")->setText(target);
        setChecked(dialog, "fileGroupBox", false);

        setChecked(dialog, "startupCheckBox", true);
        setChecked(dialog, "startupHideCheckBox", false);
        setChecked(dialog, "hideCheckBox", false);
        dialog.findChild<QSpinBox *>("delaySpinBox")->setValue(7);
        setChecked(dialog, "trayCheckBox", false);
        setChecked(dialog, "messageCheckBox", false);
        dialog.findChild<QSlider *>("qualitySlider")->setValue(42);
        setChecked(dialog, "playSoundCheckBox", true);
        setChecked(dialog, "updaterCheckBox", false);
        setChecked(dialog, "magnifyCheckBox", true);
#ifdef Q_OS_WIN
        setChecked(dialog, "cursorCheckBox", false); // Default true on Windows (above XP), false on Linux.
#else
        setChecked(dialog, "cursorCheckBox", true);
#endif
        setChecked(dialog, "saveAsCheckBox", true);
        setChecked(dialog, "previewGroupBox", true);
        dialog.findChild<QSpinBox *>("previewSizeSpinBox")->setValue(500);
        dialog.findChild<QComboBox *>("previewPositionComboBox")->setCurrentIndex(0);
        setChecked(dialog, "previewAutocloseCheckBox", true);
        dialog.findChild<QSpinBox *>("previewAutocloseTimeSpinBox")->setValue(30);
        dialog.findChild<QComboBox *>("previewAutocloseActionComboBox")->setCurrentIndex(2);
        dialog.findChild<QComboBox *>("previewDefaultActionComboBox")->setCurrentIndex(1);
        setChecked(dialog, "areaAutocloseCheckBox", true);
        setChecked(dialog, "historyCheckBox", false);

        setChecked(dialog, "closeHideCheckBox", false);
        setChecked(dialog, "clipboardCheckBox", false);
        setChecked(dialog, "urlClipboardCheckBox", true);
        setChecked(dialog, "optiPngCheckBox", true);
        setChecked(dialog, "currentMonitorCheckBox", true);
        setChecked(dialog, "replaceCheckBox", true);
        setChecked(dialog, "uploadCheckBox", true);

        const QList<QPair<const char *, const char *>> actions = {
            {"screen", "Shift+Print"}, {"area", "Ctrl+Shift+A"}, {"window", "Ctrl+Shift+W"},
            {"windowPicker", "Ctrl+Shift+P"}, {"open", "Ctrl+Shift+O"}, {"directory", "Ctrl+Shift+D"},
        };

        for (const auto &action : actions) {
            const QString name = action.first;
            QCheckBox *enabled = dialog.findChild<QCheckBox *>(name + "CheckBox");
            QVERIFY(enabled);
            enabled->setChecked(!enabled->isChecked());
            dialog.findChild<HotkeyWidget *>(name + "HotkeyWidget")->setHotkey(action.second);
        }

        dialog.findChild<QComboBox *>("uploadServiceComboBox")->setCurrentIndex(0); // pomf, the only entry since M12.
        dialog.findChild<QComboBox *>("pomfUrlComboBox")->setCurrentText("https://pomf.example.invalid/upload.php");

        dialog.saveSettings();
    }

    const QStringList saved = dump();

    if (qEnvironmentVariableIsSet("LIGHTSCREEN_PRINT_GOLDEN")) {
        qInfo().noquote() << "\n" + saved.join('\n');
    }

    COMPARE_LINES(saved, golden("roundtrip.golden"));

    {
        OptionsDialog dialog;
        QTRY_VERIFY(dialog.isEnabled());
        dialog.saveSettings();
    }

    // What the second save changes. On Linux, loadSettings forces the sound cue and cursor options off (both are
    // hidden there), and turns optimization off when "optipng" cannot be started (not on PATH). Everything else survives.
    QStringList expected = saved;

#ifdef Q_OS_LINUX
    expected.replaceInStrings(QRegularExpression("^options/cursor=.*"), "options/cursor=false");
    expected.replaceInStrings(QRegularExpression("^options/playSound=.*"), "options/playSound=false");

    if (QStandardPaths::findExecutable("optipng").isEmpty()) {
        expected.replaceInStrings(QRegularExpression("^options/optimize=.*"), "options/optimize=false");
    }
#endif

    COMPARE_LINES(dump(), expected);
}

// Indexes the dialog's combo boxes cannot show (known-bugs.md, bug 19). The format combo box lists WEBP (3), so a
// WEBP setting is shown and saved as is; a naming or preview position index the boxes do not list is kept instead of
// being saved as -1. An "upload/service" the box does not list reads as 0 (M12).
void tst_Settings::dialogOutOfRangeIndexes()
{
    mSettings->setValue("file/format", 3);
    mSettings->setValue("file/naming", 9);
    mSettings->setValue("options/previewPosition", 9);
    mSettings->setValue("upload/service", 9);

    {
        OptionsDialog dialog;
        QTRY_VERIFY(dialog.isEnabled());
        QCOMPARE(dialog.findChild<QComboBox *>("formatComboBox")->currentText(), QStringLiteral("WEBP"));
        dialog.saveSettings();
    }

    QCOMPARE(mSettings->value("file/format").toString(), QStringLiteral("3"));
    QCOMPARE(mSettings->value("file/naming").toString(), QStringLiteral("9"));
    QCOMPARE(mSettings->value("options/previewPosition").toString(), QStringLiteral("9"));
    QCOMPARE(mSettings->value("upload/service").toString(), QStringLiteral("0"));
}

// Settings written before M12 still load. "upload/service" 1 (pomf when Imgur was 0) is beyond the one-entry list and
// reads as 0, which is pomf now; the old upload/imgur/* keys are left as they were.
void tst_Settings::dialogLegacyImgurSettings()
{
    mSettings->setValue("upload/service", 1);
    mSettings->setValue("upload/pomf/pomf_url", "https://pomf.example.invalid");
    mSettings->setValue("upload/imgur/access_token", "token");
    mSettings->setValue("upload/imgur/account_username", "someone");
    mSettings->setValue("upload/imgur/album", "album");

    {
        OptionsDialog dialog;
        QTRY_VERIFY(dialog.isEnabled());
        QComboBox *service = dialog.findChild<QComboBox *>("uploadServiceComboBox");
        QCOMPARE(service->count(), 1);
        QCOMPARE(service->currentIndex(), 0);
        dialog.saveSettings();
    }

    QCOMPARE(mSettings->value("upload/service").toString(), QStringLiteral("0"));
    QCOMPARE(mSettings->value("upload/pomf/pomf_url").toString(), QStringLiteral("https://pomf.example.invalid"));
    QCOMPARE(mSettings->value("upload/imgur/access_token").toString(), QStringLiteral("token"));
    QCOMPARE(mSettings->value("upload/imgur/account_username").toString(), QStringLiteral("someone"));
    QCOMPARE(mSettings->value("upload/imgur/album").toString(), QStringLiteral("album"));
    QCOMPARE(Uploader::serviceName(mSettings->value("upload/service").toInt()), QStringLiteral("pomf"));
}

// A macro, not a function: QSKIP returns from the function it is used in.
#ifdef Q_OS_WIN
// applySettings would write the real HKCU Run key through os::setStartup.
#define REQUIRE_WINDOW_PLATFORM() \
    QSKIP("Constructs LightscreenWindow, whose applySettings writes the real Windows startup registry key.")
#else
#define REQUIRE_WINDOW_PLATFORM()                                                                   \
    if (QGuiApplication::platformName() != "xcb") {                                                 \
        QSKIP("Constructs LightscreenWindow, which needs an X display (run under xvfb-run).");      \
    }
#endif

// What screenshotAction reads when the settings hold only what the window needs to start quietly. "file/format"
// must exist or the constructor opens the options dialog instead, so the window's own format default (0 by
// toInt()) is never reached. Its other defaults differ from the dialog's: file saving off, empty prefix,
// a relative (current directory) target, and the cursor on.
void tst_Settings::windowReadsEmptySettings()
{
    REQUIRE_WINDOW_PLATFORM();

    mSettings->setValue("file/format", 1);
    mSettings->setValue("options/tray", false);
    mSettings->setValue("options/disableUpdater", true);

    LightscreenWindow window;
    Screenshot::Options options;
    QVERIFY(probeWindow(window, &options));

    const QStringList expected = {
        "animations=true",
        "clipboard=true",
        "currentMonitor=false",
        "cursor=true",
        "directory=<cwd>",
        "file=false",
        "format=1",
        "magnify=false",
        "naming.dateFormat=yyyy-MM-dd",
        "naming.flip=false",
        "naming.leadingZeros=0",
        "naming.naming=0",
        "optimize=false",
        "prefix=",
        "preview=false",
        "quality=100",
        "replace=false",
        "saveAs=false",
        "upload=false",
        "uploadService=pomf",
        "urlClipboard=false",
    };

    COMPARE_LINES(describe(options), expected);
}

// What screenshotAction reads from the dialog's defaults (tray and updater turned off so the window starts quietly).
void tst_Settings::windowReadsDialogDefaults()
{
    REQUIRE_WINDOW_PLATFORM();

    {
        OptionsDialog dialog;
        QTRY_VERIFY(dialog.isEnabled());
        dialog.saveSettings();
    }

    mSettings->setValue("options/tray", false);
    mSettings->setValue("options/disableUpdater", true);

    LightscreenWindow window;
    Screenshot::Options options;
    QVERIFY(probeWindow(window, &options));

    const QStringList expected = {
        "animations=true", // Not written by the dialog.
        "clipboard=true",
        "currentMonitor=false",
        "cursor=false", // The dialog forces it off on Linux.
        "directory=<documents>/Screenshots",
        "file=true",
        "format=1",
        "magnify=false",
        "naming.dateFormat=yyyy-MM-dd",
        "naming.flip=false",
        "naming.leadingZeros=0",
        "naming.naming=0",
        "optimize=false",
        "prefix=screenshot.",
        "preview=false",
        "quality=100",
        "replace=false",
        "saveAs=false",
        "upload=false",
        "uploadService=pomf",
        "urlClipboard=false",
    };

    COMPARE_LINES(describe(options), expected);
}

// screenshotAction reads the settings once per window and caches them until applySettings runs (after the options
// dialog closes). A setting changed behind its back is not seen by the next screenshot.
void tst_Settings::windowCachesOptions()
{
    REQUIRE_WINDOW_PLATFORM();

    mSettings->setValue("file/format", 1);
    mSettings->setValue("file/prefix", "before");
    mSettings->setValue("options/tray", false);
    mSettings->setValue("options/disableUpdater", true);

    LightscreenWindow window;
    Screenshot::Options options;
    QVERIFY(probeWindow(window, &options));
    QCOMPARE(options.prefix, QStringLiteral("before"));

    mSettings->setValue("file/prefix", "after");
    QVERIFY(probeWindow(window, &options));
    QCOMPARE(options.prefix, QStringLiteral("before"));
}

// For PNG the Screenshot constructor replaces options/quality with 80 (known-bugs.md, bug 20). Other formats keep it.
void tst_Settings::windowPngQualityIsForced()
{
    REQUIRE_WINDOW_PLATFORM();

    mSettings->setValue("file/format", Screenshot::PNG);
    mSettings->setValue("options/quality", 30);
    mSettings->setValue("options/tray", false);
    mSettings->setValue("options/disableUpdater", true);

    {
        LightscreenWindow window;
        Screenshot::Options options;
        QVERIFY(probeWindow(window, &options));
        QCOMPARE(options.quality, 80);
    }

    mSettings->setValue("file/format", Screenshot::JPEG);

    LightscreenWindow window;
    Screenshot::Options options;
    QVERIFY(probeWindow(window, &options));
    QCOMPARE(options.quality, 30);
}

QTEST_MAIN(tst_Settings)
#include "tst_settings.moc"
