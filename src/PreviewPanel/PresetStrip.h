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
class QHBoxLayout;
class QToolButton;

//! \brief Row of tone mapping presets under the image: a small preview of
//! the current HDR image with each preset, its name underneath.
class PresetStrip : public QScrollArea {
    Q_OBJECT
   public:
    explicit PresetStrip(QWidget *parent = nullptr);

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
    QHBoxLayout *m_layout;
};

#endif
