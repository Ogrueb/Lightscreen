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
#include <QDir>
#include <QImage>
#include <QTemporaryFile>

#include <widgets/uploaderoptionswidget.h>
#include <tools/uploader/customuploader.h>
#include <tools/uploader/uploader.h>

#include <tools/screenshotmanager.h>

UploaderOptionsWidget::UploaderOptionsWidget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);

    ui.fieldEdit->setPlaceholderText(CustomUploader::defaultField());
    ui.linkPathEdit->setPlaceholderText(CustomUploader::defaultLinkPath());
    ui.progressIndicatorBar->setVisible(false);
    ui.cancelButton->setVisible(false);
    ui.testButton->setEnabled(false);

    connect(ui.testButton, &QPushButton::clicked, this, &UploaderOptionsWidget::testUpload);

    connect(ui.cancelButton, &QPushButton::clicked, this, [this] {
        if (mTestUploader) {
            mTestUploader->cancel();
        }
    });

    connect(ui.urlEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        ui.testButton->setEnabled(!mTestUploader && !text.trimmed().isEmpty());
        ui.resultLabel->clear();
    });
}

UploaderOptionsWidget::~UploaderOptionsWidget()
{
    if (mTestUploader) {
        mTestUploader->disconnect(this);
        mTestUploader->cancel();
    }
}

QVariantHash UploaderOptionsWidget::uploaderSettings() const
{
    QVariantHash settings;
    settings["url"]        = ui.urlEdit->text().trimmed();
    settings["field"]      = ui.fieldEdit->text().trimmed();
    settings["headers"]    = ui.headersEdit->toPlainText().trimmed();
    settings["linkPath"]   = ui.linkPathEdit->text().trimmed();
    settings["deletePath"] = ui.deletePathEdit->text().trimmed();
    return settings;
}

void UploaderOptionsWidget::setUploaderSettings(const QVariantHash &settings)
{
    ui.urlEdit->setText(settings.value("url").toString());
    ui.fieldEdit->setText(settings.value("field", CustomUploader::defaultField()).toString());
    ui.headersEdit->setPlainText(settings.value("headers").toString());
    ui.linkPathEdit->setText(settings.value("linkPath", CustomUploader::defaultLinkPath()).toString());
    ui.deletePathEdit->setText(settings.value("deletePath").toString());
}

void UploaderOptionsWidget::testUpload()
{
    if (mTestUploader) {
        return;
    }

    mTestFile.reset(new QTemporaryFile(QDir::tempPath() + "/lightscreen-test-XXXXXX.png"));

    QImage image(1, 1, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    if (!mTestFile->open() || !image.save(mTestFile.data(), "PNG")) {
        testFinished(false, tr("Could not write the test image"));
        return;
    }

    mTestFile->close();

    ScreenshotManager *manager = ScreenshotManager::instance();
    mTestUploader = new CustomUploader(manager->settings(), manager->uploader()->network(), this);
    mTestUploader->setUploaderSettings(uploaderSettings());

    connect(mTestUploader, &ImageUploader::uploaded, this, [this](const QString &fileName, const QString &url) {
        Q_UNUSED(fileName);
        testFinished(true, url);
    });

    connect(mTestUploader, &ImageUploader::error, this, [this](ImageUploader::Error errorCode, const QString &errorString) {
        testFinished(false, errorCode == ImageUploader::CancelError ? tr("Cancelled") : errorString);
    });

    ui.testButton->setEnabled(false);
    ui.progressIndicatorBar->setVisible(true);
    ui.cancelButton->setVisible(true);
    ui.resultLabel->clear();

    mTestUploader->upload(mTestFile->fileName());
}

void UploaderOptionsWidget::testFinished(bool success, const QString &text)
{
    if (mTestUploader) {
        mTestUploader->deleteLater();
        mTestUploader = nullptr;
    }

    ui.testButton->setEnabled(!ui.urlEdit->text().trimmed().isEmpty());
    ui.progressIndicatorBar->setVisible(false);
    ui.cancelButton->setVisible(false);

    if (success) {
        ui.resultLabel->setStyleSheet("color: green;");
        ui.resultLabel->setText(QString("<a href=\"%1\">%2</a>").arg(text.toHtmlEscaped(), text.toHtmlEscaped()));
    } else {
        ui.resultLabel->setStyleSheet("color: red;");
        ui.resultLabel->setText(text.toHtmlEscaped());
    }
}
