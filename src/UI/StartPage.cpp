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

#include "StartPage.h"

#include <QAction>
#include <QEvent>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include "LumiereIcons.h"

namespace {
const QSize kThumb(176, 118);

QLabel *quietLabel(QWidget *parent) {
    QLabel *l = new QLabel(parent);
    l->setAttribute(Qt::WA_TransparentForMouseEvents);
    return l;
}

//! a small preview of image files Qt reads quickly; HDR and RAW files get
//! a neutral tile instead of a slow decode at start-up
QPixmap thumbnail(const QString &file) {
    static const QStringList quick = {QStringLiteral("jpg"),
                                      QStringLiteral("jpeg"),
                                      QStringLiteral("png"),
                                      QStringLiteral("webp")};
    const QString suffix = QFileInfo(file).suffix().toLower();
    if (quick.contains(suffix) && QFileInfo::exists(file)) {
        QImageReader reader(file);
        reader.setAutoTransform(true);
        const QSize size = reader.size();
        if (size.isValid()) {
            reader.setScaledSize(
                size.scaled(kThumb * 2, Qt::KeepAspectRatioByExpanding));
            const QImage image = reader.read();
            if (!image.isNull()) {
                QImage cropped = image.copy(QRect(
                    (image.width() - kThumb.width() * 2) / 2,
                    (image.height() - kThumb.height() * 2) / 2,
                    kThumb.width() * 2, kThumb.height() * 2));
                QPixmap p = QPixmap::fromImage(cropped);
                p.setDevicePixelRatio(2.0);
                return p;
            }
        }
    }
    return LumiereIcons::icon(QStringLiteral("layers")).pixmap(40, 40);
}
}  // namespace

StartPage::StartPage(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("StartPage"));
    setAttribute(Qt::WA_StyledBackground);

    QWidget *column = new QWidget;
    column->setObjectName(QStringLiteral("StartColumn"));
    column->setMaximumWidth(1000);

    m_title = new QLabel(column);
    m_title->setObjectName(QStringLiteral("StartTitle"));
    m_subtitle = new QLabel(column);
    m_subtitle->setObjectName(QStringLiteral("StartSubtitle"));
    m_subtitle->setWordWrap(true);

    QHBoxLayout *cards = new QHBoxLayout;
    cards->setSpacing(16);
    for (Card &c : m_cards) {
        c = makeCard();
        cards->addWidget(c.button, 1);
    }
    m_cards[0].button->setProperty("primary", true);
    m_cards[0].iconName = QStringLiteral("layers");
    m_cards[1].iconName = QStringLiteral("folder");
    m_cards[2].iconName = QStringLiteral("batch");

    m_drop = new QLabel(column);
    m_drop->setObjectName(QStringLiteral("StartDrop"));
    m_drop->setAlignment(Qt::AlignCenter);
    m_drop->setMinimumHeight(88);

    m_recentLabel = new QLabel(column);
    m_recentLabel->setObjectName(QStringLiteral("SectionLabel"));
    m_recentBox = new QWidget(column);
    m_recentGrid = new QGridLayout(m_recentBox);
    m_recentGrid->setContentsMargins(0, 0, 0, 0);
    m_recentGrid->setHorizontalSpacing(16);

    m_links = new QWidget(column);
    QHBoxLayout *links = new QHBoxLayout(m_links);
    links->setContentsMargins(0, 0, 0, 0);
    links->setSpacing(24);
    links->addStretch();

    QVBoxLayout *col = new QVBoxLayout(column);
    col->setContentsMargins(0, 56, 0, 40);
    col->setSpacing(0);
    col->addWidget(m_title);
    col->addSpacing(8);
    col->addWidget(m_subtitle);
    col->addSpacing(32);
    col->addLayout(cards);
    col->addSpacing(24);
    col->addWidget(m_drop);
    col->addSpacing(32);
    col->addWidget(m_recentLabel);
    col->addSpacing(12);
    col->addWidget(m_recentBox);
    col->addSpacing(28);
    col->addWidget(m_links);
    col->addStretch();

    // centred, and scrolls when the window is small
    QWidget *canvas = new QWidget;
    canvas->setObjectName(QStringLiteral("StartCanvas"));
    QHBoxLayout *centre = new QHBoxLayout(canvas);
    centre->setContentsMargins(32, 0, 32, 0);
    centre->addStretch();
    centre->addWidget(column, 100);
    centre->addStretch();
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("StartScroll"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(canvas);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    retranslate();
    refreshIcons();
    rebuildRecent();
}

