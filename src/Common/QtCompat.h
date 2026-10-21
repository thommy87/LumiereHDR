/*
 * This file is a part of LumiereHDR package.
 * ----------------------------------------------------------------------
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * ----------------------------------------------------------------------
 */

// Small helpers for APIs that differ between Qt 5.15 and Qt 6.

#ifndef LHDR_QTCOMPAT_H
#define LHDR_QTCOMPAT_H

#include <QLabel>
#include <QPixmap>

//! QLabel::pixmap() returns a pointer in Qt 5 and a value in Qt 6
inline QPixmap labelPixmap(const QLabel *label) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return label->pixmap();
#else
    return label->pixmap(Qt::ReturnByValue);
#endif
}

#endif  // LHDR_QTCOMPAT_H
