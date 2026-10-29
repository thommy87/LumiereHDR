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

#ifndef PRESETSTRIP_H
#define PRESETSTRIP_H

#include <QList>
#include <QPair>
#include <QScrollArea>

namespace pfs {
class Frame;
}
class QGridLayout;
class QToolButton;

//! \brief The looks (tone mapping presets): a small preview of the current
//! HDR image with each, its name underneath; in a row, or in columns.
class PresetStrip : public QScrollArea {
    Q_OBJECT
   public:
    explicit PresetStrip(QWidget *parent = nullptr);

    //! \a columns side by side, scrolling down; 0 for one scrolling row
    void setColumns(int columns, const QSize &preview);

    //! name and settings file of each preset, in display order
    void setPresets(const QList<QPair<QString, QString>> &presets);
    //! marks the preset that is applied, or none for an empty name
    void setCurrent(const QString &name);

   public Q_SLOTS:
    //! renders every preview again from \a hdr (no copy is kept)
    void updatePreviews(pfs::Frame *hdr);

   signals:
    void presetClicked(const QString &file, const QString &name);

   private:
    QList<QPair<QString, QString>> m_presets;
    QList<QToolButton *> m_buttons;
    QGridLayout *m_layout;
    int m_columns = 0;
    QSize m_preview;
    void placeButtons();
};

#endif
