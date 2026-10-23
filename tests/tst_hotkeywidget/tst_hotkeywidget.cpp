// Stage 4 safety net, session 5: widgets/hotkeywidget (docs/modernization-plan.md, section 6, item 5).
//
// Records how HotkeyWidget turns key presses into the hotkey string the options dialog saves, quirks included.
// M09 rewrites the key-combination line in keyPressEvent (QKeyCombination); the keyPress* tests are its check.
#include <QtTest>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QVBoxLayout>

#include <widgets/hotkeywidget.h>

class tst_HotkeyWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void initialState();
    void setHotkeyRoundTrip_data();
    void setHotkeyRoundTrip();
    void setHotkeyUnusualStrings_data();
    void setHotkeyUnusualStrings();
    void keyPress_data();
    void keyPress();
    void keyPressModifierOnly_data();
    void keyPressModifierOnly();
    void keyPressInvalid_data();
    void keyPressInvalid();
    void escapeWhileFocused();
    void clickFocusPromptsForHotkey();
    void tabFocusIgnoresKeys();
    void tabFocusThenKeyStartsEntry_data();
    void tabFocusThenKeyStartsEntry();
    void tabFocusMovesOnWithTab();
    void tabDuringEntryMovesOn_data();
    void tabDuringEntryMovesOn();
    void ctrlTabDuringEntryIsRecorded();
    void languageChangeResetsText();

private:
    void focusAndWait(Qt::FocusReason reason);
    void click(int key, int modifiers);

    // HotkeyWidget needs a parent: setHotkey and keyPressEvent call parentWidget()->setFocus() unchecked.
    QWidget *mWindow = nullptr;
    HotkeyWidget *mWidget = nullptr;
    QPushButton *mOther = nullptr;
};

void tst_HotkeyWidget::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
}

void tst_HotkeyWidget::init()
{
    mWindow = new QWidget;
    auto layout = new QVBoxLayout(mWindow);
    mWidget = new HotkeyWidget(mWindow);
    mOther = new QPushButton(QStringLiteral("other"), mWindow);
    layout->addWidget(mWidget);
    layout->addWidget(mOther);
}

void tst_HotkeyWidget::cleanup()
{
    delete mWindow;
    mWindow = nullptr;
    mWidget = nullptr;
    mOther = nullptr;
}

void tst_HotkeyWidget::focusAndWait(Qt::FocusReason reason)
{
    // Under Xvfb there is no window manager, so the window is activated by hand once it is mapped.
    mWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(mWindow));
    mWindow->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(mWindow));
    mWidget->setFocus(reason);
    QTRY_VERIFY(mWidget->hasFocus());
}

// QTest::keyClick(widget, Qt::Key) asserts on keys it has no ASCII for (AltGr, Super, unknown), so the key tests
// send press and release with an explicit empty text instead.
void tst_HotkeyWidget::click(int key, int modifiers)
{
    QTest::sendKeyEvent(QTest::Press, mWidget, Qt::Key(key), QString(), Qt::KeyboardModifiers(modifiers));
    QTest::sendKeyEvent(QTest::Release, mWidget, Qt::Key(key), QString(), Qt::KeyboardModifiers(modifiers));
}

void tst_HotkeyWidget::initialState()
{
    QCOMPARE(mWidget->hotkey(), QString());
    QCOMPARE(mWidget->text(), QStringLiteral("Click to select hotkey..."));
    QCOMPARE(mWidget->objectName(), QStringLiteral("HotkeyWidget"));
}

void tst_HotkeyWidget::setHotkeyRoundTrip_data()
{
    QTest::addColumn<QString>("hotkey");

    // The four from the plan, plus the other two OptionsDialog defaults.
    QTest::newRow("Print") << "Print";
    QTest::newRow("Ctrl+Print") << "Ctrl+Print";
    QTest::newRow("Ctrl+Alt+Print") << "Ctrl+Alt+Print";
    QTest::newRow("Ctrl+PgUp") << "Ctrl+PgUp";
    QTest::newRow("Alt+Print") << "Alt+Print";
    QTest::newRow("Ctrl+PgDown") << "Ctrl+PgDown";
}

void tst_HotkeyWidget::setHotkeyRoundTrip()
{
    QFETCH(QString, hotkey);

    mWidget->setHotkey(hotkey);
    QCOMPARE(mWidget->hotkey(), hotkey);
    QCOMPARE(mWidget->text(), QKeySequence(hotkey).toString(QKeySequence::NativeText));
}

