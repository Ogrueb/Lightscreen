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
#ifndef TASKBAR_H
#define TASKBAR_H

#include <QtGlobal>

class QIcon;
class QWindow;

#ifdef Q_OS_WIN
struct ITaskbarList3;
#endif

// The window's taskbar button: overlay icon and progress (ITaskbarList3).
// Windows only; every call does nothing elsewhere, or before setWindow().
class Taskbar
{
public:
    enum class Progress { None, Indeterminate, Normal, Paused };

#ifdef Q_OS_WIN
    Taskbar();
    ~Taskbar();

    void setWindow(QWindow *window);
    void setOverlayIcon(const QIcon &icon);
    void clearOverlayIcon();
    void setProgressState(Progress state);
    void setProgressValue(int percent);

private:
    Q_DISABLE_COPY(Taskbar)
    bool ready();

    ITaskbarList3 *mTaskbar = nullptr;
    bool mFailed = false;
    quintptr mWindow = 0;
    int mValue = 0;
#else
    void setWindow(QWindow *window) { Q_UNUSED(window) }
    void setOverlayIcon(const QIcon &icon) { Q_UNUSED(icon) }
    void clearOverlayIcon() {}
    void setProgressState(Progress state) { Q_UNUSED(state) }
    void setProgressValue(int percent) { Q_UNUSED(percent) }
#endif
};

#endif // TASKBAR_H
