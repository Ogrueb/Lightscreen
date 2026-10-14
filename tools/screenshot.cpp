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
#include <QClipboard>
#include <QDateTime>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QPainter>
#include <QPixmap>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>
#include <QScreen>
#include <QStandardPaths>
#include <QStringBuilder>

#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/windowpicker.h>
#include <tools/uploader/uploader.h>
#include <dialogs/areadialog.h>

#include <tools/os.h>

#ifdef Q_OS_WIN
    #include <windows.h>
#endif

#ifdef Q_OS_LINUX
    #include <QX11Info>
    #include <X11/X.h>
    #include <X11/Xlib.h>
    #undef Success
#endif

#include <cstdio>

namespace {

// Moves source over target in one step, replacing target (QFile::rename refuses to replace an existing file).
bool replaceFile(const QString &source, const QString &target)
{
#ifdef Q_OS_WIN
    return MoveFileExW(reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(source).utf16()),
                       reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(target).utf16()),
                       MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return std::rename(QFile::encodeName(source).constData(), QFile::encodeName(target).constData()) == 0;
#endif
}

} // namespace

Screenshot::Screenshot(QObject *parent, Screenshot::Options options):
    QObject(parent),
    mOptions(options),
    mPixmapDelay(false),
    mUnloaded(false),
    mUnloadFilename()
{
    mOptions.format = formatFromSetting(int(mOptions.format));

    if (mOptions.format == Screenshot::PNG) {
        mOptions.quality = 80;
    }
}

Screenshot::~Screenshot()
{
    if (!mUnloadFilename.isEmpty()) {
        QFile::remove(mUnloadFilename);
    }
}

QString Screenshot::getName(const NamingOptions &options, const QString &prefix, const QDir &directory)
{
    QString naming;
    int naming_largest = 0;

    if (options.flip) {
        naming = "%1" % prefix;
    } else {
        naming = prefix % "%1";
    }

    switch (options.naming) {
    case Screenshot::Numeric: // Numeric
        // Iterating through the folder to find the largest numeric naming.
        for (auto file : directory.entryList(QDir::Files)) {
            if (file.contains(prefix)) {
                file.chop(file.size() - file.lastIndexOf("."));
                file.remove(prefix);

                if (file.toInt() > naming_largest) {
                    naming_largest = file.toInt();
                }
            }
        }

        if (options.leadingZeros > 0) {
            //Pretty, huh?
            QString format;
            QTextStream(&format) << "%0" << (options.leadingZeros + 1) << "d";

            naming = naming.arg(QString().sprintf(format.toLatin1(), naming_largest + 1));
        } else {
            naming = naming.arg(naming_largest + 1);
        }
        break;
    case  Screenshot::Date: // Date
        naming = naming.arg(QLocale().toString(QDateTime::currentDateTime(), options.dateFormat));
        break;
    case  Screenshot::Timestamp: // Timestamp
        naming = naming.arg(QDateTime::currentDateTime().toTime_t());
        break;
    case  Screenshot::Empty:
        naming = naming.arg("");
        break;
    }

    return naming;
}

const QString &Screenshot::unloadedFileName()
{
    return mUnloadFilename;
}

const Screenshot::Options &Screenshot::options()
{
    return mOptions;
}

QPixmap &Screenshot::pixmap()
{
    return mPixmap;
}

//

void Screenshot::confirm(bool result)
{
    if (result) {
        save();
    } else {
        mOptions.result = Screenshot::Cancel;
        emit finished();
    }

    emit cleanup();

    mPixmap = QPixmap();
}

void Screenshot::confirmation()
{
    emit askConfirmation();

    if (mOptions.file) {
        unloadPixmap();
    }
}

void Screenshot::discard()
{
    confirm(false);
}

void Screenshot::markUpload()
{
    mOptions.upload = true;
}

void Screenshot::optimize()
{
    QString optiPNG;

#ifdef Q_OS_UNIX
    optiPNG = QStandardPaths::findExecutable("optipng");
#else
    optiPNG = qApp->applicationDirPath() % QDir::separator() % "optipng.exe";

    if (!QFile::exists(optiPNG)) {
        optiPNG.clear();
    }
#endif

    if (optiPNG.isEmpty()) {
        optimizationDone();
        return;
    }

    // optipng writes to a temporary file that replaces the original only on success, so an interrupted run leaves
    // the original intact. optipng refuses to overwrite an existing output, such as one left by an earlier crash.
    const QString original  = mOptions.fileName;
    const QString temporary = original % ".optipng.tmp";
    QFile::remove(temporary);

    QProcess *process = new QProcess(this);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, process, original, temporary](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0 || !QFile::exists(temporary) || !replaceFile(temporary, original)) {
            QFile::remove(temporary);
        }

        process->deleteLater();
        optimizationDone();
    });

    // finished is not emitted when the process cannot be started.
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            process->deleteLater();
            optimizationDone();
        }
    });

    process->start(optiPNG, QStringList() << "-out" << temporary << original);
}