void tst_HotkeyWidget::setHotkeyUnusualStrings_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("hotkey");

    // The empty string clears the hotkey, and the "Click to select hotkey..." prompt does not come back.
    QTest::newRow("empty") << "" << "";
    // Parsing is NativeText, saving PortableText; for these strings both agree on Linux and Windows.
    QTest::newRow("modifier order is normalized") << "Alt+Ctrl+Print" << "Ctrl+Alt+Print";
    QTest::newRow("lower case") << "ctrl+print" << "Ctrl+Print";
    // A multi-key sequence survives setHotkey/hotkey, although keyPressEvent can only make single keys.
    QTest::newRow("two keys") << "Ctrl+A, B" << "Ctrl+A, B";
    // An unknown key name gives an empty sequence.
    QTest::newRow("nonsense") << "Nonsense" << "";
}

void tst_HotkeyWidget::setHotkeyUnusualStrings()
{
    QFETCH(QString, input);
    QFETCH(QString, hotkey);

    mWidget->setHotkey(input);
    QCOMPARE(mWidget->hotkey(), hotkey);
    QCOMPARE(mWidget->text(), QKeySequence(hotkey).toString(QKeySequence::NativeText));
}

void tst_HotkeyWidget::keyPress_data()
{
    QTest::addColumn<int>("key");
    QTest::addColumn<int>("modifiers");
    QTest::addColumn<QString>("hotkey");

    QTest::newRow("Print") << int(Qt::Key_Print) << int(Qt::NoModifier) << "Print";
    QTest::newRow("Ctrl+Print") << int(Qt::Key_Print) << int(Qt::ControlModifier) << "Ctrl+Print";
    QTest::newRow("Ctrl+Alt+Print") << int(Qt::Key_Print) << int(Qt::ControlModifier | Qt::AltModifier)
                                    << "Ctrl+Alt+Print";
    QTest::newRow("Ctrl+PgUp") << int(Qt::Key_PageUp) << int(Qt::ControlModifier) << "Ctrl+PgUp";
    QTest::newRow("Shift+A") << int(Qt::Key_A) << int(Qt::ShiftModifier) << "Shift+A";
    QTest::newRow("Meta+F12") << int(Qt::Key_F12) << int(Qt::MetaModifier) << "Meta+F12";
    // The keypad flag is stripped, so keypad 5 and the 5 above the letters give the same hotkey.
    QTest::newRow("keypad 5") << int(Qt::Key_5) << int(Qt::KeypadModifier) << "5";
    QTest::newRow("Ctrl+keypad 5") << int(Qt::Key_5) << int(Qt::ControlModifier | Qt::KeypadModifier) << "Ctrl+5";
}

void tst_HotkeyWidget::keyPress()
{
    QFETCH(int, key);
    QFETCH(int, modifiers);
    QFETCH(QString, hotkey);

    mWidget->setHotkey(QStringLiteral("F1"));
    click(key, modifiers);
    QCOMPARE(mWidget->hotkey(), hotkey);
    QCOMPARE(mWidget->text(), QKeySequence(hotkey).toString(QKeySequence::NativeText));
}

void tst_HotkeyWidget::keyPressModifierOnly_data()
{
    QTest::addColumn<int>("key");
    QTest::addColumn<int>("modifiers");

    QTest::newRow("Shift") << int(Qt::Key_Shift) << int(Qt::ShiftModifier);
    QTest::newRow("Control") << int(Qt::Key_Control) << int(Qt::ControlModifier);
    QTest::newRow("Alt") << int(Qt::Key_Alt) << int(Qt::AltModifier);
    QTest::newRow("Meta") << int(Qt::Key_Meta) << int(Qt::MetaModifier);
    QTest::newRow("AltGr") << int(Qt::Key_AltGr) << int(Qt::GroupSwitchModifier);
    QTest::newRow("Super_L") << int(Qt::Key_Super_L) << int(Qt::NoModifier);
    QTest::newRow("Super_R") << int(Qt::Key_Super_R) << int(Qt::NoModifier);
    // The context-menu key counts as a modifier too, so it can never be a hotkey.
    QTest::newRow("Menu") << int(Qt::Key_Menu) << int(Qt::NoModifier);
}

void tst_HotkeyWidget::keyPressModifierOnly()
{
    QFETCH(int, key);
    QFETCH(int, modifiers);

    mWidget->setHotkey(QStringLiteral("Ctrl+Print"));
    const QString text = mWidget->text();

    click(key, modifiers);
    QCOMPARE(mWidget->hotkey(), QStringLiteral("Ctrl+Print"));
    QCOMPARE(mWidget->text(), text);
}

void tst_HotkeyWidget::keyPressInvalid_data()
{
    QTest::addColumn<int>("key");
    QTest::addColumn<int>("modifiers");

    QTest::newRow("Escape") << int(Qt::Key_Escape) << int(Qt::NoModifier);
    QTest::newRow("Ctrl+Escape") << int(Qt::Key_Escape) << int(Qt::ControlModifier);
    QTest::newRow("unknown") << int(Qt::Key_unknown) << int(Qt::NoModifier);
}

