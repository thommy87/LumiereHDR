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
#include <QHBoxLayout>
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
    QWidget *row = new QWidget(this);
    m_layout = new QHBoxLayout(row);
    m_layout->setContentsMargins(6, 4, 6, 4);
    m_layout->setSpacing(6);
    m_layout->addStretch();
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
        button->setIconSize(QSize(PREVIEW_WIDTH, PREVIEW_HEIGHT));
        button->setCheckable(true);
        button->setAutoRaise(true);
        const QString file = preset.second;
        const QString name = preset.first;
        connect(button, &QToolButton::clicked, this,
                [this, file, name]() { emit presetClicked(file, name); });
        // before the trailing stretch
        m_layout->insertWidget(m_layout->count() - 1, button);
        m_buttons << button;
    }
}

void PresetStrip::setCurrent(const QString &name) {
    for (QToolButton *button : m_buttons)
        button->setChecked(!name.isEmpty() && button->text() == name);
}

void PresetStrip::updatePreviews(pfs::Frame *hdr) {
    if (hdr == nullptr || m_buttons.isEmpty()) return;

    int width = PREVIEW_WIDTH;
    if (hdr->getHeight() > hdr->getWidth())
        width = PREVIEW_HEIGHT * hdr->getWidth() / hdr->getHeight();
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
            m_buttons[i]->setIcon(QIcon(QPixmap::fromImage(*image)));
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
                icon = QIcon(QPixmap::fromImage(*image));
            }
        } catch (...) {
            // an unreadable preset keeps an empty preview
        }
        m_buttons[i]->setIcon(icon);
    }
    QApplication::restoreOverrideCursor();
}
