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
#include <dialogs/namingdialog.h>
#include <tools/screenshot.h>
#include <tools/os.h>
#include <tools/screenshotmanager.h>
#include <tools/settings.h>

#include <QDateTime>
#include <QDesktopServices>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSettings>
#include <QUrl>

NamingDialog::NamingDialog(Screenshot::Naming naming, QWidget *parent) :
    QDialog(parent)
{
    ui.setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);

    // No character a file name cannot hold. A validator on the box's text field, which receives the typing (an event
    // filter on the box itself never saw a key), also covers pasting.
#ifdef Q_OS_WIN
    const QRegularExpression dateFormat("[^?:\\\\/*\"<>|]*");
#else
    const QRegularExpression dateFormat("[^/]*");
#endif
    ui.dateFormatComboBox->lineEdit()->setValidator(new QRegularExpressionValidator(dateFormat, ui.dateFormatComboBox));

    // Settings
    auto settings = ScreenshotManager::instance()->settings();
    ui.flipNamingCheckBox->setChecked(Settings::value(settings, "options/flip").toBool());

    ui.dateFormatComboBox->setCurrentIndex(
        ui.dateFormatComboBox->findText(Settings::value(settings, "options/naming/dateFormat").toString())
    );

    if (ui.dateFormatComboBox->currentIndex() == -1) {
        ui.dateFormatComboBox->addItem(Settings::value(settings, "options/naming/dateFormat").toString());
        ui.dateFormatComboBox->setCurrentIndex(ui.dateFormatComboBox->count() - 1);
    }

    updateDatePreview();

    ui.leadingZerosSpinBox->setValue(Settings::value(settings, "options/naming/leadingZeros").toInt());

    // Signals/Slots
    connect(ui.buttonBox    , &QDialogButtonBox::accepted, this, &NamingDialog::saveSettings);
    connect(ui.dateHelpLabel, &QLabel::linkActivated, this, [](const QUrl &url) {
        QDesktopServices::openUrl(QUrl(url));
    });
    connect(ui.dateFormatComboBox, &QComboBox::currentTextChanged, this, &NamingDialog::updateDatePreview);

    // Stack & window size adjustments
    ui.stack->setCurrentIndex((int)naming);
    ui.stack->currentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    resize(minimumSizeHint());
}

void NamingDialog::updateDatePreview() {
    ui.datePreviewLabel->setText(tr("<u>Preview</u>: ") + QDateTime::currentDateTime().toString(ui.dateFormatComboBox->currentText()));
}

void NamingDialog::saveSettings() {
    auto settings = ScreenshotManager::instance()->settings();
    settings->setValue("options/flip"               , ui.flipNamingCheckBox->isChecked());
    settings->setValue("options/naming/dateFormat"  , ui.dateFormatComboBox->currentText());
    settings->setValue("options/naming/leadingZeros", ui.leadingZerosSpinBox->value());
}

