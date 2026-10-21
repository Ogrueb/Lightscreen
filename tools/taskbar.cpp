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
#include <QIcon>
#include <QPixmap>
#include <QWindow>

#include <qt_windows.h>
#include <shobjidl.h>

#include <tools/taskbar.h>

Taskbar::Taskbar() = default;

Taskbar::~Taskbar()
{
    if (mTaskbar) {
        mTaskbar->Release();
    }
}

void Taskbar::setWindow(QWindow *window)
{
    mWindow = window ? static_cast<quintptr>(window->winId()) : 0;
}

void Taskbar::setOverlayIcon(const QIcon &icon)
{
    if (!ready()) {
        return;
    }

    const int size = ::GetSystemMetrics(SM_CXSMICON);
    const QPixmap pixmap = icon.pixmap(size, size);

    HICON hicon = pixmap.toImage().toHICON();

    // The taskbar keeps its own copy of the icon.
    mTaskbar->SetOverlayIcon(reinterpret_cast<HWND>(mWindow), hicon, L"");

    if (hicon) {
        ::DestroyIcon(hicon);
    }
}

void Taskbar::clearOverlayIcon()
{
    if (ready()) {
        mTaskbar->SetOverlayIcon(reinterpret_cast<HWND>(mWindow), nullptr, nullptr);
    }
}

void Taskbar::setProgressState(Progress state)
{
    if (state == Progress::None) {
        mValue = 0;
    }

    if (!ready()) {
        return;
    }

    const HWND hwnd = reinterpret_cast<HWND>(mWindow);

    switch (state) {
    case Progress::None:
        mTaskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
        break;
    case Progress::Indeterminate:
        mTaskbar->SetProgressState(hwnd, TBPF_INDETERMINATE);
        break;
    case Progress::Normal:
    case Progress::Paused:
        // SetProgressValue leaves "no progress" for "normal"; the state is set after it so "paused" sticks.
        mTaskbar->SetProgressValue(hwnd, static_cast<ULONGLONG>(mValue), 100);
        mTaskbar->SetProgressState(hwnd, state == Progress::Paused ? TBPF_PAUSED : TBPF_NORMAL);
        break;
    }
}

void Taskbar::setProgressValue(int percent)
{
    mValue = qBound(0, percent, 100);

    if (ready()) {
        mTaskbar->SetProgressValue(reinterpret_cast<HWND>(mWindow), static_cast<ULONGLONG>(mValue), 100);
    }
}

// Creates the COM object on first use (Qt has already initialised COM on the GUI thread).
bool Taskbar::ready()
{
    if (!mWindow || mFailed) {
        return false;
    }

    if (mTaskbar) {
        return true;
    }

    ITaskbarList3 *taskbar = nullptr;

    if (FAILED(::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&taskbar)))) {
        mFailed = true;
        return false;
    }

    if (FAILED(taskbar->HrInit())) {
        taskbar->Release();
        mFailed = true;
        return false;
    }

    mTaskbar = taskbar;
    return true;
}
