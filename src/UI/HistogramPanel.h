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

#ifndef HISTOGRAMPANEL_H
#define HISTOGRAMPANEL_H

#include <QImage>
#include <QWidget>

class HistogramLDR;
class QLabel;

//! \brief Histogram of the shown image, with the share of clipped pixels.
class HistogramPanel : public QWidget {
    Q_OBJECT
   public:
    explicit HistogramPanel(QWidget *parent = nullptr);
    //! a null image clears the panel
    void setImage(const QImage &image);

   private:
    HistogramLDR *m_histogram;
    QLabel *m_highlights;
    QLabel *m_shadows;
};

#endif
