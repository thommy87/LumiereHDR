/**
 * @brief
 *
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 *               2006,2007 Giuseppe Rota
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
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 * $Id: luminancerange_widget.cpp,v 1.2 2005/09/02 13:10:35 rafm Exp $
 */

#include "LuminanceRangeWidget.h"

#include <QCursor>
#include <QPainter>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <QMouseEvent>
#include <cassert>
#include <vector>
#include <cmath>
#include <algorithm>

#include <Libpfs/array2d.h>

#include "Histogram.h"

static const float exposureStep = 0.25f;
static const float shrinkStep = 0.1f;
static const float minWindowSize = 0.1f;

static const int dragZoneMargin = 5;  // How many pizels from the range window
                                      // border should dragging be activated

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

LuminanceRangeWidget::LuminanceRangeWidget(QWidget *parent)
    : QFrame(parent),
      dragMode(DRAG_NO),
      showVP(false),
      valuePointer(0.f),
      histogram(nullptr),
      histogramImage(nullptr)

{
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    setMouseTracking(true);

    minValue = -6;
    maxValue = 8;
    windowMin = 0;
    windowMax = 2;

    mouseDragStart = -1;
    dragShift = 0;
}

LuminanceRangeWidget::~LuminanceRangeWidget() { delete histogram; }

QSize LuminanceRangeWidget::sizeHint() const { return QSize(300, 22); }

#define getWindowX(x)                                                       \
    ((int)((x - minValue) / (maxValue - minValue) * (float)fRect.width()) + \
     fRect.left())

void LuminanceRangeWidget::paintEvent(QPaintEvent * /*pe */) {
    QPainter p(this);

    QRect fRect = getPaintRect();

    if (fRect.width() < 50)  // Does not make sense to paint anything
        return;
    // p.begin(this);
    // Paint range window
    {
        int x1, x2;
        x1 = getWindowX(draggedMin());
        x2 = getWindowX(draggedMax());
        QColor selectionColor = mouseDragStart == DRAGNOTSTARTED
                                    ? QColor(0, 100, 255)
                                    : QColor(0, 150, 255);
        p.fillRect(x1, fRect.top(), x2 - x1, fRect.height(),
                   QBrush(selectionColor));
    }

    // Paint histogram
    if (histogramImage != nullptr) {
        if (histogram == nullptr || histogram->getBins() != fRect.width()) {
            delete histogram;
            // Build histogram from at least 5000 pixels
            int accuracy =
                histogramImage->getRows() * histogramImage->getCols() / 5000;
            //       int accuracy =1;
            if (accuracy < 1) accuracy = 1;
            histogram = new Histogram(fRect.width(), accuracy);
            histogram->computeLog(histogramImage, minValue, maxValue);
        }

        float maxP = histogram->getMaxP();
        int i = 0;
        p.setPen(Qt::green);
        for (int x = fRect.left(); i < histogram->getBins(); x++, i++) {
            if (histogram->getP(i) > 0) {
                int barSize =
                    (int)((float)fRect.height() * histogram->getP(i) / maxP);
                p.drawLine(x, fRect.bottom(), x, fRect.bottom() - barSize);
            }
        }
    }

    // Paint scale
    QFont labelFont(QStringLiteral("SansSerif"), 8);
    p.setFont(labelFont);
    p.setPen(Qt::black);
    QRect textBounding = p.boundingRect(
        fRect, Qt::AlignHCenter | Qt::AlignBottom, QStringLiteral("-8"));
    for (float x = ceil(minValue); x <= floor(maxValue); x++) {
        int rx = getWindowX(x);
        p.drawLine(rx, fRect.top(), rx, textBounding.top());
        char str[15];
        snprintf(str, sizeof(str), "%g", x);
        p.drawText(rx - 20, textBounding.top(), 40, textBounding.height(),
                   Qt::AlignHCenter | Qt::AlignBottom, str);
    }

    //   Paint value pointer
    if (showVP) {
        int x = getWindowX(valuePointer);
        if (fRect.contains(x, fRect.y())) {
            p.setPen(Qt::yellow);
            p.drawLine(x, fRect.top(), x, fRect.bottom());
        }
    }
}
float LuminanceRangeWidget::draggedMin() {
    if (mouseDragStart == DRAGNOTSTARTED) return windowMin;
    if (dragMode == DRAG_MIN) {
        float draggedPos = windowMin + dragShift;
        return min(draggedPos, windowMax - minWindowSize);
    } else if (dragMode == DRAG_MINMAX) {
        return windowMin + dragShift;
    }
    return windowMin;
}

float LuminanceRangeWidget::draggedMax() {
    if (mouseDragStart == DRAGNOTSTARTED) return windowMax;
    if (dragMode == DRAG_MAX) {
        float draggedPos = windowMax + dragShift;
        return max(draggedPos, windowMin + minWindowSize);
    } else if (dragMode == DRAG_MINMAX) {
        return windowMax + dragShift;
    }
    return windowMax;
}

void LuminanceRangeWidget::mousePressEvent(QMouseEvent *me) {
    if (dragMode == DRAG_NO) return;

    mouseDragStart = me->x();
    dragShift = 0;
    update();
}

