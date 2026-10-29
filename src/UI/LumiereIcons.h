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

#ifndef LUMIEREICONS_H
#define LUMIEREICONS_H

#include <QIcon>
#include <QString>

//! \brief The line icons of the interface (ui/icons/<name>.svg), drawn in
//! the text colour of the current palette, so they suit light and dark.
//! Icons made before a change of colour scheme must be fetched again.
namespace LumiereIcons {
QIcon icon(const QString &name);
}

#endif
