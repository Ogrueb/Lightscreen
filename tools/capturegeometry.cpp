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
#include <tools/capturegeometry.h>

#include <cmath>

namespace CaptureGeometry {

QRect deviceRect(const Screen &screen)
{
    QSize size = screen.deviceSize;

    if (size.isEmpty()) {
        size = QSize(qRound(screen.geometry.width() * screen.ratio), qRound(screen.geometry.height() * screen.ratio));
    }

    return QRect(screen.geometry.topLeft(), size);
}

QRect deviceUnion(const QList<Screen> &screens)
{
    QRect result;

    for (const Screen &screen : screens) {
        result = result.united(deviceRect(screen));
    }

    return result;
}

int screenAt(const QList<Screen> &screens, const QPoint &logical)
{
    for (int i = 0; i < screens.size(); ++i) {
        if (screens.at(i).geometry.contains(logical)) {
            return i;
        }
    }

    return -1;
}

QPoint logicalToDevice(const QList<Screen> &screens, const QPoint &logical)
{
    const int index = screenAt(screens, logical);

    if (index < 0) {
        return logical;
    }

    const Screen &screen = screens.at(index);
    const QPoint origin = screen.geometry.topLeft();
    const QPoint offset = logical - origin;

    return origin + QPoint(int(std::floor(offset.x() * screen.ratio)), int(std::floor(offset.y() * screen.ratio)));
}

int dominantScreen(const QList<Screen> &screens, const QRect &device)
{
    int result = -1;
    qint64 largest = 0;

    for (int i = 0; i < screens.size(); ++i) {
        const QRect part = deviceRect(screens.at(i)).intersected(device);
        const qint64 area = qint64(part.width()) * part.height();

        if (area > largest) {
            largest = area;
            result = i;
        }
    }

    return result;
}

QRect windowGeometry(const Screen &screen, const QRect &device)
{
    const QPointF origin = screen.geometry.topLeft();
    const QPointF topLeft = origin + (QPointF(device.topLeft()) - origin) / screen.ratio;
    const QPointF bottomRight = origin + (QPointF(device.topLeft() + QPoint(device.width(), device.height())) - origin) / screen.ratio;

    const QPoint first(int(std::floor(topLeft.x())), int(std::floor(topLeft.y())));
    const QPoint last(int(std::ceil(bottomRight.x())), int(std::ceil(bottomRight.y())));

    return QRect(first, QSize(last.x() - first.x(), last.y() - first.y()));
}

QRect selectionToDevice(const QRect &selection, qreal ratio, const QSize &bounds)
{
    const int left = int(std::floor(selection.left() * ratio));
    const int top = int(std::floor(selection.top() * ratio));
    const int right = int(std::ceil((selection.left() + selection.width()) * ratio));
    const int bottom = int(std::ceil((selection.top() + selection.height()) * ratio));

    QRect result(left, top, right - left, bottom - top);

    if (!bounds.isEmpty() && result.width() > 0 && result.height() > 0) {
        result = result.intersected(QRect(QPoint(0, 0), bounds));
    }

    return result;
}

} // namespace CaptureGeometry