void tst_HotkeyWidget::keyPressInvalid()
{
    QFETCH(int, key);
    QFETCH(int, modifiers);

    QSignalSpy errors(mWidget, &HotkeyWidget::invalidHotkeyError);
    mWidget->setHotkey(QStringLiteral("Ctrl+Print"));

    // Without focus there is no FocusOut, so the text stays and no signal fires.
    click(key, modifiers);
    QCOMPARE(mWidget->text(), QStringLiteral("Invalid hotkey"));
    QCOMPARE(mWidget->hotkey(), QStringLiteral("Ctrl+Print"));
    QCOMPARE(errors.count(), 0);
}

void tst_HotkeyWidget::escapeWhileFocused()
{
    mWidget->setHotkey(QStringLiteral("Ctrl+Print"));
    focusAndWait(Qt::MouseFocusReason);

    const QString defaultStyleSheet = mWidget->styleSheet();
    QSignalSpy errors(mWidget, &HotkeyWidget::invalidHotkeyError);

    // "Invalid hotkey" moves focus to the parent; the FocusOut emits the signal, colours the text red for a second
    // and puts the old hotkey's text straight back, so "Invalid hotkey" is never seen.
    QTest::keyClick(mWidget, Qt::Key_Escape);
    QCOMPARE(errors.count(), 1);
    QVERIFY(!mWidget->hasFocus());
    QCOMPARE(mWidget->hotkey(), QStringLiteral("Ctrl+Print"));
    QCOMPARE(mWidget->text(), QKeySequence(QStringLiteral("Ctrl+Print")).toString(QKeySequence::NativeText));
    QCOMPARE(mWidget->styleSheet(), defaultStyleSheet + QStringLiteral("color: #d90000;"));
    QTRY_COMPARE_WITH_TIMEOUT(mWidget->styleSheet(), defaultStyleSheet, 3000);
}

void tst_HotkeyWidget::clickFocusPromptsForHotkey()
{
    mWidget->setHotkey(QStringLiteral("Ctrl+Print"));
    focusAndWait(Qt::MouseFocusReason);
    QCOMPARE(mWidget->text(), QStringLiteral("Type your hotkey"));

    // A valid key sets the hotkey and gives focus back to the parent.
    QTest::keyClick(mWidget, Qt::Key_F5, Qt::AltModifier);
    QCOMPARE(mWidget->hotkey(), QStringLiteral("Alt+F5"));
    QCOMPARE(mWidget->text(), QKeySequence(QStringLiteral("Alt+F5")).toString(QKeySequence::NativeText));
    QVERIFY(!mWidget->hasFocus());
}

void tst_HotkeyWidget::tabFocusIgnoresKeys()
{
    mWidget->setHotkey(QStringLiteral("Ctrl+Print"));
    focusAndWait(Qt::TabFocusReason);
    QCOMPARE(mWidget->text(), QKeySequence(QStringLiteral("Ctrl+Print")).toString(QKeySequence::NativeText));

    // Focus reached with Tab only highlights the button: other keys are ignored, Escape included, so tabbing
    // through the dialog cannot change a hotkey by accident.
    QTest::keyClick(mWidget, Qt::Key_F5);
    QTest::keyClick(mWidget, Qt::Key_Escape);
    QTest::keyClick(mWidget, Qt::Key_A, Qt::ControlModifier);
    QCOMPARE(mWidget->hotkey(), QStringLiteral("Ctrl+Print"));
    QCOMPARE(mWidget->text(), QKeySequence(QStringLiteral("Ctrl+Print")).toString(QKeySequence::NativeText));
    QVERIFY(mWidget->hasFocus());

    // Leaving clears the Tab-focus state: a later key press without focus is recorded, as for a widget that was
    // never focused (bug 26 left the flag set until the next non-Tab FocusIn).
    mOther->setFocus(Qt::TabFocusReason);
    QTRY_VERIFY(!mWidget->hasFocus());
    QTest::keyClick(mWidget, Qt::Key_F5);
    QCOMPARE(mWidget->hotkey(), QStringLiteral("F5"));
}

void tst_HotkeyWidget::tabFocusThenKeyStartsEntry_data()
{
    QTest::addColumn<int>("key");

    QTest::newRow("Space") << int(Qt::Key_Space);
    QTest::newRow("Return") << int(Qt::Key_Return);
    QTest::newRow("Enter") << int(Qt::Key_Enter);
}