StartPage::Card StartPage::makeCard() {
    Card c;
    c.action = nullptr;
    c.button = new QPushButton(this);
    c.button->setObjectName(QStringLiteral("StartCard"));
    c.button->setMinimumHeight(196);
    c.button->setCursor(Qt::PointingHandCursor);
    c.icon = quietLabel(c.button);
    c.icon->setObjectName(QStringLiteral("StartCardIcon"));
    c.icon->setFixedSize(48, 48);
    c.icon->setAlignment(Qt::AlignCenter);
    c.title = quietLabel(c.button);
    c.title->setObjectName(QStringLiteral("StartCardTitle"));
    c.text = quietLabel(c.button);
    c.text->setObjectName(QStringLiteral("StartCardText"));
    c.text->setWordWrap(true);
    c.shortcut = quietLabel(c.button);
    c.shortcut->setObjectName(QStringLiteral("Keycap"));
    QVBoxLayout *l = new QVBoxLayout(c.button);
    l->setContentsMargins(24, 24, 24, 20);
    l->setSpacing(6);
    l->addWidget(c.icon);
    l->addSpacing(8);
    l->addWidget(c.title);
    l->addWidget(c.text);
    l->addStretch();
    l->addWidget(c.shortcut, 0, Qt::AlignLeft);
    return c;
}

void StartPage::setCardActions(QAction *newHdr, QAction *open,
                               QAction *batch) {
    QAction *actions[3] = {newHdr, open, batch};
    for (int i = 0; i < 3; ++i) {
        m_cards[i].action = actions[i];
        connect(m_cards[i].button, &QPushButton::clicked, actions[i],
                &QAction::trigger);
    }
    retranslate();
}

void StartPage::setLinkActions(const QList<QAction *> &links) {
    QHBoxLayout *layout = static_cast<QHBoxLayout *>(m_links->layout());
    for (QAction *a : links) {
        QToolButton *b = new QToolButton(m_links);
        b->setObjectName(QStringLiteral("LinkButton"));
        b->setDefaultAction(a);
        b->setToolButtonStyle(Qt::ToolButtonTextOnly);
        b->setCursor(Qt::PointingHandCursor);
        layout->insertWidget(layout->count() - 1, b);
    }
}

void StartPage::setRecentFiles(const QStringList &files) {
    if (files == m_recent) return;
    m_recent = files;
    rebuildRecent();
}

void StartPage::rebuildRecent() {
    while (QLayoutItem *item = m_recentGrid->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    const int count = qMin(5, m_recent.size());
    m_recentLabel->setVisible(count > 0);
    m_recentBox->setVisible(count > 0);
    for (int i = 0; i < count; ++i) {
        const QString file = m_recent.at(i);
        QToolButton *b = new QToolButton(m_recentBox);
        b->setObjectName(QStringLiteral("RecentTile"));
        b->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        b->setIcon(QIcon(thumbnail(file)));
        b->setIconSize(kThumb);
        b->setText(QFontMetrics(b->font())
                       .elidedText(QFileInfo(file).fileName(), Qt::ElideMiddle,
                                   kThumb.width()));
        b->setToolTip(file);
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QToolButton::clicked, this,
                [this, file]() { emit recentFileRequested(file); });
        m_recentGrid->addWidget(b, 0, i, Qt::AlignLeft);
    }
    m_recentGrid->setColumnStretch(5, 1);
}

void StartPage::retranslate() {
    m_title->setText(tr("What would you like to start with?"));
    m_subtitle->setText(tr("Turn several exposures into one picture that "
                           "holds all the light of the scene."));
    m_cards[0].title->setText(tr("Bracketed photos to HDR"));
    m_cards[0].text->setText(
        tr("3 to 9 photos of the same scene, or one RAW photo"));
    m_cards[1].title->setText(tr("Open HDR image"));
    m_cards[1].text->setText(
        tr("EXR, HDR, TIFF, PFM, AVIF, JPEG XL and more"));
    m_cards[2].title->setText(tr("Batch processing"));
    m_cards[2].text->setText(
        tr("Whole folders: find brackets, make HDRs, apply looks"));
    for (Card &c : m_cards) {
        const QString keys =
            c.action ? c.action->shortcut().toString(QKeySequence::NativeText)
                     : QString();
        c.shortcut->setText(keys);
        c.shortcut->setVisible(!keys.isEmpty());
        c.button->setAccessibleName(c.title->text());
        c.button->setAccessibleDescription(c.text->text());
    }
    m_drop->setText(tr("Or simply drop photos or HDR files here"));
    m_recentLabel->setText(tr("RECENTLY USED"));
}

void StartPage::refreshIcons() {
    for (Card &c : m_cards)
        if (!c.iconName.isEmpty())
            c.icon->setPixmap(LumiereIcons::icon(c.iconName).pixmap(26, 26));
    rebuildRecent();
}

void StartPage::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) retranslate();
    if (event->type() == QEvent::PaletteChange) refreshIcons();
    QWidget::changeEvent(event);
}
