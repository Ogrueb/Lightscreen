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
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QStringList>
#include <QVariant>

class QSettings;

// One default for every option the app reads (S06), so the options dialog, the main window and the other readers
// agree on what a missing key means. Keys are full paths ("options/tray"). Settings that are state rather than
// options (window positions, lastScreenshot, lastUpdateCheck) and the uploader's own upload/custom/* keys, whose
// defaults live in CustomUploader, are not listed; neither is options/optimizer, whose absence means "automatic".
namespace Settings {

// The default for a key; an invalid QVariant for a key that is not listed. Two defaults depend on portable mode:
// file/target (a relative "Screenshots" folder instead of one under Documents) and options/history (off).
QVariant defaultValue(const QString &key, bool portable = false);

// settings->value(key) with the key's default. The key is relative to the settings' current group, as for
// QSettings::value; the default is looked up by the full key.
QVariant value(const QSettings *settings, const QString &key, bool portable = false);

// Every listed key.
QStringList keys();

} // namespace Settings

#endif // SETTINGS_H
