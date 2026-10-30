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
#include <QPainterPath>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <QMouseEvent>
#include <cassert>
#include <vector>
#include <cmath>
#include <algorithm>

#include <Libpfs/array2d.h>
#include <Libpfs/utils/robustrange.h>

#include "Histogram.h"

static const float exposureStep = 0.25f;
static const float shrinkStep = 0.1f;
static const float minWindowSize = 0.1f;

static const int dragZoneMargin = 6;  // How many pizels from the range window
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
    setFrameStyle(QFrame::NoFrame);
    setMinimumHeight(minimumSizeHint().height());
    setMouseTracking(true);

    minValue = -6;
    maxValue = 8;
    windowMin = 0;
    windowMax = 2;

    mouseDragStart = -1;
    dragShift = 0;
}

LuminanceRangeWidget::~LuminanceRangeWidget() { delete histogram; }

QSize LuminanceRangeWidget::sizeHint() const { return QSize(360, 40); }

QSize LuminanceRangeWidget::minimumSizeHint() const { return QSize(200, 40); }

#define getWindowX(x)                                                       \
    ((int)((x - minValue) / (maxValue - minValue) * (float)fRect.width()) + \
     fRect.left())

namespace {
QColor mixColors(const QColor &a, const QColor &b, qreal t) {
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t);
}
// room under the curve for the scale and its numbers
const int kScaleHeight = 13;
}  // namespace

void LuminanceRangeWidget::paintEvent(QPaintEvent * /*pe */) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect fRect = getPaintRect();
    if (fRect.width() < 50)  // Does not make sense to paint anything
        return;
    const qreal top = fRect.top();
    const qreal bottom = fRect.bottom() + 1 - kScaleHeight;  // curve base

    // colours follow the light or dark design
    const QColor back = palette().color(QPalette::Window);
    const QColor text = palette().color(QPalette::WindowText);
    const bool dark = back.lightness() < 128;
    const QColor accent = palette().color(QPalette::Highlight);
    const QColor fill = mixColors(back, text, dark ? 0.26 : 0.20);
    const QColor axis = mixColors(back, text, dark ? 0.22 : 0.18);
    const QColor muted = mixColors(back, text, 0.55);

    // the chosen range as a lightly tinted band
    const qreal x1 = getWindowX(draggedMin());
    const qreal x2 = getWindowX(draggedMax());
    QColor band = accent;
    band.setAlphaF(mouseDragStart == DRAGNOTSTARTED ? (dark ? 0.16 : 0.13)
                                                    : (dark ? 0.24 : 0.20));
    p.fillRect(QRectF(x1, top, x2 - x1, bottom - top), band);

    // the brightness distribution as a smooth, soft area
    if (histogramImage != nullptr) {
        if (histogram == nullptr || histogram->getBins() != fRect.width()) {
            delete histogram;
            // Build histogram from at least 5000 pixels
            int accuracy =
                histogramImage->getRows() * histogramImage->getCols() / 5000;
            if (accuracy < 1) accuracy = 1;
            histogram = new Histogram(fRect.width(), accuracy);
            histogram->computeLog(histogramImage, minValue, maxValue);
        }
        const int bins = histogram->getBins();
        // square root keeps small parts visible next to a tall peak,
        // a light blur takes the steps out
        std::vector<float> values(bins);
        for (int i = 0; i < bins; ++i)
            values[i] = std::sqrt(qMax(histogram->getP(i), 0.f));
        std::vector<float> soft(bins);
        const int radius = 3;
        float top_value = 0.f;
        for (int i = 0; i < bins; ++i) {
            float sum = 0.f, weight = 0.f;
            for (int k = -radius; k <= radius; ++k) {
                const int j = i + k;
                if (j < 0 || j >= bins) continue;
                const float g = std::exp(-float(k * k) / 4.5f);
                sum += values[j] * g;
                weight += g;
            }
            soft[i] = sum / weight;
            top_value = qMax(top_value, soft[i]);
        }
        if (top_value > 0.f) {
            QPainterPath area;
            area.moveTo(fRect.left(), bottom);
            for (int i = 0; i < bins; ++i)
                area.lineTo(fRect.left() + i + 0.5,
                            bottom - soft[i] / top_value * (bottom - top - 2));
            area.lineTo(fRect.left() + bins, bottom);
            area.closeSubpath();
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawPath(area);
        }
    }

    // the range's edges
    p.setPen(QPen(accent, 1.5));
    p.drawLine(QPointF(x1, top), QPointF(x1, bottom));
    p.drawLine(QPointF(x2, top), QPointF(x2, bottom));

    // the scale: equally fine ticks, small grey numbers
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(axis);
    p.drawLine(fRect.left(), int(bottom), fRect.right(), int(bottom));
    QFont labelFont = font();
    labelFont.setPixelSize(9);
    p.setFont(labelFont);
    const qreal unit = fRect.width() / (maxValue - minValue);
    const int every = unit < 14 ? 2 : 1;
    for (int v = int(std::ceil(minValue)); v <= int(std::floor(maxValue)); ++v) {
        const int rx = getWindowX(float(v));
        const bool major = v % 2 == 0;
        p.setPen(axis);
        p.drawLine(rx, int(bottom), rx, int(bottom) + (major ? 4 : 2));
        if (v % every == 0 && v != int(std::ceil(minValue)) &&
            v != int(std::floor(maxValue))) {
            p.setPen(muted);
            p.drawText(QRectF(rx - 12, bottom + 3, 24, kScaleHeight - 3),
                       Qt::AlignHCenter | Qt::AlignVCenter,
                       v > 0 ? QStringLiteral("+%1").arg(v) : QString::number(v));
        }
    }
    p.setRenderHint(QPainter::Antialiasing, true);

    // round handles on both edges
    const qreal mid = (top + bottom) / 2;
    p.setBrush(palette().color(QPalette::Base));
    p.setPen(QPen(accent, 2.0));
    p.drawEllipse(QPointF(x1, mid), 4.5, 4.5);
    p.drawEllipse(QPointF(x2, mid), 4.5, 4.5);

    //   Paint value pointer
    if (showVP) {
        const int x = getWindowX(valuePointer);
        if (fRect.contains(x, fRect.y())) {
            p.setRenderHint(QPainter::Antialiasing, false);
            p.setPen(QPen(text, 1.0));
            p.drawLine(x, int(top), x, int(bottom));
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
        QRect fRect = getPaintRect();
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
        pfs::utils::robustWindow(*histogramImage, windowMin, windowMax);
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
    // room at the sides for the round handles
    return frameRect().adjusted(7, 2, -7, -1);
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
