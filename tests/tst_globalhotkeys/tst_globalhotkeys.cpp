// The vendored UGlobalHotkeys on Windows (bug 33): hotkeys are registered to the thread, not to a window, so they
// keep working while every Lightscreen window is hidden (the app only in the tray). The test posts the WM_HOTKEY
// thread message Windows sends for a registered hotkey and checks that activated() fires, also under a hidden
// parent window. The Linux path is covered by tst_mainwindow's hotkey tests under Xvfb.
#include <QtTest>
#include <QWidget>

#include <tools/UGlobalHotkey/uglobalhotkeys.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class tst_GlobalHotkeys : public QObject
{
    Q_OBJECT

#ifdef Q_OS_WIN
private:
    // An unusual combination, unlikely to be taken on the CI runner.
    static constexpr const char *Sequence = "Ctrl+Alt+Shift+F12";

    static void postHotkey(size_t id)
    {
        QVERIFY(PostThreadMessageW(GetCurrentThreadId(), WM_HOTKEY, WPARAM(id), 0));
    }

    static bool registered(UGlobalHotkeys &hotkeys, size_t id)
    {
        return hotkeys.registerHotkey(QString::fromLatin1(Sequence), id);
    }

// QSKIP only leaves the function it is called in, so it is called in each test, not in a helper.
#define REQUIRE_REGISTERED(hotkeys, id)                                                                  \
    if (!registered(hotkeys, id)) {                                                                        \
        QSKIP("RegisterHotKey refused the test combination (taken by another program?)");                \
    }
#endif

private slots:
#ifndef Q_OS_WIN
    void initTestCase()
    {
        QSKIP("Windows only; tst_mainwindow covers the Linux path");
    }
#else
    void registeredHotkeyFires()
    {
        UGlobalHotkeys hotkeys;
        REQUIRE_REGISTERED(hotkeys, 7)
        QSignalSpy activated(&hotkeys, &UGlobalHotkeys::activated);

        postHotkey(7);

        QTRY_COMPARE(activated.count(), 1);
        QCOMPARE(activated.first().first().value<size_t>(), size_t(7));
    }

    // The tray case: the hotkeys object belongs to a main window that is hidden.
    void firesWhileTheParentIsHidden()
    {
        QWidget window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        auto *hotkeys = new UGlobalHotkeys(&window);
        REQUIRE_REGISTERED(*hotkeys, 8)
        window.hide();
        QSignalSpy activated(hotkeys, &UGlobalHotkeys::activated);

        postHotkey(8);

        QTRY_COMPARE(activated.count(), 1);
    }

    void unknownIdIsIgnored()
    {
        UGlobalHotkeys hotkeys;
        REQUIRE_REGISTERED(hotkeys, 9)
        QSignalSpy activated(&hotkeys, &UGlobalHotkeys::activated);

        postHotkey(99);
        QTest::qWait(200);

        QCOMPARE(activated.count(), 0);
    }

    void unregisteredHotkeyIsIgnored()
    {
        UGlobalHotkeys hotkeys;
        REQUIRE_REGISTERED(hotkeys, 10)
        hotkeys.unregisterHotkey(10);
        QSignalSpy activated(&hotkeys, &UGlobalHotkeys::activated);

        postHotkey(10);
        QTest::qWait(200);

        QCOMPARE(activated.count(), 0);
    }
#endif
};

QTEST_MAIN(tst_GlobalHotkeys)
#include "tst_globalhotkeys.moc"
