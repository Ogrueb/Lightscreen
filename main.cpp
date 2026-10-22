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
#include <QApplication>
#include <QLocale>

#include <tools/os.h>
#include <tools/settings.h>
#include "tools/SingleApplication/singleapplication.h"

#include <lightscreenwindow.h>

int main(int argc, char *argv[])
{
#ifdef QT_DEBUG
    qSetMessagePattern("%{message} @%{line}[%{function}()]");
#endif

    Settings::applyApplicationIdentity();
    QApplication::setApplicationVersion(APP_VERSION);
    // Qt 6 always scales for high DPI. PassThrough (Qt 6's default, set here to be explicit) uses the exact Windows
    // scale (125%, 150%, 175%), so the windows match other apps; until 3.0 it was rounded to 100% or 200% as in
    // Qt 5. Capture and area selection work in fractional ratios since S04 (tools/capturegeometry).
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    SingleApplication application(argc, argv);

    application.setQuitOnLastWindowClosed(false);

    LightscreenWindow lightscreen;

    if (application.arguments().size() > 1) {
        lightscreen.executeArguments(application.arguments());
    } else {
        lightscreen.show();
    }

    QObject::connect(&application, &SingleApplication::instanceArguments, &lightscreen, &LightscreenWindow::executeArguments);
    QObject::connect(&lightscreen, &LightscreenWindow::finished, &application, &SingleApplication::quit);

    return application.exec();
}
