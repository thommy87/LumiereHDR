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

#include "TopBar.h"

#include <QButtonGroup>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QToolButton>

#include "LumiereIcons.h"

namespace {
QToolButton *iconButton(QWidget *parent) {
    QToolButton *b = new QToolButton(parent);
    b->setAutoRaise(true);
    b->setIconSize(QSize(20, 20));
    b->setFixedSize(36, 36);
    return b;
}
}  // namespace

TopBar::TopBar(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("TopBar"));
    setAttribute(Qt::WA_StyledBackground);
    setFixedHeight(52);

    m_logo = new QLabel(this);
    m_logo->setPixmap(QIcon(QStringLiteral(":/program-icons/luminance-hdr"))
                          .pixmap(26, 26));
    m_name = new QLabel(QStringLiteral("LumièreHDR"), this);
    m_name->setObjectName(QStringLiteral("TopBarName"));

    QWidget *left = new QWidget(this);
    QHBoxLayout *leftLayout = new QHBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);
    leftLayout->addWidget(m_logo);
    leftLayout->addWidget(m_name);
    leftLayout->addStretch();

    m_modes = new QButtonGroup(this);
    m_modes->setExclusive(true);
    QWidget *middle = new QWidget(this);
    QHBoxLayout *middleLayout = new QHBoxLayout(middle);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(4);
    for (int i = 0; i < 4; ++i) {
        QToolButton *b = new QToolButton(middle);
        b->setObjectName(QStringLiteral("ModeButton"));
        b->setCheckable(true);
        b->setAutoRaise(true);
        b->setToolButtonStyle(i == Start ? Qt::ToolButtonTextBesideIcon
                                         : Qt::ToolButtonTextOnly);
        b->setIconSize(QSize(18, 18));
        b->setMinimumHeight(36);
        m_modes->addButton(b, i);
        m_modeButtons[i] = b;
        middleLayout->addWidget(b);
    }
    connect(m_modes, &QButtonGroup::idClicked, this, [this](int id) {
        emit modeRequested(static_cast<Mode>(id));
    });

    m_search = new QToolButton(this);
    m_search->setObjectName(QStringLiteral("SearchButton"));
    m_search->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_search->setIconSize(QSize(16, 16));
    m_search->setFixedHeight(32);
    m_search->setMinimumWidth(150);
    connect(m_search, &QToolButton::clicked, this, &TopBar::searchRequested);

    m_batch = iconButton(this);
    connect(m_batch, &QToolButton::clicked, this, &TopBar::batchRequested);
    m_settings = iconButton(this);
    connect(m_settings, &QToolButton::clicked, this,
            &TopBar::settingsRequested);
    m_menu = iconButton(this);
    m_menuPopup = new QMenu(m_menu);
    m_menu->setMenu(m_menuPopup);
    m_menu->setPopupMode(QToolButton::InstantPopup);

    QWidget *right = new QWidget(this);
    QHBoxLayout *rightLayout = new QHBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);
    rightLayout->addStretch();
    rightLayout->addWidget(m_search);
    rightLayout->addSpacing(4);
    rightLayout->addWidget(m_batch);
    rightLayout->addWidget(m_settings);
    rightLayout->addWidget(m_menu);

    // equal sides keep the workspaces in the middle of the window
    left->setFixedWidth(330);
    right->setFixedWidth(330);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 12, 0);
    layout->setSpacing(16);
    layout->addWidget(left);
    layout->addStretch();
    layout->addWidget(middle);
    layout->addStretch();
    layout->addWidget(right);

    retranslate();
    refreshIcons();
    setMode(Start);
}

void TopBar::setMode(Mode mode) {
    m_modeButtons[mode]->setChecked(true);
}

TopBar::Mode TopBar::mode() const {
    return static_cast<Mode>(m_modes->checkedId());
}

void TopBar::setModeEnabled(Mode mode, bool enabled) {
    m_modeButtons[mode]->setEnabled(enabled);
}

void TopBar::setMenus(const QList<QMenu *> &menus) {
    m_menuPopup->clear();
    for (QMenu *menu : menus) m_menuPopup->addMenu(menu);
}

void TopBar::retranslate() {
    m_modeButtons[Start]->setText(tr("Start"));
    m_modeButtons[Start]->setToolTip(
        tr("Start page: new HDR, open an image, recent files"));
    m_modeButtons[Merge]->setText(tr("1  Merge HDR"));
    m_modeButtons[Merge]->setToolTip(
        tr("Merge bracketed photos, or one RAW photo, into an HDR image"));
    m_modeButtons[Develop]->setText(tr("2  Develop"));
    m_modeButtons[Develop]->setToolTip(
        tr("Give the HDR image its look: style, light, colours"));
    m_modeButtons[Export]->setText(tr("3  Export"));
    m_modeButtons[Export]->setToolTip(
        tr("Save the result as JPEG, TIFF, Ultra HDR and more"));
    m_search->setText(tr("Search   Ctrl+K"));
    m_search->setToolTip(tr("Find any command or setting by its name"));
    m_batch->setToolTip(tr("Batch processing"));
    m_batch->setAccessibleName(tr("Batch processing"));
    m_settings->setToolTip(tr("Preferences"));
    m_settings->setAccessibleName(tr("Preferences"));
    m_menu->setToolTip(tr("All menus"));
    m_menu->setAccessibleName(tr("All menus"));
}

void TopBar::refreshIcons() {
    m_modeButtons[Start]->setIcon(LumiereIcons::icon(QStringLiteral("home")));
    m_search->setIcon(LumiereIcons::icon(QStringLiteral("search")));
    m_batch->setIcon(LumiereIcons::icon(QStringLiteral("batch")));
    m_settings->setIcon(LumiereIcons::icon(QStringLiteral("settings")));
    m_menu->setIcon(LumiereIcons::icon(QStringLiteral("menu")));
}

void TopBar::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) retranslate();
    if (event->type() == QEvent::PaletteChange) refreshIcons();
    QWidget::changeEvent(event);
}
