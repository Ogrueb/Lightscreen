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
#include <QJsonObject>
#include <QInputDialog>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QMessageBox>

#include <widgets/pomfoptionswidget.h>
#include <tools/uploader/uploader.h>
#include <tools/uploader/pomfuploader.h>

#include <tools/screenshotmanager.h>
#include <tools/os.h>

PomfOptionsWidget::PomfOptionsWidget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);

    ui.progressIndicatorBar->setVisible(false);
    ui.cancelButton->setVisible(false);

    connect(ui.verifyButton, &QPushButton::clicked, this, [&]() {
        ui.verifyButton->setEnabled(false);
        ui.progressIndicatorBar->setVisible(true);
        ui.cancelButton->setVisible(true);
        disconnect(ui.cancelButton);
        ui.cancelButton->disconnect();

        QPointer<QWidget> guard(parentWidget());
        QPointer<QNetworkReply> reply = PomfUploader::verify(ScreenshotManager::instance()->uploader()->network(), ui.pomfUrlComboBox->currentText(), [&, guard](bool result) {
            if (guard.isNull()) return;

            ui.verifyButton->setEnabled(true);
            ui.progressIndicatorBar->setVisible(false);

            if (result) {
                ui.verifyButton->setText(tr("Valid uploader!"));
                ui.verifyButton->setStyleSheet("color: green;");
                ui.verifyButton->setIcon(os::icon("yes"));
            } else if (ui.cancelButton->isVisible() == true) { // Not cancelled
                ui.verifyButton->setStyleSheet("color: red;");
                ui.verifyButton->setIcon(os::icon("no"));
                ui.verifyButton->setText(tr("Invalid uploader :("));
            }

            ui.cancelButton->setVisible(false);
        });

        if (reply) {
            connect(ui.cancelButton, &QPushButton::clicked, [&, reply] {
                if (reply) {
                    ui.cancelButton->setVisible(false);
                    reply->abort();
                }
            });
        }
    });

    connect(ui.pomfUrlComboBox, &QComboBox::currentTextChanged, [&](const QString &text) {
        bool validUrl = false;

        if (!text.isEmpty() && (text.startsWith("http://") || text.startsWith("https://"))) { // TODO: Something a bit more complex
            validUrl = true;
        }

        ui.verifyButton->setEnabled(validUrl);

        if (ui.verifyButton->styleSheet().count() > 0) {
            ui.verifyButton->setStyleSheet("");
            ui.verifyButton->setIcon(QIcon());
            ui.verifyButton->setText(tr("Verify"));
        }
    });
}