void LuminanceRangeWidget::mouseReleaseEvent(QMouseEvent *) {
    float newWindowMin = draggedMin();
    float newWindowMax = draggedMax();
    mouseDragStart = DRAGNOTSTARTED;
    windowMin = newWindowMin;
    windowMax = newWindowMax;
    dragShift = 0;
    update();
    emit updateRangeWindow();
}

void LuminanceRangeWidget::mouseMoveEvent(QMouseEvent *me) {
    //  printf( "MouseButton: %d\n", me->button() );

    if ((me->buttons() & Qt::LeftButton) != 0 && dragMode != DRAG_NO) {
        if (mouseDragStart != DRAGNOTSTARTED) {
            QRect fRect = getPaintRect();

            int windowCordShift = me->x() - mouseDragStart;
            dragShift = (float)windowCordShift / (float)fRect.width() *
                        (maxValue - minValue);
            update();
        }

    } else {
        QRect fRect = rect();
        int winBegPos = getWindowX(windowMin);
        int winEndPos = getWindowX(windowMax);
        if (abs(me->x() - winBegPos) < dragZoneMargin) {
            setCursor(QCursor(QCursor(Qt::SplitHCursor)));
            dragMode = DRAG_MIN;
        } else if (abs(me->x() - winEndPos) < dragZoneMargin) {
            setCursor(QCursor(QCursor(Qt::SplitHCursor)));
            dragMode = DRAG_MAX;
        } else if (me->x() > winBegPos && me->x() < winEndPos) {
            setCursor(QCursor(Qt::PointingHandCursor));
            dragMode = DRAG_MINMAX;
        } else {
            unsetCursor();
            dragMode = DRAG_NO;
        }
    }
}

void LuminanceRangeWidget::increaseExposure() {
    windowMin -= exposureStep;
    windowMax -= exposureStep;
    update();
    emit updateRangeWindow();
}

void LuminanceRangeWidget::decreaseExposure() {
    windowMin += exposureStep;
    windowMax += exposureStep;
    update();
    emit updateRangeWindow();
}

void LuminanceRangeWidget::extendRange() {
    if (windowMax - windowMin > 10) return;
    windowMin -= shrinkStep;
    windowMax += shrinkStep;
    update();
    emit updateRangeWindow();
}

void LuminanceRangeWidget::shrinkRange() {
    if (windowMax - windowMin < 0.19) return;
    windowMin += shrinkStep;
    windowMax -= shrinkStep;
    update();
    emit updateRangeWindow();
}

void LuminanceRangeWidget::setHistogramImage(const pfs::Array2Df *image) {
    histogramImage = image;
    delete histogram;
    histogram = nullptr;
    update();
}

void LuminanceRangeWidget::fitToDynamicRange() {
    if (histogramImage != nullptr) {
        // the brightest and darkest few pixels (lamps, sensor noise) would
        // decide the window and leave the picture far too dark, so take
        // the 0.1 and 99.5 percent points instead
        std::vector<float> values;
        const size_t size = histogramImage->getRows() * histogramImage->getCols();
        const size_t stride = std::max<size_t>(1, size / 200000);
        values.reserve(size / stride + 1);
        for (size_t i = 0; i < size; i += stride) {
            const float v = (*histogramImage)(i);
            if (std::isfinite(v)) values.push_back(v);
        }
        float min = 0.000001f, max = 1.f;
        if (!values.empty()) {
            const size_t lo = values.size() / 1000;
            const size_t hi = values.size() - 1 - values.size() / 200;
            std::nth_element(values.begin(), values.begin() + lo, values.end());
            min = values[lo];
            std::nth_element(values.begin(), values.begin() + hi, values.end());
            max = values[hi];
        }
        if (max <= 0.000001f) max = 1.f;

        if (min <= 0.000001f)
            min = 0.000001f;  // If data contains negative values

        windowMin = log10(min);
        windowMax = log10(max);

        if (windowMax - windowMin < 0.5) {  // Window too small
            float m = (windowMin + windowMax) / 2.f;
            windowMax = m + 0.25;
            windowMin = m - 0.25;
        }
        update();
        emit updateRangeWindow();
    }
}

void LuminanceRangeWidget::lowDynamicRange() {
    windowMin = 1.0f;
    windowMax = 3.0f;

    update();
    emit updateRangeWindow();
}

QRect LuminanceRangeWidget::getPaintRect() const {
    QRect fRect = frameRect();
    fRect.setLeft(fRect.left() + 1);
    fRect.setTop(fRect.top() + 1);
    fRect.setBottom(fRect.bottom() - 1);
    fRect.setRight(fRect.right() - 1);
    return fRect;
}

void LuminanceRangeWidget::showValuePointer(float value) {
    QRect fRect = getPaintRect();
    int oldx = showVP ? getWindowX(valuePointer) : -1;

    valuePointer = value;
    showVP = true;

    int newx = getWindowX(valuePointer);
    if (oldx == -1) oldx = newx;

    QRect updateRect(min(oldx, newx), fRect.top(),
                     max(oldx, newx) - min(oldx, newx) + 1, fRect.height());

    update(updateRect);
}

void LuminanceRangeWidget::setRangeWindowMinMax(float min, float max) {
    assert(min < max);
    windowMin = min;
    windowMax = max;
    update();
    emit updateRangeWindow();
}

void LuminanceRangeWidget::hideValuePointer() {
    showVP = false;
    update();
}