void Screenshot::optimizationDone()
{
    if (mOptions.upload) {
        upload();
    } else {
        emit finished();
    }
}

void Screenshot::save()
{
    QString name = "";
    QString fileName = "";
    Screenshot::Result result = Screenshot::Failure;

    if (mOptions.file && !mOptions.saveAs)  {
        name = newFileName();
    } else if (mOptions.file && mOptions.saveAs) {
        name = QFileDialog::getSaveFileName(0, tr("Save as.."), newFileName(), "*" % extension());
    }

    if (!mOptions.replace && QFile::exists(name % extension())) {
        name = uniqueName(name, extension());
    }

    if (mOptions.clipboard && !(mOptions.upload && mOptions.urlClipboard)) {
        // An unloaded pixmap is reloaded for the clipboard only, so the file branch below still renames the temp file.
        QApplication::clipboard()->setPixmap(mUnloaded ? QPixmap(mUnloadFilename) : mPixmap, QClipboard::Clipboard);

        if (!mOptions.file) {
            result = Screenshot::Success;
        }
    }

    if (mOptions.file) {
        fileName = name % extension();

        if (name.isEmpty()) {
            result = Screenshot::Cancel;
        } else if (mUnloaded) {
            result = (QFile::rename(mUnloadFilename, fileName)) ? Screenshot::Success : Screenshot::Failure;
        } else if (mPixmap.save(fileName, 0, mOptions.quality)) {
            result = Screenshot::Success;
        } else {
            result = Screenshot::Failure;
        }
    }

    mOptions.fileName = fileName;
    mOptions.result   = result;

    if (mOptions.result == Screenshot::Failure) {
        emit finished();

        // A failed save, or nothing to do, ends here: no history row, no optimize or upload.
        // Upload-only (no file, no clipboard image) also reports Failure and still uploads below.
        if (mOptions.file || !mOptions.upload) {
            return;
        }
    }

    if (mOptions.format == Screenshot::PNG && mOptions.optimize && mOptions.file) {
        if (!mOptions.upload) {
            ScreenshotManager::instance()->saveHistory(mOptions.fileName);
        }

        optimize();
    } else if (mOptions.upload) {
        upload();
    } else if (mOptions.file) {
        ScreenshotManager::instance()->saveHistory(mOptions.fileName);
        emit finished();
    } else {
        emit finished();
    }
}

void Screenshot::setPixmap(const QPixmap &pixmap)
{
    mPixmap = pixmap;

    if (mPixmap.isNull()) {
        emit confirm(false);
    } else {
        confirmation();
    }
}

void Screenshot::take()
{
    switch (mOptions.mode) {
    case Screenshot::WholeScreen:
        wholeScreen();
        break;

    case Screenshot::SelectedArea:
        selectedArea();
        break;

    case Screenshot::ActiveWindow:
        activeWindow();
        break;

    case Screenshot::SelectedWindow:
        selectedWindow();
        break;
    }

    if (mPixmapDelay) {
        return;
    }

    if (mPixmap.isNull()) {
        confirm(false);
    } else {
        confirmation();
    }
}

void Screenshot::upload()
{
    if (mOptions.file) {
        Uploader::instance()->upload(mOptions.fileName, mOptions.uploadService);
    } else if (unloadPixmap()) {
        Uploader::instance()->upload(mUnloadFilename, mOptions.uploadService);
    } else {
        emit finished();
    }
}

void Screenshot::uploadDone(const QString &url)
{
    if (mOptions.urlClipboard && !url.isEmpty()) {
        QApplication::clipboard()->setText(url, QClipboard::Clipboard);
    }

    emit finished();
}

void Screenshot::refresh()
{
    grabDesktop();
}

//

void Screenshot::activeWindow()
{
#ifdef Q_OS_WIN
    HWND fWindow = GetForegroundWindow();

    if (fWindow == NULL) {
        return;
    }

    if (fWindow == GetDesktopWindow()) {
        wholeScreen();
        return;
    }

    mPixmap = os::grabWindow((WId)GetForegroundWindow());
#endif

#if defined(Q_OS_LINUX)
    Window focus;
    int revert;

    XGetInputFocus(QX11Info::display(), &focus, &revert);

    mPixmap = QPixmap::grabWindow(focus);
#endif
}

const QString Screenshot::extension() const
{
    return extensionFor(mOptions.format);
}

QString Screenshot::extensionFor(Format format)
{
    switch (format) {
    case Screenshot::PNG:
        return QStringLiteral(".png");
        break;
    case Screenshot::BMP:
        return QStringLiteral(".bmp");
        break;
    case Screenshot::WEBP:
        return QStringLiteral(".webp");
        break;
    case Screenshot::JPEG:
        return QStringLiteral(".jpg");
        break;
    }

    return QStringLiteral(".png");
}

