/*
 * This file is a part of LumiereHDR package.
 * ----------------------------------------------------------------------
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * ----------------------------------------------------------------------
 */

#include "LumiereIcons.h"

#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSvgRenderer>

namespace {
QPixmap render(const QByteArray &svg, const QColor &color, int size,
               qreal dpr) {
    QByteArray data = svg;
    data.replace("currentColor", color.name(QColor::HexRgb).toLatin1());
    QSvgRenderer renderer(data);
    QPixmap pixmap(QSize(size, size) * dpr);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();
    pixmap.setDevicePixelRatio(dpr);
    return pixmap;
}
}  // namespace

QIcon LumiereIcons::icon(const QString &name) {
    QFile file(QStringLiteral(":/ui/icons/%1.svg").arg(name));
    if (!file.open(QIODevice::ReadOnly)) return QIcon();
    const QByteArray svg = file.readAll();

    const QPalette palette = QApplication::palette();
    const QColor normal = palette.color(QPalette::Active, QPalette::ButtonText);
    const QColor disabled =
        palette.color(QPalette::Disabled, QPalette::ButtonText);
    // a checked tool (a mode, a toggled view) in the accent
    const QColor on = palette.color(QPalette::Active, QPalette::Link);

    QIcon icon;
    for (int size : {16, 20, 24, 32}) {
        for (qreal dpr : {1.0, 2.0}) {
            icon.addPixmap(render(svg, normal, size, dpr), QIcon::Normal,
                           QIcon::Off);
            icon.addPixmap(render(svg, on, size, dpr), QIcon::Normal,
                           QIcon::On);
            icon.addPixmap(render(svg, disabled, size, dpr), QIcon::Disabled);
        }
    }
    return icon;
}
