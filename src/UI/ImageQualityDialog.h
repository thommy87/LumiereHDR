/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Franco Comida
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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

#ifndef IMAGEQUALITYDIALOG_H
#define IMAGEQUALITYDIALOG_H

#include <QDialog>
#include <QScopedPointer>

#include "Common/LuminanceOptions.h"

namespace Ui {
class ImgQualityDialog;
}

namespace pfs {
class Frame;
}

class ImageQualityDialog : public QDialog {
    Q_OBJECT

   public:
    ImageQualityDialog(const pfs::Frame *frame, const QString &fmt,
                       int defaultValue = -1, QWidget *parent = 0);
    ~ImageQualityDialog();

    int getQuality(void) const;

    //! a tick box under the quality, e.g. "Ultra HDR" for JPEG, with a line
    //! of explanation; its last choice per \a key is remembered
    void offerHdr(const QString &key, const QString &label,
                  const QString &explanation, bool defaultOn);
    bool hdrChosen() const;

   protected slots:
    void on_getSizeButton_clicked();
    void reset(int);

   protected:
    const pfs::Frame *m_frame;
    QString m_format;

    QScopedPointer<Ui::ImgQualityDialog> m_ui;
    QScopedPointer<LuminanceOptions> m_options;
    class QCheckBox *m_hdr = nullptr;
    QString m_hdrKey;
};

#endif  // IMAGEQUALITYDIALOG_H