// Reads a "file/format" value; anything that is not a known format falls back to PNG.
Screenshot::Format Screenshot::formatFromSetting(const QVariant &value)
{
    bool ok = false;
    const int format = value.toInt(&ok);

    if (!ok || format < Screenshot::PNG || format > Screenshot::WEBP) {
        return Screenshot::PNG;
    }

    return Screenshot::Format(format);
}

// Appends " (n)" to the name, n being one more than the largest number among the files in the same directory
// named exactly "<name> (<n>)<extension>", then counting up until no such file exists.
QString Screenshot::uniqueName(const QString &nameWithoutExtension, const QString &extension)
{
    const QFileInfo info(nameWithoutExtension % extension);
    const QRegularExpression pattern(QRegularExpression::anchoredPattern(
        QRegularExpression::escape(QFileInfo(nameWithoutExtension).fileName()) % " \\((\\d+)\\)"
        % QRegularExpression::escape(extension)));

    int count = 0;

    for (const auto &file : info.dir().entryList(QDir::Files)) {
        const auto match = pattern.match(file);

        if (match.hasMatch()) {
            count = qMax(count, match.captured(1).toInt());
        }
    }

    QString candidate;

    do {
        candidate = nameWithoutExtension % " (" % QString::number(++count) % ")";
    } while (QFile::exists(candidate % extension));

    return candidate;
}

void Screenshot::grabDesktop()
{
    QRect geometry;
    QPoint cursorPosition = QCursor::pos();

    if (mOptions.currentMonitor) {
        int currentScreen = qApp->desktop()->screenNumber(cursorPosition);

        geometry = qApp->desktop()->screen(currentScreen)->geometry();
        cursorPosition = cursorPosition - geometry.topLeft();
    } else {
        int top = 0;

        for (QScreen *screen : QGuiApplication::screens()) {
            auto screenRect = screen->geometry();

            if (screenRect.top() < 0) {
                top += screenRect.top() * -1;
            }

            if (screenRect.left() < 0) {
                cursorPosition.setX(cursorPosition.x() + screenRect.width()); //= localCursorPos + screenRect.normalized().topLeft();
            }

            geometry = geometry.united(screenRect);
        }

        cursorPosition.setY(cursorPosition.y() + top);
    }

    mPixmap = QApplication::primaryScreen()->grabWindow(QApplication::desktop()->winId(), geometry.x(), geometry.y(), geometry.width(), geometry.height());
    mPixmap.setDevicePixelRatio(QApplication::desktop()->devicePixelRatio());

    if (mOptions.cursor && !mPixmap.isNull()) {
        QPainter painter(&mPixmap);
        auto cursorInfo = os::cursor();
        auto cursorPixmap = cursorInfo.first;
        cursorPixmap.setDevicePixelRatio(QApplication::desktop()->devicePixelRatio()); 

#if 0 // Debug cursor position helper
        painter.setBrush(QBrush(Qt::darkRed));
        painter.setPen(QPen(QBrush(Qt::red), 5));
        QRectF rect;
        rect.setSize(QSizeF(100, 100));
        rect.moveCenter(cursorPosition);
        painter.drawRoundRect(rect, rect.size().height()*2, rect.size().height()*2);
#endif

        painter.drawPixmap(cursorPosition-cursorInfo.second, cursorPixmap);
    }
}

const QString Screenshot::newFileName() const
{
    if (!mOptions.directory.exists()) {
        mOptions.directory.mkpath(mOptions.directory.path());
    }

    QString naming = Screenshot::getName(mOptions.namingOptions, mOptions.prefix, mOptions.directory);
    QString path   = QDir::toNativeSeparators(mOptions.directory.path());

    // Cleanup
    if (path.at(path.size() - 1) != QDir::separator() && !path.isEmpty()) {
        path.append(QDir::separator());
    }

    QString fileName;
    fileName.append(path);
    fileName.append(naming);

    return fileName;
}

void Screenshot::selectedArea()
{
    grabDesktop();

    if (mPixmap.isNull()) {
        return;
    }

    AreaDialog selector(this);
    int result = selector.exec();

    if (result == QDialog::Accepted) {
        mPixmap = mPixmap.copy(selector.resultRect());
    } else {
        mPixmap = QPixmap();
    }
}

void Screenshot::selectedWindow()
{
    WindowPicker *windowPicker = new WindowPicker;
    mPixmapDelay = true;

    connect(windowPicker, SIGNAL(pixmap(QPixmap)), this, SLOT(setPixmap(QPixmap)));
}

bool Screenshot::unloadPixmap()
{
    if (mUnloaded) {
        return true;
    }

    // Unloading the pixmap to reduce memory usage during previews
    mUnloadFilename = QString("%1/.screenshot.%2%3").arg(mOptions.directory.path()).arg(qrand() * qrand() + QDateTime::currentDateTime().toTime_t()).arg(extension());
    mUnloaded       = mPixmap.save(mUnloadFilename, 0, mOptions.quality);

    if (mUnloaded) {
        mPixmap = QPixmap();
    }

    return mUnloaded;
}

void Screenshot::wholeScreen()
{
    grabDesktop();
}
