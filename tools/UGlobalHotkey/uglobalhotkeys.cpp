#include <QtCore>
#if defined(Q_OS_WIN)
#include <windows.h>
#include <QCoreApplication>
#elif defined(Q_OS_LINUX)
#include <QApplication>
#endif

#include "hotkeymap.h"
#include "uglobalhotkeys.h"

#include <QDebug>

UGlobalHotkeys::UGlobalHotkeys(QWidget *parent)
    : QWidget(parent)
{
#if defined(Q_OS_WIN)
    // Hotkeys are registered to the thread, not to a window (RegisterHotKey with no HWND): WM_HOTKEY then arrives
    // as a thread message, which Qt's event dispatcher passes to native event filters. Registered to this widget's
    // own handle, as upstream did, they stopped arriving on Qt 6 once the main window was hidden to the tray
    // (bug 33).
    qApp->installNativeEventFilter(this);
#elif defined(Q_OS_LINUX)
    qApp->installNativeEventFilter(this);
    X11Connection = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
    X11Wid = xcb_setup_roots_iterator(xcb_get_setup(X11Connection)).data->root;
    X11KeySymbs = xcb_key_symbols_alloc(X11Connection);
#endif
}

bool UGlobalHotkeys::registerHotkey(const QString &keySeq, size_t id)
{
    return registerHotkey(UKeySequence(keySeq), id);
}

#if defined(Q_OS_MAC)
OSStatus macHotkeyHandler(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData)
{
    Q_UNUSED(nextHandler);
    EventHotKeyID hkCom;
    GetEventParameter(theEvent, kEventParamDirectObject, typeEventHotKeyID, NULL,
                      sizeof(hkCom), NULL, &hkCom);
    size_t id = hkCom.id;

    UGlobalHotkeys *caller = (UGlobalHotkeys *)userData;
    caller->onHotkeyPressed(id);
    return noErr;
}
#endif

bool UGlobalHotkeys::registerHotkey(const UKeySequence &keySeq, size_t id)
{
    if (keySeq.size() == 0) {
        return false;
    }
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    if (Registered.find(id) != Registered.end()) {
        unregisterHotkey(id);
    }
#endif
#if defined(Q_OS_WIN)
    size_t winMod = 0;
    size_t key = VK_F2;

    for (size_t i = 0; i != keySeq.size(); i++) {
        if (keySeq[i] == Qt::Key_Control) {
            winMod |= MOD_CONTROL;
        } else if (keySeq[i] == Qt::Key_Alt) {
            winMod |= MOD_ALT;
        } else if (keySeq[i] == Qt::Key_Shift) {
            winMod |= MOD_SHIFT;
        } else if (keySeq[i] == Qt::Key_Meta) {
            winMod |= MOD_WIN;
        } else {
            key = QtKeyToWin(keySeq[i]);
        }
    }

    if (!RegisterHotKey(nullptr, int(id), UINT(winMod), UINT(key))) {
        return false;
    } else {
        Registered.insert(id);
    }
#elif defined(Q_OS_LINUX)
    regLinuxHotkey(keySeq, id);
#endif
#if defined(Q_OS_MAC)
    unregisterHotkey(id);

    EventHotKeyRef gMyHotKeyRef;
    EventHotKeyID gMyHotKeyID;
    EventTypeSpec eventType;
    eventType.eventClass = kEventClassKeyboard;
    eventType.eventKind = kEventHotKeyPressed;

    InstallApplicationEventHandler(&macHotkeyHandler, 1, &eventType, this, NULL);

    gMyHotKeyID.signature = uint32_t(id);
    gMyHotKeyID.id = uint32_t(id);

    UKeyData macKey = QtKeyToMac(keySeq);

    RegisterEventHotKey(macKey.key, macKey.mods, gMyHotKeyID,
                        GetApplicationEventTarget(), 0, &gMyHotKeyRef);

    HotkeyRefs[id] = gMyHotKeyRef;
#endif

    return true;
}

