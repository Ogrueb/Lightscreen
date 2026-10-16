/*
 * Copyright (C) Christian Kaiser
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef OS_H_
#define OS_H_

#include <QWidget>

class QPixmap;
class QPoint;
class QString;
class QUrl;
class QGraphicsEffect;
class QIcon;

#if defined(Q_OS_WIN)
    // As <windows.h> declares them (STRICT), without pulling it into every includer.
    struct HBITMAP__;
    typedef struct HBITMAP__ *HBITMAP;
    struct HICON__;
    typedef struct HICON__ *HICON;
#elif defined(Q_OS_LINUX)
    typedef unsigned long XID;
    typedef XID Window;
    typedef struct _XDisplay Display;
#endif

namespace os {
enum class Sound { Screenshot, Error };

// Plays a sound cue without waiting for it to finish (Windows only; does nothing elsewhere).
void playSound(Sound sound);

#if defined(Q_OS_WIN)
// QtWinExtras' conversions on Qt 5, QImage's on Qt 6. alpha reads the bitmap as premultiplied ARGB.
QPixmap pixmapFromHBITMAP(HBITMAP bitmap, bool alpha = false);
QPixmap pixmapFromHICON(HICON icon);
#endif

// Returns the cursor pixmap in Windows
QPair<QPixmap, QPoint> cursor();

// A QTimeLine based effect for a slot (TODO: look at the new effect classes)
void effect(QObject *target, const char *slot, int frames, int duration = 400, const char *cleanup = 0);

// Returns the current users's Documents/My Documents folder
QString getDocumentsPath();

// Returns the pixmap of the given window id.
QPixmap grabWindow(WId winId);

// Set the target window as the foreground window (Windows only)
void setForegroundWindow(QWidget *window);

// Adds lightscreen to the startup list in Windows & Linux (KDE, Gnome and Xfce for now).
void setStartup(bool startup, bool hide);

// Returns the Exec= value of a Linux autostart entry for program: quoted and escaped per the Desktop Entry spec.
QString desktopExec(const QString &program, bool hide);

// Creates a new QGraphicsDropShadowEffect to apply to widgets.
QGraphicsEffect *shadow(const QColor &color = Qt::black, int blurRadius = 6, int offset = 1);

// Returns a QIcon for the given icon name (taking into account color schemes and whatnot).
QIcon icon(const QString &name, QColor backgroundColor = QColor());

// X11-specific functions for the Window Picker
#if defined(Q_OS_LINUX)
    // The application's X display connection (QX11Info on Qt 5, QNativeInterface::QX11Application on Qt 6).
    Display *x11Display();
    // The root window of the default screen of x11Display().
    Window x11RootWindow();
    Window findRealWindow(Window w, int depth = 0);
    Window windowUnderCursor(bool includeDecorations = true);
#endif
}

#endif /*OS_H_*/
