/*
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
#include <tools/optimizer.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

#ifdef Q_OS_WIN
    #include <windows.h>
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

// Both at their default level (2). oxipng reads "-out" as "-o ut", so it needs the double dash.
QStringList arguments(const QString &tool, const QString &output, const QString &input)
{
    if (tool == QLatin1String("oxipng")) {
        return { "-q", "--out", output, input };
    }

    return { "-quiet", "-out", output, input };
}

} // namespace

Optimizer::Optimizer(QSettings *settings, const QString &dataPath, QObject *parent) :
    QObject(parent), mSettings(settings), mProcess(nullptr)
{
    mPendingFileName = QDir(dataPath).filePath("optimize-pending.txt");
}

Optimizer::~Optimizer()
{
    // Whatever is running stays in the pending file and runs again after the next start.
    if (mProcess) {
        disconnect(mProcess, nullptr, this, nullptr);
        mProcess->kill();
        mProcess->waitForFinished(3000);
        QFile::remove(temporaryFileName(mQueue.first()));
    }
}

QStringList Optimizer::toolNames()
{
    return { "oxipng", "optipng" };
}

QString Optimizer::program(const QString &tool)
{
#ifdef Q_OS_WIN
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath(tool + ".exe");
    return QFile::exists(path) ? path : QString();
#else
    return QStandardPaths::findExecutable(tool);
#endif
}

QString Optimizer::resolveTool(const QString &choice)
{
    QStringList candidates = toolNames();

    if (candidates.contains(choice)) {
        candidates.removeAll(choice);
        candidates.prepend(choice);
    }

    for (const QString &tool : candidates) {
        if (!program(tool).isEmpty()) {
            return tool;
        }
    }

    return QString();
}

QString Optimizer::temporaryFileName(const QString &fileName)
{
    return fileName + ".optimize.tmp";
}

void Optimizer::add(const QString &fileName)
{
    mQueue.append(fileName);
    save();
    emit pendingChanged(mQueue.size());
    next();
}

int Optimizer::pending() const
{
    return mQueue.size();
}

QString Optimizer::pendingFileName() const
{
    return mPendingFileName;
}

// Queues the files left in the pending file by an earlier run.
void Optimizer::resume()
{
    QFile file(mPendingFileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    const QStringList files = QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
    file.close();

    if (files.isEmpty()) {
        return;
    }

    mQueue.append(files);
    save();
    emit pendingChanged(mQueue.size());
    next();
}

// Ends the first entry: removes it from the queue and reports it.
void Optimizer::finish()
{
    const QString fileName = mQueue.takeFirst();
    save();
    emit pendingChanged(mQueue.size());
    emit optimizationDone(fileName);
}

void Optimizer::next()
{
    // A file that cannot run (no tool, file gone, process not started) finishes at once and the next one is tried.
    while (!mProcess && !mQueue.isEmpty()) {
        const QString original = mQueue.first();
        const QString tool = resolveTool(mSettings ? mSettings->value("options/optimizer").toString() : QString());

        if (tool.isEmpty() || !QFile::exists(original)) {
            finish();
            continue;
        }

        // A temporary file left by an interrupted run would make optipng refuse to write.
        const QString temporary = temporaryFileName(original);
        QFile::remove(temporary);

        mProcess = new QProcess(this);

        connect(mProcess, &QProcess::finished, this, [this, original, temporary](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitStatus != QProcess::NormalExit || exitCode != 0 || !QFile::exists(temporary) || !replaceFile(temporary, original)) {
                QFile::remove(temporary);
            }

            mProcess->deleteLater();
            mProcess = nullptr;
            finish();
            next();
        });

        mProcess->start(program(tool), arguments(tool, temporary, original));

        // finished is not emitted when the process cannot be started.
        if (!mProcess->waitForStarted(-1)) {
            mProcess->deleteLater();
            mProcess = nullptr;
            finish();
        }
    }
}

void Optimizer::save()
{
    if (mQueue.isEmpty()) {
        QFile::remove(mPendingFileName);
        return;
    }

    QDir().mkpath(QFileInfo(mPendingFileName).path());

    QSaveFile file(mPendingFileName);

    // Written with "\n" on every platform; resume() reads it in text mode, which accepts "\r\n" too.
    if (file.open(QIODevice::WriteOnly)) {
        file.write(mQueue.join('\n').toUtf8() + '\n');
        file.commit();
    }
}
