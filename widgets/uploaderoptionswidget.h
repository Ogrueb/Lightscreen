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
#ifndef UPLOADEROPTIONSWIDGET_H
#define UPLOADEROPTIONSWIDGET_H

#include <QPointer>
#include <QScopedPointer>
#include <QVariantHash>
#include <QWidget>
#include "ui_uploaderoptionswidget.h"

class CustomUploader;
class QTemporaryFile;

// The custom uploader's settings ("upload/custom/*") and a "Test upload" button that posts a 1x1 PNG with the
// values as they are in the form, saved or not.
class UploaderOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UploaderOptionsWidget(QWidget *parent = 0);
    ~UploaderOptionsWidget();

    QVariantHash uploaderSettings() const;
    void setUploaderSettings(const QVariantHash &settings);

public slots:
    void testUpload();

private:
    void testFinished(bool success, const QString &text);

    Ui::UploaderOptions ui;
    QPointer<CustomUploader> mTestUploader;
    QScopedPointer<QTemporaryFile> mTestFile;
};

#endif // UPLOADEROPTIONSWIDGET_H
