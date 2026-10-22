/*
 * Copyright (C) 2026 Lightscreen Rd contributors
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
#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <QObject>
#include <QStringList>

class QProcess;
class QSettings;

// The PNG optimization queue (bug 10): files run one at a time, in the order they were added, through oxipng or
// optipng. Each tool writes to a temporary file that replaces the original only on success, so an interrupted run
// leaves the original intact. The queue is kept in a file under the data directory, so files still pending when the
// app stops are optimized after the next start (resume()). optimizationDone fires exactly once per added file.
class Optimizer : public QObject
{
    Q_OBJECT

public:
    Optimizer(QSettings *settings, const QString &dataPath, QObject *parent = nullptr);
    ~Optimizer();

    // The supported tools, in the order the automatic choice tries them.
    static QStringList toolNames();
    // The tool's executable: on PATH on Linux, next to the application on Windows. Empty when it is not found.
    static QString program(const QString &tool);
    // The tool that runs for a choice ("options/optimizer"): the chosen one if found, else the other one if found;
    // with no valid choice, the first one found. Empty when none is found.
    static QString resolveTool(const QString &choice);
    static QString temporaryFileName(const QString &fileName);

    void add(const QString &fileName);
    int pending() const;
    QString pendingFileName() const;
    void resume();

signals:
    void optimizationDone(const QString &fileName);
    void pendingChanged(int pending);

private:
    void finish();
    void next();
    void save();

    QSettings *mSettings;
    QString mPendingFileName;
    QStringList mQueue; // the first entry is the one running, if mProcess is set
    QProcess *mProcess;
};

#endif // OPTIMIZER_H
