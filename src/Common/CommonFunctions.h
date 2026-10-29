/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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

#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H

#include <QImage>
#include <QString>

#include <HdrCreation/fusionoperator.h>
#include <HdrWizard/HdrCreationItem.h>
#include <Libpfs/colorspace/hsl.h>
#include <Libpfs/utils/minmax.h>

void computeAutolevels(const QImage *data, const float threshold, float &minL,
                       float &maxL, float &gammaL);

struct ConvertToQRgb {
    float gamma;
    explicit ConvertToQRgb(float gamma = 1.0f);
    void operator()(float r, float g, float b, QRgb &rgb) const;
};

struct LoadFile {
    explicit LoadFile(bool fromFITS = false) : m_datamax(0.f), m_datamin(0.f) {
        m_fromFITS = fromFITS;
    }
    void operator()(HdrCreationItem &currentItem);
    float normalize(float);
    float m_datamax;
    float m_datamin;
    bool m_fromFITS;
};

struct SaveFile {
    int m_mode;
    float m_minLum;
    float m_maxLum;
    bool m_deflateCompression;
    SaveFile(int mode, float minLum = 0.0f, float maxLum = 1.0f,
             bool deflateCompression = true);
    void operator()(HdrCreationItem &currentItem);
};

struct RefreshPreview {
    void operator()(HdrCreationItem &currentItem);
};

QString getQString(libhdr::fusion::FusionOperator fo);
QString getQString(libhdr::fusion::WeightFunctionType wf);
QString getQString(libhdr::fusion::ResponseCurveType rf);
#endif
