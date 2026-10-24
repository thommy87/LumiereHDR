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

#include "HistogramPanel.h"

#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>

#include "GammaAndLevels.h"

namespace {
// above this share of pixels, clipping is worth a warning
const double kWarnPercent = 0.5;
// the histogram does not need every pixel of a large image
const int kMaxSide = 1000;
}  // namespace

HistogramPanel::HistogramPanel(QWidget *parent) : QWidget(parent) {
    m_histogram = new HistogramLDR(this);
    m_histogram->setColorHistogram(true);
    m_histogram->setToolTip(
        tr("Brightness distribution of the shown image, from black on the "
           "left to white on the right. Double-click to switch between "
           "colour and grey."));
    m_highlights = new QLabel(this);
    m_shadows = new QLabel(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_histogram, 1);
    layout->addWidget(m_highlights);
    layout->addWidget(m_shadows);
    setImage(QImage());
}

void HistogramPanel::setImage(const QImage &source) {
    QImage image = source;
    if (!image.isNull() &&
        qMax(image.width(), image.height()) > kMaxSide)
        image = image.scaled(kMaxSide, kMaxSide, Qt::KeepAspectRatio,
                             Qt::FastTransformation);
    if (!image.isNull()) image = image.convertToFormat(QImage::Format_RGB32);

    m_histogram->setData(&image);
    m_histogram->update();

    if (image.isNull()) {
        m_highlights->setText(tr("No image shown"));
        m_shadows->clear();
        return;
    }

    // a pixel is clipped when a channel sits at pure white, or all at black
    qint64 bright = 0, dark = 0;
    for (int y = 0; y < image.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb p = line[x];
            if (qRed(p) == 255 || qGreen(p) == 255 || qBlue(p) == 255) ++bright;
            if (qRed(p) == 0 && qGreen(p) == 0 && qBlue(p) == 0) ++dark;
        }
    }
    const double total = double(image.width()) * image.height();
    auto show = [](QLabel *label, const QString &text, double percent) {
        label->setText(text.arg(QLocale().toString(percent, 'f', 1)));
        // warning colour from the palette keeps both schemes readable
        label->setStyleSheet(percent > kWarnPercent
                                 ? QStringLiteral("color: #e8590c; "
                                                  "font-weight: bold;")
                                 : QString());
    };
    show(m_highlights, tr("Clipped highlights: %1 %"), 100.0 * bright / total);
    show(m_shadows, tr("Clipped shadows: %1 %"), 100.0 * dark / total);
}