void tst_HotkeyWidget::tabFocusThenKeyStartsEntry()
{
    QFETCH(int, key);

    mWidget->setHotkey(QStringLiteral("Ctrl+Print"));
    focusAndWait(Qt::TabFocusReason);

    // Bug 26: Space or Enter on a button reached with Tab starts hotkey entry, as a click does.
    QTest::keyClick(mWidget, Qt::Key(key));
    QCOMPARE(mWidget->text(), QStringLiteral("Type your hotkey"));
    QCOMPARE(mWidget->hotkey(), QStringLiteral("Ctrl+Print"));
    QVERIFY(mWidget->hasFocus());

    // The next key is the hotkey; focus goes back to the parent, as after a click.
    QTest::keyClick(mWidget, Qt::Key_F5, Qt::AltModifier);
    QCOMPARE(mWidget->hotkey(), QStringLiteral("Alt+F5"));
    QCOMPARE(mWidget->text(), QKeySequence(QStringLiteral("Alt+F5")).toString(QKeySequence::NativeText));
    QVERIFY(!mWidget->hasFocus());
}

void tst_HotkeyWidget::tabFocusMovesOnWithTab()
{
    mWidget->setHotkey(QStringLiteral("Ctrl+Print"));
    focusAndWait(Qt::TabFocusReason);

    // Tab still moves on to the next widget, so a keyboard user is not trapped on the hotkey button. Focus must
    // stay there: leaving used to hand it to the parent instead.
    QTest::keyClick(mWidget, Qt::Key_Tab);
    QTRY_VERIFY(mOther->hasFocus());
    QTest::qWait(50);
    QVERIFY(mOther->hasFocus());
    QCOMPARE(mWidget->hotkey(), QStringLiteral("Ctrl+Print"));

    // Backtab comes back, again only highlighting the button.
    QTest::keyClick(mOther, Qt::Key_Backtab, Qt::ShiftModifier);
    QTRY_VERIFY(mWidget->hasFocus());
    QCOMPARE(mWidget->text(), QKeySequence(QStringLiteral("Ctrl+Print")).toString(QKeySequence::NativeText));
}

// QA pass A5: with the button waiting for a hotkey ("Type your hotkey", after Space, Enter or a click), a plain
// Tab or Shift+Tab used to be recorded as the hotkey. It now ends the entry with the hotkey unchanged and moves focus
// on, so a keyboard user can always leave the button. A bare Tab is never a usable global hotkey.
void tst_HotkeyWidget::tabDuringEntryMovesOn_data()
{
    QTest::addColumn<int>("key");
    QTest::addColumn<int>("modifiers");

    QTest::newRow("Tab") << int(Qt::Key_Tab) << int(Qt::NoModifier);
    QTest::newRow("Shift+Tab as Backtab") << int(Qt::Key_Backtab) << int(Qt::ShiftModifier);
    QTest::newRow("Shift+Tab as Tab") << int(Qt::Key_Tab) << int(Qt::ShiftModifier);
}

void tst_HotkeyWidget::tabDuringEntryMovesOn()
{
    QFETCH(int, key);
    QFETCH(int, modifiers);

    mWidget->setHotkey(QStringLiteral("Ctrl+Print"));
    focusAndWait(Qt::TabFocusReason);
    QTest::keyClick(mWidget, Qt::Key_Space);
    QCOMPARE(mWidget->text(), QStringLiteral("Type your hotkey"));

    click(key, modifiers);

    QCOMPARE(mWidget->hotkey(), QStringLiteral("Ctrl+Print"));
    QTRY_VERIFY(mOther->hasFocus());
    QCOMPARE(mWidget->text(), QKeySequence(QStringLiteral("Ctrl+Print")).toString(QKeySequence::NativeText));
    QVERIFY(QWidget::keyboardGrabber() != mWidget);
}

// A combination with Tab is still a hotkey.
void tst_HotkeyWidget::ctrlTabDuringEntryIsRecorded()
{
    mWidget->setHotkey(QStringLiteral("Ctrl+Print"));
    focusAndWait(Qt::TabFocusReason);
    QTest::keyClick(mWidget, Qt::Key_Space);

    click(Qt::Key_Tab, Qt::ControlModifier);

    QCOMPARE(mWidget->hotkey(), QStringLiteral("Ctrl+Tab"));
}

void tst_HotkeyWidget::languageChangeResetsText()
{
    mWidget->setHotkey(QStringLiteral("Ctrl+Print"));
    mWidget->setText(QStringLiteral("something else"));

    QEvent event(QEvent::LanguageChange);
    QApplication::sendEvent(mWidget, &event);
    QCOMPARE(mWidget->text(), QKeySequence(QStringLiteral("Ctrl+Print")).toString(QKeySequence::NativeText));
}

QTEST_MAIN(tst_HotkeyWidget)
#include "tst_hotkeywidget.moc"
