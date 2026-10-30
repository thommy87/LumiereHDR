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

#include "FilmStrip.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QCursor>
#include <QHBoxLayout>
#include <QListWidget>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>

#include "Viewers/GenericViewer.h"

namespace {
const QSize kThumb(116, 62);

QString shownName(QString text) {
    if (text.startsWith(QLatin1String("(*) "))) text.remove(0, 4);
    return text;
}
}  // namespace

FilmStrip::FilmStrip(QTabWidget *tabs, QWidget *parent)
    : QWidget(parent), m_tabs(tabs) {
    setObjectName(QStringLiteral("FilmStrip"));
    setAttribute(Qt::WA_StyledBackground);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("FilmList"));
    m_list->setViewMode(QListView::IconMode);
    m_list->setFlow(QListView::LeftToRight);
    m_list->setWrapping(false);
    m_list->setMovement(QListView::Static);
    m_list->setIconSize(kThumb);
    m_list->setGridSize(QSize(kThumb.width() + 16, kThumb.height() + 34));
    m_list->setTextElideMode(Qt::ElideMiddle);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setFixedHeight(kThumb.height() + 48);
    m_list->viewport()->installEventFilter(this);
    m_list->installEventFilter(this);
    m_list->setToolTip(
        tr("Right-click a picture or press Del to close it"));

    m_buttons = new QWidget(this);
    QHBoxLayout *buttons = new QHBoxLayout(m_buttons);
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->setSpacing(8);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 14, 0);
    layout->setSpacing(12);
    layout->addWidget(m_list, 1);
    layout->addWidget(m_buttons);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0 && row < m_tabs->count() && row != m_tabs->currentIndex())
            m_tabs->setCurrentIndex(row);
    });
    connect(m_tabs, &QTabWidget::currentChanged, this, &FilmStrip::refresh);

    // the tab widget tells nobody about new or changed pictures, so the
    // strip looks again now and then while it is shown
    m_timer = new QTimer(this);
    m_timer->setInterval(700);
    connect(m_timer, &QTimer::timeout, this, &FilmStrip::refresh);
}

void FilmStrip::addButton(QAction *action) {
    QToolButton *b = new QToolButton(m_buttons);
    b->setObjectName(QStringLiteral("FilmButton"));
    b->setDefaultAction(action);
    b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_buttons->layout()->addWidget(b);
}

void FilmStrip::refresh() {
    QStringList signature;
    for (int i = 0; i < m_tabs->count(); ++i) {
        GenericViewer *v = qobject_cast<GenericViewer *>(m_tabs->widget(i));
        signature << QStringLiteral("%1|%2|%3")
                         .arg(quintptr(m_tabs->widget(i)))
                         .arg(v ? v->pictureKey() : 0)
                         .arg(m_tabs->tabText(i));
    }
    if (signature != m_signature) {
        m_signature = signature;
        const QSignalBlocker block(m_list);
        m_list->clear();
        for (int i = 0; i < m_tabs->count(); ++i) {
            GenericViewer *v = qobject_cast<GenericViewer *>(m_tabs->widget(i));
            const QString name = shownName(m_tabs->tabText(i));
            QListWidgetItem *item = new QListWidgetItem(
                v && v->isHDR() ? tr("HDR · %1").arg(name) : name, m_list);
            if (v) {
                QPixmap thumb = v->thumbnail(kThumb * 2);
                thumb.setDevicePixelRatio(2.0);
                item->setIcon(QIcon(thumb));
            }
            item->setToolTip(m_tabs->tabText(i));
            item->setSizeHint(m_list->gridSize());
        }
    }
    const QSignalBlocker block(m_list);
    m_list->setCurrentRow(m_tabs->currentIndex());
    if (m_list->currentItem()) m_list->scrollToItem(m_list->currentItem());
}

bool FilmStrip::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_list && event->type() == QEvent::KeyPress) {
        // Del closes the picture picked in the strip
        const int key = static_cast<QKeyEvent *>(event)->key();
        if ((key == Qt::Key_Delete || key == Qt::Key_Backspace) &&
            m_list->currentRow() >= 0) {
            emit closeRequested(m_list->currentRow());
            return true;
        }
    }
    if (watched == m_list->viewport()) {
        // close with the middle button or from the context menu
        int row = -1;
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *e = static_cast<QMouseEvent *>(event);
            if (e->button() == Qt::MiddleButton) {
                row = m_list->row(m_list->itemAt(
                    m_list->viewport()->mapFromGlobal(QCursor::pos())));
                if (row >= 0) emit closeRequested(row);
                return row >= 0;
            }
        } else if (event->type() == QEvent::ContextMenu) {
            QContextMenuEvent *e = static_cast<QContextMenuEvent *>(event);
            row = m_list->row(m_list->itemAt(e->pos()));
            if (row >= 0) {
                QMenu menu(this);
                QAction *close = menu.addAction(tr("Close"));
                if (menu.exec(e->globalPos()) == close) emit closeRequested(row);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FilmStrip::showEvent(QShowEvent *event) {
    refresh();
    m_timer->start();
    QWidget::showEvent(event);
}

void FilmStrip::hideEvent(QHideEvent *event) {
    m_timer->stop();
    QWidget::hideEvent(event);
}
