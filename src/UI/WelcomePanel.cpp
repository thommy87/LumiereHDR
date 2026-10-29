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

#include "WelcomePanel.h"

#include <QAction>
#include <QEvent>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

WelcomePanel::WelcomePanel(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("WelcomePanel"));

    QLabel *logo = new QLabel(this);
    logo->setPixmap(
        QIcon(QStringLiteral(":/program-icons/luminance-hdr")).pixmap(96, 96));
    logo->setAlignment(Qt::AlignCenter);

    m_title = new QLabel(this);
    m_title->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.5);
    titleFont.setBold(true);
    m_title->setFont(titleFont);

    // large enough to hit easily, the first one is the usual start
    m_newButton = new QPushButton(this);
    m_newButton->setDefault(true);
    m_openButton = new QPushButton(this);
    for (QPushButton *b : {m_newButton, m_openButton}) {
        b->setMinimumSize(280, 44);
        b->setIconSize(QSize(24, 24));
    }

    m_hint = new QLabel(this);
    m_hint->setAlignment(Qt::AlignCenter);
    m_hint->setWordWrap(true);

    QVBoxLayout *column = new QVBoxLayout;
    column->setSpacing(12);
    column->addWidget(logo);
    column->addWidget(m_title);
    column->addSpacing(8);
    column->addWidget(m_newButton, 0, Qt::AlignHCenter);
    column->addWidget(m_openButton, 0, Qt::AlignHCenter);
    column->addSpacing(8);
    column->addWidget(m_hint);

    // centred in the image area, whatever its size
    QHBoxLayout *row = new QHBoxLayout;
    row->addStretch();
    row->addLayout(column);
    row->addStretch();
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addLayout(row);
    layout->addStretch();

    retranslate();
}

void WelcomePanel::setActions(QAction *newHdr, QAction *open) {
    m_newButton->setIcon(newHdr->icon());
    m_openButton->setIcon(open->icon());
    connect(m_newButton, &QPushButton::clicked, newHdr, &QAction::trigger);
    connect(m_openButton, &QPushButton::clicked, open, &QAction::trigger);
}

void WelcomePanel::retranslate() {
    m_title->setText(tr("Start with your photos"));
    m_newButton->setText(tr("Create HDR from bracketed photos..."));
    m_newButton->setToolTip(
        tr("Merge several exposures of the same scene, or one RAW photo, "
           "into an HDR image"));
    m_openButton->setText(tr("Open HDR image..."));
    m_openButton->setToolTip(
        tr("Open an HDR image you already have (EXR, HDR, TIFF, ...)"));
    m_hint->setText(tr("or drop photos or an HDR image here"));
}

void WelcomePanel::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) retranslate();
    QWidget::changeEvent(event);
}
