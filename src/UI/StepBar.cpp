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

#include "StepBar.h"

#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QToolButton>

StepBar::StepBar(QWidget *parent) : QWidget(parent), m_current(Import) {
    setObjectName(QStringLiteral("StepBar"));
    // fill the rest of the toolbar row, steps centred in it
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addStretch();
    for (int i = 0; i < 3; ++i) {
        if (i > 0) {
            QLabel *arrow = new QLabel(QStringLiteral("›"), this);
            arrow->setEnabled(false);
            layout->addWidget(arrow);
        }
        m_buttons[i] = new QToolButton(this);
        m_buttons[i]->setCheckable(true);
        m_buttons[i]->setAutoRaise(true);
        m_buttons[i]->setToolButtonStyle(Qt::ToolButtonTextOnly);
        // the highlight follows the current step, never a click
        connect(m_buttons[i], &QToolButton::toggled, this,
                [this](bool) { refresh(); });
        layout->addWidget(m_buttons[i]);
    }
    layout->addStretch();
    connect(m_buttons[Tone], &QToolButton::clicked, this,
            &StepBar::toneRequested);
    // palette() keeps the colours right in light and dark schemes
    setStyleSheet(QStringLiteral(
        "#StepBar QToolButton { padding: 3px 14px; border-radius: 11px; }"
        "#StepBar QToolButton:checked { background: palette(highlight);"
        " color: palette(highlighted-text); font-weight: bold; }"));
    retranslate();
    refresh();
    setToneEnabled(false);
    setExportEnabled(false);
}

void StepBar::setImportActions(QAction *newHdr, QAction *open) {
    QToolButton *b = m_buttons[Import];
    connect(b, &QToolButton::clicked, newHdr, &QAction::trigger);
    QMenu *menu = new QMenu(b);
    menu->addAction(newHdr);
    menu->addAction(open);
    b->setMenu(menu);
    b->setPopupMode(QToolButton::MenuButtonPopup);
}

void StepBar::setExportActions(QAction *saveAs, QAction *saveAll) {
    QToolButton *b = m_buttons[Export];
    connect(b, &QToolButton::clicked, saveAs, &QAction::trigger);
    QMenu *menu = new QMenu(b);
    menu->addAction(saveAs);
    menu->addAction(saveAll);
    b->setMenu(menu);
    b->setPopupMode(QToolButton::MenuButtonPopup);
}

void StepBar::setCurrentStep(Step step) {
    m_current = step;
    refresh();
}

void StepBar::setToneEnabled(bool enabled) {
    m_buttons[Tone]->setEnabled(enabled);
}

void StepBar::setExportEnabled(bool enabled) {
    m_buttons[Export]->setEnabled(enabled);
}

void StepBar::refresh() {
    for (int i = 0; i < 3; ++i) {
        const bool current = i == m_current;
        if (m_buttons[i]->isChecked() != current) {
            const bool blocked = m_buttons[i]->blockSignals(true);
            m_buttons[i]->setChecked(current);
            m_buttons[i]->blockSignals(blocked);
        }
    }
}

void StepBar::retranslate() {
    m_buttons[Import]->setText(tr("1  Import / Merge"));
    m_buttons[Import]->setToolTip(
        tr("Create a new HDR image from bracketed exposures, or open an "
           "existing image"));
    m_buttons[Tone]->setText(tr("2  Tone map"));
    m_buttons[Tone]->setToolTip(
        tr("Tone map the HDR image with the settings in the panel on the "
           "left"));
    m_buttons[Export]->setText(tr("3  Export"));
    m_buttons[Export]->setToolTip(
        tr("Save the current image; all results are in the menu"));
}

void StepBar::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) retranslate();
    QWidget::changeEvent(event);
}
