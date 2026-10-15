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
#ifndef UPLOADDIALOG_H
#define UPLOADDIALOG_H

#include <QDialog>
#include <QItemSelection>

namespace Ui {
class HistoryDialog;
}

class QSqlTableModel;
class QSortFilterProxyModel;
class HistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistoryDialog(QWidget *parent = 0);
    ~HistoryDialog();

private slots:
    void clear();
    void contextMenu(const QPoint &point);
    void copy();
    void deleteImage();
    void location();
    void removeHistoryEntry();
    void refresh();
    void openUrl(const QModelIndex &index);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void upload();
    void uploadProgress(int progress);
    void init();

protected:
    bool eventFilter(QObject *object, QEvent *event);
    bool event(QEvent *event);

private:
    Ui::HistoryDialog *ui;
    QSqlTableModel *mModel;
    QSortFilterProxyModel *mFilterModel;
    QString     mSelectedScreenshot;
    QModelIndex mContextIndex;
};

#endif // UPLOADDIALOG_H
