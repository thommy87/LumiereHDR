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

#include "PresetStrip.h"

#include <QApplication>
#include <QGridLayout>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QScopedPointer>
#include <QToolButton>

#include <cmath>

#include "Core/TMWorker.h"
#include "Core/TonemappingOptions.h"
#include "Fileformat/pfsoutldrimage.h"
#include "Libpfs/frame.h"
#include "Libpfs/manip/copy.h"
#include "Libpfs/manip/resize.h"
#include "Libpfs/utils/robustrange.h"
#include "PreviewPanel/PreviewLabel.h"

PresetStrip::PresetStrip(QWidget *parent) : QScrollArea(parent) {
    m_preview = QSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
    QWidget *row = new QWidget(this);
    row->setObjectName(QStringLiteral("PresetGrid"));
    m_layout = new QGridLayout(row);
    m_layout->setContentsMargins(6, 4, 6, 4);
    m_layout->setSpacing(6);
    setWidget(row);
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    // one row: preview, name and room for the scroll bar
    setFixedHeight(PREVIEW_HEIGHT + 60);
}

void PresetStrip::setPresets(const QList<QPair<QString, QString>> &presets) {
    qDeleteAll(m_buttons);
    m_buttons.clear();
    m_presets = presets;
    for (const auto &preset : m_presets) {
        QToolButton *button = new QToolButton(widget());
        button->setText(preset.first);
        button->setToolTip(tr("Apply the preset \"%1\"").arg(preset.first));
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setObjectName(QStringLiteral("LookButton"));
        button->setIconSize(m_preview);
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setCursor(Qt::PointingHandCursor);
        const QString file = preset.second;
        const QString name = preset.first;
        connect(button, &QToolButton::clicked, this,
                [this, file, name]() { emit presetClicked(file, name); });
        m_buttons << button;
    }
    placeButtons();
}

void PresetStrip::setColumns(int columns, const QSize &preview) {
    m_columns = columns;
    m_preview = preview;
    for (QToolButton *button : m_buttons) button->setIconSize(m_preview);
    if (m_columns > 0) {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setMinimumHeight(0);
        setMaximumHeight(QWIDGETSIZE_MAX);
        m_layout->setContentsMargins(12, 12, 12, 12);
        m_layout->setHorizontalSpacing(8);
        m_layout->setVerticalSpacing(10);
    }
    placeButtons();
}

void PresetStrip::placeButtons() {
    // takes the buttons out and puts them back in their new places
    while (QLayoutItem *item = m_layout->takeAt(0)) delete item;
    for (int i = 0; i < m_buttons.size(); ++i) {
        if (m_columns > 0) {
            m_buttons[i]->setSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Fixed);
            m_layout->addWidget(m_buttons[i], i / m_columns, i % m_columns);
        } else {
            m_layout->addWidget(m_buttons[i], 0, i);
        }
    }
    // the rest of the room stays empty, after the last row or column
    if (m_columns > 0) {
        for (int c = 0; c < m_columns; ++c) m_layout->setColumnStretch(c, 1);
        m_layout->setRowStretch((m_buttons.size() + m_columns - 1) / m_columns,
                                1);
    } else {
        m_layout->setColumnStretch(m_buttons.size(), 1);
    }
}

namespace {
//! fills the preview size, cut to it in the middle like a photo frame
QIcon coverIcon(const QImage &image, const QSize &size) {
    if (image.isNull()) return QIcon();
    const qreal dpr = 2.0;
    const QSize target = size * dpr;
    QImage scaled = image.scaled(target, Qt::KeepAspectRatioByExpanding,
                                 Qt::SmoothTransformation);
    scaled = scaled.copy((scaled.width() - target.width()) / 2,
                         (scaled.height() - target.height()) / 2,
                         target.width(), target.height());
    // rounded corners like the rest of the window
    QImage rounded(target, QImage::Format_ARGB32_Premultiplied);
    rounded.fill(Qt::transparent);
    QPainter p(&rounded);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QBrush(scaled));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(QPointF(0, 0), QSizeF(target)), 6 * dpr, 6 * dpr);
    p.end();
    QPixmap pixmap = QPixmap::fromImage(rounded);
    pixmap.setDevicePixelRatio(dpr);
    return QIcon(pixmap);
}
}  // namespace

void PresetStrip::setCurrent(const QString &name) {
    for (QToolButton *button : m_buttons)
        button->setChecked(!name.isEmpty() && button->text() == name);
}

void PresetStrip::updatePreviews(pfs::Frame *hdr) {
    if (hdr == nullptr || m_buttons.isEmpty()) return;

    // sharp on high-density screens: twice the shown size
    int width = qMax(PREVIEW_WIDTH, m_preview.width() * 2);
    const int height = qMax(PREVIEW_HEIGHT, m_preview.height() * 2);
    if (hdr->getHeight() * width > hdr->getWidth() * height)
        width = int(height * hdr->getWidth() / hdr->getHeight());
    else if (m_columns > 0)
        // a cover crop needs the full height of the preview
        width = qMax(width, int(height * hdr->getWidth() / hdr->getHeight()));
    QScopedPointer<pfs::Frame> small(pfs::resize(hdr, width, BilinearInterp));

    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (int i = 0; i < m_buttons.size(); ++i) {
        QIcon icon;
        // "Unedited" (TonemappingPanel::uneditedPreset): the HDR as the HDR
        // tab first shows it
        if (m_presets[i].second == QLatin1String("unedited:")) {
            float logMin = 0.f, logMax = 1.f;
            if (const pfs::Array2Df *y = small->getChannel("Y"))
                pfs::utils::robustWindow(*y, logMin, logMax);
            QScopedPointer<QImage> image(fromLDRPFStoQImage(
                small.data(), std::pow(10.f, logMin), std::pow(10.f, logMax),
                MAP_GAMMA2_2));
            m_buttons[i]->setIcon(m_columns > 0
                                      ? coverIcon(*image, m_preview)
                                      : QIcon(QPixmap::fromImage(*image)));
            continue;
        }
        try {
            QScopedPointer<TonemappingOptions> opts(
                TMOptionsOperations::parseFile(m_presets[i].second));
            opts->origxsize = small->getWidth();
            opts->xsize = small->getWidth();
            opts->tonemapSelection = false;
            // the operators work in place
            QScopedPointer<pfs::Frame> copy(pfs::copy(small.data()));
            TMWorker worker;
            QScopedPointer<pfs::Frame> result(
                worker.computeTonemap(copy.data(), opts.data(), BilinearInterp));
            if (result) {
                QScopedPointer<QImage> image(fromLDRPFStoQImage(result.data()));
                icon = m_columns > 0 ? coverIcon(*image, m_preview)
                                     : QIcon(QPixmap::fromImage(*image));
            }
        } catch (...) {
            // an unreadable preset keeps an empty preview
        }
        m_buttons[i]->setIcon(icon);
    }
    QApplication::restoreOverrideCursor();
}