void UGlobalHotkeys::unregisterHotkey(size_t id)
{
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    Q_ASSERT(Registered.find(id) != Registered.end() && "Unregistered hotkey");
#endif
#if defined(Q_OS_WIN)
    UnregisterHotKey(nullptr, int(id));
#elif defined(Q_OS_LINUX)
    unregLinuxHotkey(id);
#endif
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    Registered.remove(id);
#endif
#if defined(Q_OS_MAC)
    if (HotkeyRefs.find(id) != HotkeyRefs.end()) {
        UnregisterEventHotKey(HotkeyRefs[id]);
    }
#endif
}

void UGlobalHotkeys::unregisterAllHotkeys()
{
#ifdef Q_OS_WIN
    const QSet<size_t> ids = Registered; // unregisterHotkey() removes from Registered
    for (const size_t id : ids) {
        this->unregisterHotkey(id);
    }
#elif defined(Q_OS_LINUX)
    const QList<size_t> ids = Registered.keys(); // unregisterHotkey() removes from Registered
    for (const size_t id : ids) {
        this->unregisterHotkey(id);
    }
#endif
}

UGlobalHotkeys::~UGlobalHotkeys()
{
#if defined(Q_OS_WIN)
    for (QSet<size_t>::iterator i = Registered.begin(); i != Registered.end(); ++i) {
        UnregisterHotKey(nullptr, int(*i));
    }
#elif defined(Q_OS_LINUX)
    xcb_key_symbols_free(X11KeySymbs);
#endif
}

#if defined(Q_OS_MAC)
void UGlobalHotkeys::onHotkeyPressed(size_t id)
{
    emit activated(id);
}
#endif

#if defined(Q_OS_WIN)
bool UGlobalHotkeys::nativeEventFilter(const QByteArray &eventType, void *message, UNativeEventResult *result)
{
    Q_UNUSED(result);

    if (eventType != "windows_generic_MSG") {
        return false;
    }

    const MSG *msg = static_cast<const MSG *>(message);

    // Thread hotkeys carry no window. Another UGlobalHotkeys in the same thread sees the message too and ignores
    // ids it did not register.
    if (msg->message != WM_HOTKEY || msg->hwnd != nullptr || !Registered.contains(size_t(msg->wParam))) {
        return false;
    }

    emit activated(size_t(msg->wParam));
    return true;
}

#elif defined(Q_OS_LINUX)

bool UGlobalHotkeys::nativeEventFilter(const QByteArray &eventType, void *message, UNativeEventResult *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    return linuxEvent(static_cast<xcb_generic_event_t *>(message));
}

bool UGlobalHotkeys::linuxEvent(xcb_generic_event_t *message)
{
    if ((message->response_type & ~0x80) == XCB_KEY_PRESS) {
        xcb_key_press_event_t *ev = (xcb_key_press_event_t *)message;
        auto ind = Registered.key({ev->detail, (ev->state & ~XCB_MOD_MASK_2)});

        if (ind == 0) // this is not hotkeys
            return false;

        emit activated(ind);
        return true;
    }
    return false;
}

void UGlobalHotkeys::regLinuxHotkey(const UKeySequence &keySeq, size_t id)
{
    UHotkeyData data;
    UKeyData keyData = QtKeyToLinux(keySeq);
    
    xcb_keycode_t *keyC = xcb_key_symbols_get_keycode(X11KeySymbs, keyData.key);

    if (keyC == XCB_NO_SYMBOL) { // 0x0
        qWarning() << "Cannot find symbol";
        return;
    }

    data.keyCode = *keyC;
    data.mods = keyData.mods;

    xcb_grab_key(X11Connection, 1, X11Wid, data.mods, data.keyCode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    // NumLk
    xcb_grab_key(X11Connection, 1, X11Wid, data.mods | XCB_MOD_MASK_2, data.keyCode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    Registered.insert(id, data);
}

void UGlobalHotkeys::unregLinuxHotkey(size_t id)
{
    UHotkeyData data = Registered.take(id);
    xcb_ungrab_key(X11Connection, data.keyCode, X11Wid, data.mods);
    xcb_ungrab_key(X11Connection, data.keyCode, X11Wid, data.mods | XCB_MOD_MASK_2);
}
#endif
